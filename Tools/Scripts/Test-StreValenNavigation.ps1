#requires -Version 7.2
# Fail-closed tests; mocked boundaries never launch a tool or modify Data.
$ErrorActionPreference='Stop'
$module=Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
& $module {
    $script:passed=0
    function Reject($Body,$Pattern) {
        try { & $Body | Out-Null } catch {
            if ($_.Exception.Message -notmatch $Pattern) { throw }
            $script:passed++; return
        }
        throw "Expected rejection: $Pattern"
    }
    $request=@{Operation='FixValenNavigation';ShowXEdit=$true;Variant='A';EditorId='';Name=''}
    $request.Variant='B'
    Reject { New-StreSession @{} 'STRE_AlternateStart.esp' $request } 'no A/B'
    $request.Variant='A'; $request.ShowXEdit=$false
    Reject { New-StreSession @{} 'STRE_AlternateStart.esp' $request } 'ShowXEdit'
    $request.ShowXEdit=$true; $request.EditorId='STRE_Other'
    Reject { New-StreSession @{} 'STRE_AlternateStart.esp' $request } 'arbitrary'
    $request.EditorId=''
    function Assert-StreMutationGit { throw 'clean checkout gate reached' }
    Reject { New-StreSession @{} 'STRE_AlternateStart.esp' $request } 'clean checkout gate'
    function Assert-StreMutationGit { @{Branch='feat/test';Head='a'*40} }
    function Get-StreHash { '0'*64 }
    $config=@{RepoRoot='C:/synthetic';DataRoot='C:/synthetic/Data';XEditPath='C:/synthetic/xedit';AutoModRoot='C:/synthetic/tools';ImportScript='C:/synthetic/import.ps1'}
    Reject { New-StreSession $config 'STRE_AlternateStart.esp' $request } 'deployed D'
    $state=@{Schema=1;State='Imported';Plugin='STRE_AlternateStart.esp';Request=$request;Git=@{Branch='feat/test';Head='a'*40};Config=$config;XEditHash='A'*64}
    Reject { Import-StrePresentationValidated $config 'C:/synthetic' $state } 'awaiting human'
    $state.State='AwaitingHumanXEdit'; $state.Git.Head='b'*40
    Reject { Import-StrePresentationValidated $config 'C:/synthetic' $state } 'Git changed'
    $state.Git.Head='a'*40
    $changed=$config.Clone(); $changed.DataRoot='C:/other'
    Reject { Import-StrePresentationValidated $changed 'C:/synthetic' $state } 'configuration changed'
    function Assert-StreMatrixProtected { throw 'protected originals gate reached' }
    Reject { Import-StrePresentationValidated $config 'C:/synthetic' $state } 'protected originals gate'
    function Assert-StreMatrixProtected { }
    Reject { Import-StrePresentationValidated $config 'C:/synthetic' $state } 'HUMAN_XEDIT_REQUIRED'
    $root=Join-Path ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))) ('_audit/tests/navigation-'+[guid]::NewGuid().ToString('N'))
    $null=New-Item -ItemType Directory -Path $root
    $receipt=@{Schema=1;Reviewer='Synthetic test only';XEditSha256='A'*64;PluginSha256='0'*64;LogSha256='0'*64;
        SortMasters=$true;CheckForErrors=$true;SavedAndClosed=$true;ReopenedSavedFile=$true;SecondCheckForErrors=$true;NoErrorsReported=$true}
    foreach ($field in @('SortMasters','CheckForErrors','SavedAndClosed','ReopenedSavedFile','SecondCheckForErrors','NoErrorsReported')) {
        $bad=$receipt.Clone(); $bad[$field]='true'
        Write-StreJson (Join-Path $root 'validation.json') $bad
        Reject { Import-StrePresentationValidated $config $root $state } 'HUMAN_XEDIT_REQUIRED'
    }
    $bad=$receipt.Clone(); $bad.PluginSha256='F'*64
    Write-StreJson (Join-Path $root 'validation.json') $bad
    Reject { Import-StrePresentationValidated $config $root $state } 'saved bytes/tool'
    Write-StreJson (Join-Path $root 'validation.json') $receipt
    Reject { Import-StrePresentationValidated $config $root $state } 'human log missing'
    Set-Content -LiteralPath (Join-Path $root 'xedit-validation.log') 'Synthetic test, NOT an attestation.'
    function Invoke-StreMatrixPython($Config,$Script,$Arguments) {
        if ($Script -cne 'valen_navigation.py' -or '--saved' -cnotin $Arguments) { throw 'Wrong navigation audit route' }
        throw 'saved navigation audit gate reached'
    }
    function Assert-StreMatrixCI { throw 'FORBIDDEN CI' }
    Reject { Import-StrePresentationValidated $config $root $state } 'saved navigation audit gate'
    Write-Output "PASS: $script:passed navigation guard checks"
}
