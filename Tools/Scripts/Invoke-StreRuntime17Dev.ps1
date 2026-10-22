#requires -Version 7.2
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string]$ConfigPath,
    [ValidateSet('BuildDeploy','Verify','RestoreStaging','VerifyRestored')][string]$Mode = 'BuildDeploy',
    [string]$ReceiptPath,
    [string]$ExpectedCommit,
    [ValidateRange(1,16)][int]$Jobs = 6
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Stre.Runtime17Dev.psm1') -Force
# WhatIf is deliberately a configuration-only plan: no destination probes,
# path resolution, process enumeration, build, or receipt creation.
Invoke-R17Workflow -ConfigPath $ConfigPath -Mode $Mode -ReceiptPath $ReceiptPath `
    -ExpectedCommit $ExpectedCommit -Jobs $Jobs -PlanOnly:$WhatIfPreference
