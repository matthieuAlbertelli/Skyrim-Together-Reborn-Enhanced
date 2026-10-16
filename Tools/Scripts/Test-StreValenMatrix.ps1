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
    $temporaryParent=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $fixtureName='stre-matrix-identity-'+[guid]::NewGuid().ToString('N')
    $fixtureRoot=[IO.Path]::GetFullPath((Join-Path $temporaryParent $fixtureName))
    $null=New-Item -ItemType Directory -Path $fixtureRoot
    try {
        $gitPath=Join-Path $fixtureRoot 'git.psc'
        $livePath=Join-Path $fixtureRoot 'data.psc'
        $lf=[Text.Encoding]::UTF8.GetBytes("Scriptname STRE_Test`n; été`n")
        $crlf=[Text.Encoding]::UTF8.GetBytes("Scriptname STRE_Test`r`n; été`r`n")
        [IO.File]::WriteAllBytes($gitPath,$lf)
        [IO.File]::WriteAllBytes($livePath,$lf)
        $identity=Get-StreMatrixCompanionIdentity $gitPath $livePath
        if ($identity.Comparison -cne 'ByteIdentical' -or $identity.GitSha256 -cne $identity.DataSha256) { throw 'Exact identity failed.' }
        $script:matrixPassed++
        [IO.File]::WriteAllBytes($gitPath,$crlf)
        $gitBefore=Get-StreHash $gitPath
        $liveBefore=Get-StreHash $livePath
        $identity=Get-StreMatrixCompanionIdentity $gitPath $livePath
        if ($identity.Comparison -cne 'PscCrLfOnly' -or $identity.GitSha256 -ceq $identity.DataSha256 -or
            $identity.GitSha256 -cne $gitBefore -or $identity.DataSha256 -cne $liveBefore -or
            (Get-StreHash $gitPath) -cne $gitBefore -or (Get-StreHash $livePath) -cne $liveBefore) { throw 'PSC comparison must preserve both distinct raw hashes and files.' }
        $script:matrixPassed++
        [IO.File]::WriteAllBytes($livePath,[Text.Encoding]::UTF8.GetBytes("Scriptname STRE_Other`n; été`n"))
        Reject { Get-StreMatrixCompanionIdentity $gitPath $livePath } 'beyond PSC line endings'
        # PowerShell -ceq uses linguistic comparison and can ignore soft hyphen.
        # The bytes must remain significant even when the script looks identical.
        [IO.File]::WriteAllBytes($livePath,[Text.Encoding]::UTF8.GetBytes("Scriptname STRE_Test`n; été$([char]173)`n"))
        Reject { Get-StreMatrixCompanionIdentity $gitPath $livePath } 'beyond PSC line endings'
        [IO.File]::WriteAllBytes($livePath,([byte[]](239,187,191)+$lf))
        Reject { Get-StreMatrixCompanionIdentity $gitPath $livePath } 'beyond PSC line endings'
        [IO.File]::WriteAllBytes($livePath,[Text.Encoding]::UTF8.GetBytes("Scriptname STRE_Test`r; été`r"))
        Reject { Get-StreMatrixCompanionIdentity $gitPath $livePath } 'beyond PSC line endings'
        [IO.File]::WriteAllBytes($livePath,[Text.Encoding]::UTF8.GetBytes("Scriptname STRE_Test`n; été"))
        Reject { Get-StreMatrixCompanionIdentity $gitPath $livePath } 'beyond PSC line endings'
        $gitBinary=Join-Path $fixtureRoot 'git.pex'
        $liveBinary=Join-Path $fixtureRoot 'data.pex'
        [IO.File]::WriteAllBytes($gitBinary,$crlf)
        [IO.File]::WriteAllBytes($liveBinary,$lf)
        Reject { Get-StreMatrixCompanionIdentity $gitBinary $liveBinary } 'companion differs'
        [IO.File]::WriteAllBytes($gitPath,([byte[]](0)+$crlf))
        [IO.File]::WriteAllBytes($livePath,([byte[]](0)+$lf))
        Reject { Get-StreMatrixCompanionIdentity $gitPath $livePath } 'beyond PSC line endings'
    } finally {
        if ([IO.Path]::GetFileName($fixtureRoot) -cne $fixtureName -or
            [IO.Path]::GetFullPath((Split-Path -Parent $fixtureRoot)).TrimEnd('\') -cne $temporaryParent.TrimEnd('\')) { throw 'Unsafe fixture cleanup path.' }
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
    Write-Output "PASS: $script:matrixPassed matrix preparation safeguards"
}
