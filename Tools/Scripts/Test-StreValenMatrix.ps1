#requires -Version 7.2
# No game, xEdit, compilation or network. Tests fail-closed preparation gates.
$ErrorActionPreference='Stop'
$module=Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
& $module {
    $script:matrixPassed=0
    $script:matrixHead='a'*40
    $script:matrixRuns=@()
    function Invoke-StreProcess { param($File,$Arguments,$Directory) @{ExitCode=0;Err='';Out=(ConvertTo-Json -InputObject $script:matrixRuns -Depth 6 -Compress)} }
    function Reject($Body,$Pattern) {
        try { & $Body | Out-Null } catch {
            if ($_.Exception.Message -notmatch $Pattern) { throw }
            $script:matrixPassed++; return
        }
        throw "Expected rejection: $Pattern"
    }
    $config=@{RepoRoot=(Get-Location).Path}
    Reject { New-StreValenMatrixSession $config 'STRE_AlternateStart.esp' @{ShowXEdit=$false} } 'ShowXEdit'
    Reject { New-StreValenMatrixSession $config 'STRE_AlternateStart.esp' @{ShowXEdit=$true;EditorId='STRE_Arbitrary'} } 'Fixed matrix'
    Reject { Invoke-StreMatrixPython $config 'arbitrary.py' @() } 'Unsupported matrix'
    Reject { Assert-StreMatrixCI $config $script:matrixHead } 'CI must be green'
    foreach ($name in @('Build windows','Build linux')) {
        $script:matrixRuns+=@{name=$name;headSha=$script:matrixHead;status='completed';conclusion='success';createdAt='2026-10-16T00:00:00Z'}
    }
    if (@(Assert-StreMatrixCI $config $script:matrixHead).Count -ne 2) { throw 'Both exact-head CI evidence records required.' }
    $script:matrixPassed++
    foreach ($field in @('status','conclusion','headSha')) {
        $old=$script:matrixRuns[0][$field]
        $script:matrixRuns[0][$field]='wrong'
        Reject { Assert-StreMatrixCI $config $script:matrixHead } 'CI must be green'
        $script:matrixRuns[0][$field]=$old
    }
    $script:matrixRuns+=@{name='Build windows';headSha=$script:matrixHead;status='completed';conclusion='failure';createdAt='2026-10-16T01:00:00Z'}
    Reject { Assert-StreMatrixCI $config $script:matrixHead } 'CI must be green'
    Write-Output "PASS: $script:matrixPassed matrix preparation safeguards"
}
