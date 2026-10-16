# Internal bounded batches through the existing explicit CK importer.
# Never dot-source the build/deploy entry point or pass AllowDirtyTarget.
#requires -Version 7.2
param(
    [Parameter(Mandatory)][string]$ImportScript,
    [Parameter(Mandatory)][string]$Inventory,
    [Parameter(Mandatory)][string]$SourceDataRoot
)
$ErrorActionPreference = 'Stop'
$report = Get-Content -LiteralPath $Inventory -Raw | ConvertFrom-Json -AsHashtable
if ($report.success -isnot [bool] -or -not $report.success) { throw 'Unvalidated matrix inventory.' }
. (Join-Path $PSScriptRoot 'Stre.CkImportBatches.ps1')
Invoke-StreCkImportBatches -ImportScript $ImportScript -SourceDataRoot $SourceDataRoot `
    -RelativePath @($report.Files.Keys | Sort-Object)
