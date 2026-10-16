# Keep the existing CK importer below Windows' native command-line limit.
function Invoke-StreCkImportBatches {
    param(
        [Parameter(Mandatory)][string]$ImportScript,
        [Parameter(Mandatory)][string[]]$RelativePath,
        [Parameter(Mandatory)][string]$SourceDataRoot,
        [switch]$AllowDirtyTarget
    )
    $batch=[Collections.Generic.List[string]]::new()
    $characters=0
    $options=@{SourceDataRoot=$SourceDataRoot}
    if($AllowDirtyTarget){$options.AllowDirtyTarget=$true}
    foreach($relative in $RelativePath) {
        # The importer also sends these paths to native git status after copying.
        $cost=$relative.Length + 'GameFiles/Skyrim/'.Length + 3
        if($cost -gt 16000){throw 'CK import path exceeds the native command budget.'}
        if($batch.Count -and ($batch.Count -ge 64 -or $characters+$cost -gt 16000)) {
            & $ImportScript -RelativePath $batch.ToArray() @options
            if(-not $?){throw 'CK importer failed.'}
            $batch.Clear(); $characters=0
        }
        $batch.Add($relative); $characters+=$cost
    }
    if($batch.Count) {
        & $ImportScript -RelativePath $batch.ToArray() @options
        if(-not $?){throw 'CK importer failed.'}
    }
}
