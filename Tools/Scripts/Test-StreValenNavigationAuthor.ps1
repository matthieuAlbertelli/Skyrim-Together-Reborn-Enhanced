#requires -Version 7.2
# Synthetic integration only. No installed game files or human attestation.
param([Parameter(Mandatory)][string]$XEditPath)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$module=Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
& $module {
    param($Repo,$XEdit)
    Assert-GameProcessesStopped
    if ((Get-Item -LiteralPath $XEdit).VersionInfo.ProductName -cne 'xEdit') { throw 'Official xEdit required.' }
    $root=Join-Path $Repo ('_audit/modding/'+[guid]::NewGuid().ToString('N'))
    $config=@{RepoRoot=$Repo;XEditPath=(Resolve-StrePath $XEdit)}
    & python (Join-Path $Repo 'Tools/Scripts/valen/navigation/fixture.py') $root
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic fixture generation failed.' }
    Initialize-StreValenPhase $config $root
    $null=Invoke-StreMatrixPython $config 'valen_navigation.py' @('--root',$root,'--fixture')
    $protected=@{}
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root 'Data') -File) { $protected[$file.FullName]=Get-StreHash $file.FullName }
    Write-Host "SYNTHETIC NAVIGATION FIXTURE: $root"
    Invoke-StreValenAuthor $config $root DryRun
    foreach ($path in $protected.Keys) { if ((Get-StreHash $path) -cne $protected[$path]) { throw 'Fixture DryRun mutated input.' } }
    if (Test-Path (Join-Path $root 'authored.esp')) { throw 'DryRun created output.' }
    Move-Item -LiteralPath (Join-Path $root 'authoring-result.txt') -Destination (Join-Path $root 'dry-run-result.txt')
    Invoke-StreValenAuthor $config $root Prepare
    foreach ($path in $protected.Keys) { if ((Get-StreHash $path) -cne $protected[$path]) { throw 'Fixture Prepare mutated input/master.' } }
    $audit=Invoke-StreMatrixPython $config 'valen_navigation.py' @('--root',$root,'--fixture','--audit')
    Write-StreJson (Join-Path $root 'fixture-result.json') @{success=$true;Fixture=$root;Audit=$audit;Protected=$protected}
    @{success=$true;Fixture=$root;Audit=$audit;Scope='Synthetic serialization only; human xEdit/runtime NOT_TESTED'} | ConvertTo-Json -Depth 8
} $repo $XEditPath
