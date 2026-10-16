#requires -Version 7.2
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Preflight', 'Inspect', 'PapyrusStatus', 'DryRun', 'Prepare', 'ImportValidated')]
    [string]$Action,
    [string]$AutoModRoot = $env:STRE_AUTOMOD_ROOT,
    [string]$XEditPath = $env:STRE_XEDIT_PATH,
    [string]$DataRoot = $env:STRE_SKYRIM_DATA,
    [string]$Plugin = 'STRE_AlternateStart.esp',
    [ValidateSet('AddQuest', 'AddGlobal', 'AddValenHelgen', 'AddValenDialogueMatrixV1')][string]$Operation = 'AddGlobal',
    [switch]$ShowXEdit,
    [string]$EditorId,
    [string]$Name,
    [byte]$Priority = 50,
    [single]$Value = 0,
    [string]$Session,
    [string]$RecordType = 'quest'
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force
try {
    $config = Resolve-StreConfiguration -RepoRoot (Join-Path $PSScriptRoot '../..') `
        -AutoModRoot $AutoModRoot -XEditPath $XEditPath -DataRoot $DataRoot
    $request = @{ Operation = $Operation; EditorId = $EditorId; Name = $Name; Priority = $Priority; Value = $Value; ShowXEdit = [bool]$ShowXEdit }
    Invoke-StreModding -Config $config -Action $Action -Plugin $Plugin -Request $request `
        -Session $Session -RecordType $RecordType | ConvertTo-Json -Depth 32
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
