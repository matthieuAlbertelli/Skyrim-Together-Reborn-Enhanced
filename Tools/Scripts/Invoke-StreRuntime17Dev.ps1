#requires -Version 7.2
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string]$ConfigPath,
    [ValidateSet('BuildDeploy','Verify','RestoreStaging','VerifyRestored')][string]$Mode = 'BuildDeploy',
    [string]$ReceiptPath,
    [string]$ExpectedCommit,
    # Internal entry-selected source for a NEW build. Resume reads its receipt.
    [string]$SourceRepository,
    [ValidateRange(1,16)][int]$Jobs = 6
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Stre.Runtime17Dev.psm1') -Force
# WhatIf reads local config/source/receipt metadata only: no destination probes,
# process enumeration, build, or receipt creation.
Invoke-R17Workflow -ConfigPath $ConfigPath -Mode $Mode -ReceiptPath $ReceiptPath `
    -ExpectedCommit $ExpectedCommit -Jobs $Jobs -PlanOnly:$WhatIfPreference -SourceRepository $SourceRepository
