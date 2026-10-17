#requires -Version 7.2
# Official xEdit integration, invented synthetic masters and #94 fixture only.
param([Parameter(Mandatory)][string]$XEditPath)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$module=Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
& $module {
    param($Repo,$XEdit)
    Assert-GameProcessesStopped
    $root=Join-Path $Repo ('_audit/modding/'+[guid]::NewGuid().ToString('N'))
    $config=@{RepoRoot=$Repo;XEditPath=(Resolve-StrePath $XEdit)}
    if ((Get-Item -LiteralPath $XEdit).VersionInfo.ProductName -cne 'xEdit') { throw 'Official xEdit required.' }
    & python (Join-Path $Repo 'Tools/Scripts/valen/matrix/fixture.py') $root
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic fixture generation failed.' }
    $protected=@{}
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root 'Data') -File) { $protected[$file.FullName]=Get-StreHash $file.FullName }
    Initialize-StreMatrixPhase $config $root
    Write-Host "SYNTHETIC MATRIX FIXTURE: $root"
    Invoke-StreValenAuthor $config $root DryRun
    foreach ($path in $protected.Keys) { if ((Get-StreHash $path) -cne $protected[$path]) { throw 'Fixture DryRun mutated input.' } }
    if (Test-Path (Join-Path $root 'authored.esp')) { throw 'DryRun created an output.' }
    Move-Item -LiteralPath (Join-Path $root 'authoring-result.txt') -Destination (Join-Path $root 'dry-run-result.txt')
    Invoke-StreValenAuthor $config $root Prepare
    foreach ($path in $protected.Keys) { if ((Get-StreHash $path) -cne $protected[$path]) { throw 'Fixture Prepare mutated input/master.' } }
    $audit=Invoke-StreMatrixAudit $config $root (Join-Path $root 'authored.esp')
    Write-StreJson (Join-Path $root 'fixture-result.json') @{success=$true;Fixture=$root;Audit=$audit;Protected=$protected}
    @{success=$true;Fixture=$root;Audit=$audit;Scope='Synthetic authoring only, no human xEdit/runtime validation'} | ConvertTo-Json -Depth 8
} $repo $XEditPath
