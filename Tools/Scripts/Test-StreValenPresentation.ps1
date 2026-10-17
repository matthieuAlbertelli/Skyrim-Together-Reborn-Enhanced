#requires -Version 7.2
# Guard tests do not launch xEdit or touch game Data.
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
    Reject { New-StreValenPresentationSession @{} 'STRE_AlternateStart.esp' @{ShowXEdit=$false} } 'ShowXEdit'
    Reject { New-StreValenPresentationSession @{} 'STRE_AlternateStart.esp' @{ShowXEdit=$true;Variant='C'} } 'Only presentation'
    Reject { New-StreValenPresentationSession @{} 'STRE_AlternateStart.esp' @{ShowXEdit=$true;Variant='B';EditorId='STRE_Arbitrary';Name=''} } 'arbitrary'
    function Assert-StreMutationGit { throw 'clean checkout gate reached' }
    Reject { New-StreSession @{} 'STRE_AlternateStart.esp' @{Operation='FixValenPresentation';ShowXEdit=$true;Variant='B';EditorId='';Name=''} } 'clean checkout gate'
    function Assert-StreMutationGit { @{Branch='feat/test';Head='a'*40} }
    function Get-StreHash { '0'*64 }
    Reject { New-StreValenPresentationSession @{RepoRoot='C:/synthetic';DataRoot='C:/synthetic/Data'} 'STRE_AlternateStart.esp' @{ShowXEdit=$true;Variant='A';EditorId='';Name=''} } 'tested c48be63'
    Reject { Import-StrePresentationValidated @{} 'C:/synthetic' @{Schema=1;State='Imported';Request=@{Operation='FixValenPresentation';Variant='B'}} } 'awaiting human'
    Reject { Import-StrePresentationValidated @{} 'C:/synthetic' @{Schema=1;State='AwaitingHumanXEdit';Request=@{Operation='FixValenPresentation';Variant='C'}} } 'awaiting human'
    $config=@{RepoRoot='C:/synthetic';DataRoot='C:/synthetic/Data';XEditPath='C:/synthetic/xedit';AutoModRoot='C:/synthetic/tools';ImportScript='C:/synthetic/import.ps1'}
    $state=@{Schema=1;State='AwaitingHumanXEdit';Plugin='STRE_AlternateStart.esp';Request=@{Operation='FixValenPresentation';Variant='B'};Git=@{Branch='feat/test';Head='b'*40};Config=$config}
    Reject { Import-StrePresentationValidated $config 'C:/synthetic' $state } 'Git changed'
    $state.Git.Head='a'*40
    function Assert-StreMatrixProtected { }
    Reject { Import-StrePresentationValidated $config 'C:/synthetic' $state } 'HUMAN_XEDIT_REQUIRED'
    Write-Output "PASS: $script:passed presentation guard checks"
}
