#requires -Version 7.2
# Optional standalone local entry. Copy beside _audit/runtime17-dev.local.json
# as build-and-deploy-dev.ps1 only when restoring/creating a personal entry.
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][switch]$Runtime17TwoPc,
    [string]$Runtime17ConfigPath = (Join-Path $PSScriptRoot '_audit/runtime17-dev.local.json'),
    [ValidateSet('BuildDeploy','Verify','RestoreStaging','VerifyRestored')][string]$Runtime17Mode = 'BuildDeploy',
    [string]$Runtime17ReceiptRoot,
    [string]$Runtime17ExpectedCommit
)
$ErrorActionPreference='Stop'
if (!$Runtime17TwoPc -or !$Runtime17ConfigPath -or ![IO.Path]::IsPathFullyQualified($Runtime17ConfigPath) -or $Runtime17ConfigPath.StartsWith('\\')) { throw 'Select explicit mode and a local configuration.' }
$config=Get-Content -LiteralPath $Runtime17ConfigPath -Raw | ConvertFrom-Json
if (!$config.repository -or ![IO.Path]::IsPathFullyQualified($config.repository) -or $config.repository.StartsWith('\\')) { throw 'Select a local source worktree.' }
& (Join-Path $config.repository 'Tools/Scripts/Invoke-StreRuntime17Dev.ps1') -ConfigPath $Runtime17ConfigPath -Mode $Runtime17Mode -ReceiptPath $Runtime17ReceiptRoot -ExpectedCommit $Runtime17ExpectedCommit -WhatIf:$WhatIfPreference
