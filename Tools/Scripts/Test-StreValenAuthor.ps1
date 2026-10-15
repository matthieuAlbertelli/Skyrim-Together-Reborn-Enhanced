#requires -Version 7.2
# Interactive official xEdit integration test, ONLY synthetic master/plugin files.
param([Parameter(Mandatory)][string]$XEditPath)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$module = Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
& $module {
    param($Repo, $XEdit)
    Assert-GameProcessesStopped
    $root = Join-Path $Repo ('_audit/modding/' + [guid]::NewGuid().ToString('N'))
    $config = @{RepoRoot=$Repo; XEditPath=(Resolve-StrePath $XEdit)}
    & python (Join-Path $Repo 'Tools/Scripts/valen/fixture.py') $root
    if ($LASTEXITCODE -ne 0) { throw 'Synthetic fixture creation failed.' }
    $before = Get-StreHash (Join-Path $root 'before.esp')
    $masters = @{}
    foreach ($master in $script:Masters) { $masters[$master] = Get-StreHash (Join-Path $root "Data/$master") }
    Initialize-StreValenPhase $config $root
    Write-Host "SYNTHETIC FIXTURE: $root"
    Invoke-StreValenAuthor $config $root DryRun
    if ((Get-StreHash (Join-Path $root 'Data/STRE_AlternateStart.esp')) -ne $before -or (Test-Path (Join-Path $root 'authored.esp'))) { throw 'Fixture dry-run wrote content.' }
    Move-Item -LiteralPath (Join-Path $root 'authoring-result.txt') -Destination (Join-Path $root 'dry-run-result.txt')
    Invoke-StreValenAuthor $config $root Prepare
    $audit = Invoke-StreValenAudit $config $root (Join-Path $root 'authored.esp')
    if ((Get-StreHash (Join-Path $root 'Data/STRE_AlternateStart.esp')) -ne $before) { throw 'Fixture input modified.' }
    foreach ($master in $script:Masters) { if ((Get-StreHash (Join-Path $root "Data/$master")) -ne $masters[$master]) { throw 'Fixture master modified.' } }
    Write-StreJson (Join-Path $root 'fixture-audit.json') $audit
    @{Fixture=$root; Success=$true; Scope='Synthetic authoring and independent audit; NOT human xEdit validation'} | ConvertTo-Json
} $repo $XEditPath
