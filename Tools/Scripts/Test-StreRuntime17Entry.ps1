#requires -Version 7.2
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputRoot,
    [string]$EntryPath = (Join-Path $PSScriptRoot 'Start-StreRuntime17Dev.example.ps1')
)
$ErrorActionPreference='Stop'
# Only fresh local fixture repos, copied entries, inert helpers and pwsh children.
# No invocation targets the supplied entry in place. Legacy is never executed.
foreach ($path in @($OutputRoot,$EntryPath)) {
    if (![IO.Path]::IsPathFullyQualified($path) -or $path.StartsWith('\\') -or $path.StartsWith('//')) { throw 'Local absolute test paths required.' }
}
if (Test-Path -LiteralPath $OutputRoot) { throw 'Select a fresh test output directory.' }
$entryBytes=[IO.File]::ReadAllBytes($EntryPath)
$entryText=[Text.Encoding]::UTF8.GetString($entryBytes)
$example=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'Start-StreRuntime17Dev.example.ps1'))
$marker='# BEGIN PERSONAL LEGACY BODY'
if (!$entryText.Contains($marker) -or $entryText.Substring(0,$entryText.IndexOf($marker)).Replace([string][char]13,'') -cne $example.Substring(0,$example.IndexOf($marker)).Replace([string][char]13,'')) { throw 'Test the actual entry with the current versioned routing prefix.' }
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$results=[Collections.Generic.List[object]]::new()
function Check($Condition,$Message) { if (!$Condition) { throw $Message } }
function Case($Name,[scriptblock]$Body) { & $Body; $results.Add(@{case=$Name;result='PASS'}); Write-Host "PASS $Name" }
function Put($Path,$Text) { New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null; [IO.File]::WriteAllText($Path,$Text) }
function Invoke-FixtureGit($Repo,[string[]]$Arguments) {
    $out=@(& git --no-optional-locks -C $Repo @Arguments)
    if ($LASTEXITCODE) { throw "Fixture Git failed: $Arguments" }
    $out -join ''
}
$mockHelper=@'
[CmdletBinding(SupportsShouldProcess)]
param($ConfigPath,$Mode,$ReceiptPath,$ExpectedCommit,$SourceRepository)
[ordered]@{config=$ConfigPath;mode=$Mode;receipt=$ReceiptPath;expected=$ExpectedCommit;source=$SourceRepository;plan=[bool]$WhatIfPreference;helper=$PSCommandPath} | ConvertTo-Json -Compress
'@
function Commit-Fixture($Fixture) {
    $null=Invoke-FixtureGit $Fixture.repo @('add','.')
    $null=Invoke-FixtureGit $Fixture.repo @('commit','-qm','fixture only')
    $Fixture.commit=Invoke-FixtureGit $Fixture.repo @('rev-parse','HEAD')
}
function New-Fixture {
    $root=Join-Path $OutputRoot ([guid]::NewGuid().ToString('N'))
    $repo=Join-Path $root 'feature checkout with spaces'; New-Item -ItemType Directory -Path $repo -Force | Out-Null
    $null=Invoke-FixtureGit $repo @('init','-q','-b','feat/entry-source')
    $null=Invoke-FixtureGit $repo @('config','core.autocrlf','false')
    $null=Invoke-FixtureGit $repo @('config','core.longpaths','true')
    $null=Invoke-FixtureGit $repo @('config','user.name','STRE fixture'); $null=Invoke-FixtureGit $repo @('config','user.email','fixture@example.invalid')
    $null=Invoke-FixtureGit $repo @('remote','add','origin','https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced')
    Put (Join-Path $repo '.gitignore') ('build-and-deploy-dev.ps1'+[char]10+'_audit/'+[char]10)
    Put (Join-Path $repo 'Code/common/Runtime/Compatibility.h') 'TargetVersion{1, 7, 104, 0}; RequiredSkseFileVersion{0, 2, 3, 1}; skse64_1_7_104.dll'
    Put (Join-Path $repo 'Code/common/BuildInfo.h.in') ('#define BUILD_COMMIT "runtime17-'+[char]36+'{GIT_COMMIT_LONG}"')
    $helper=Join-Path $repo 'Tools/Scripts/Invoke-StreRuntime17Dev.ps1'
    Put $helper $mockHelper
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Stre.Runtime17Dev.psm1') -Destination (Split-Path -Parent $helper)
    $sub=Join-Path $repo 'Libraries/Fixture'; New-Item -ItemType Directory -Path $sub -Force | Out-Null
    $null=Invoke-FixtureGit $sub @('init','-q')
    $null=Invoke-FixtureGit $sub @('config','user.name','STRE fixture'); $null=Invoke-FixtureGit $sub @('config','user.email','fixture@example.invalid')
    $null=Invoke-FixtureGit $sub @('remote','add','origin','https://github.com/TiltedPhoques/Fixture')
    Put (Join-Path $sub 'fixture.txt') 'submodule'
    $null=Invoke-FixtureGit $sub @('add','.'); $null=Invoke-FixtureGit $sub @('commit','-qm','fixture')
    $sha=Invoke-FixtureGit $sub @('rev-parse','HEAD')
    Put (Join-Path $repo '.gitmodules') ('[submodule "Libraries/Fixture"]{0}path = Libraries/Fixture{0}url = https://github.com/TiltedPhoques/Fixture{0}' -f [Environment]::NewLine)
    $null=Invoke-FixtureGit $repo @('update-index','--add','--cacheinfo',"160000,$sha,Libraries/Fixture")
    $entry=Join-Path $repo 'build-and-deploy-dev.ps1'; [IO.File]::WriteAllBytes($entry,$entryBytes)
    $configPath=Join-Path $repo '_audit/runtime17-dev.local.json'
    $old=Join-Path $root 'old Runtime17 checkout must not be accessed'
    # A local non-directory sentinel; any fallback/probe will fail safely.
    Put $old 'untouched historical-path sentinel'
    $config=@{schema=1; repository=$old; receipts=(Join-Path $root 'receipts'); buildRoot=(Join-Path $root 'build');
        referencePath=(Join-Path $root 'reference.json'); referenceSha256=('a'*64);
        targets=@(@{id='A';stagingRoot='\\must-not-be-resolved.invalid\stage-a';gameRoot='\\must-not-be-resolved.invalid\game-a'},
                  @{id='B';stagingRoot='\\must-not-be-resolved.invalid\stage-b';gameRoot='\\must-not-be-resolved.invalid\game-b'})}
    Put $configPath ($config | ConvertTo-Json -Depth 10)
    $fixture=@{root=$root;repo=$repo;entry=$entry;helper=$helper;config=$configPath;old=$old;sub=$sub}
    Commit-Fixture $fixture
    $fixture
}
function Run-Entry($Fixture,[string[]]$Parameters=@()) {
    $previous=$ErrorActionPreference; $ErrorActionPreference='Continue'
    try {
        $lines=@(& pwsh -NoProfile -File $Fixture.entry @Parameters 2>&1)
        $code=$LASTEXITCODE
    } finally { $ErrorActionPreference=$previous }
    @{code=$code;output=($lines | Out-String)}
}
function Read-Mock($Result) {
    Check ($Result.code -eq 0) $Result.output
    @($Result.output -split '[\r\n]+' | Where-Object { $_.StartsWith('{') })[-1] | ConvertFrom-Json
}
function Snapshot($Root) {
    # Include Git index/refs too: optional Git index refresh is disabled.
    @(Get-ChildItem -LiteralPath $Root -File -Recurse -Force | Sort-Object FullName | ForEach-Object {
        "$($_.FullName)|$($_.Length)|$((Get-FileHash -LiteralPath $_.FullName).Hash)"
    }) -join [char]10
}
$f=New-Fixture
Case 'actual copied personal entry has byte-identical routing and untouched body' {
    Check ((Get-FileHash -LiteralPath $f.entry).Hash -eq (Get-FileHash -LiteralPath $EntryPath).Hash) 'Entry was simplified before testing'
}
Case 'no arguments select script feature root and print branch full SHA and mode' {
    $r=Run-Entry $f; $m=Read-Mock $r
    Check ($m.source -eq $f.repo -and $m.mode -ceq 'BuildDeploy' -and $m.helper -eq $f.helper) 'Wrong default route/helper'
    Check ($r.output.Contains($f.repo) -and $r.output.Contains('branch=feat/entry-source') -and $r.output.Contains("commit=$($f.commit)") -and $r.output.Contains('mode=BuildDeploy')) 'Missing source display'
}
Case 'absolute invocation from another checkout retains script source' {
    $other=New-Fixture
    Push-Location $other.repo
    try { $m=Read-Mock (Run-Entry $f); Check ($m.source -eq $f.repo) 'Caller cwd selected source' }
    finally { Pop-Location }
}
Case 'historical config repository is never consulted to choose helper' {
    $before=Snapshot $f.root
    $m=Read-Mock (Run-Entry $f)
    Check ($m.source -eq $f.repo -and $m.config -eq $f.config) 'Historical checkout selected'
    Check ((Snapshot $f.root) -ceq $before) 'Entry changed historical sentinel/configuration'
}
Case 'explicit Runtime17TwoPc alias has identical source and arguments' {
    $a=Read-Mock (Run-Entry $f); $b=Read-Mock (Run-Entry $f @('-Runtime17TwoPc'))
    Check (($a | ConvertTo-Json -Compress) -ceq ($b | ConvertTo-Json -Compress)) 'Alias changed routing'
}
Case 'WhatIf forwards inert plan without Legacy or file writes' {
    $before=Snapshot $f.root
    $m=Read-Mock (Run-Entry $f @('-WhatIf'))
    Check ($m.plan -and $m.mode -eq 'BuildDeploy' -and $m.source -eq $f.repo) 'Wrong WhatIf route'
    Check ((Snapshot $f.root) -ceq $before) 'WhatIf changed files'
}
foreach ($mode in @('Verify','RestoreStaging','VerifyRestored')) {
    Case "$mode forwards receipt and never requests a new source/build" {
        $receipt=Join-Path $f.root 'retained receipt'
        $p=@('-Runtime17Mode',$mode,'-Runtime17ReceiptRoot',$receipt,'-Runtime17ConfigPath',$f.config)
        if ($mode -eq 'Verify') { $p+=@('-Runtime17ExpectedCommit',$f.commit) }
        $m=Read-Mock (Run-Entry $f $p)
        Check ($m.mode -ceq $mode -and $m.receipt -eq $receipt -and !$m.source -and $m.config -eq $f.config) 'Resume became a new build'
        if ($mode -eq 'Verify') { Check ($m.expected -ceq $f.commit) 'ExpectedCommit lost' }
    }
}
Case 'contradictory modes are rejected before either helper or Legacy' {
    foreach ($p in @(
        @('-Legacy','-Runtime17TwoPc'), @('-Legacy','-WhatIf'),
        @('-Legacy','-Runtime17ConfigPath',$f.config), @('-Runtime17ReceiptRoot',$f.root),
        @('-Runtime17Mode','Verify'), @('-Runtime17Phase','Deploy'),
        @('-Runtime17TwoPc','-Runtime17RepositoryRoot',$f.repo),
        @('-Runtime17Mode','RestoreStaging','-Runtime17ReceiptRoot',$f.root,'-Runtime17ExpectedCommit',$f.commit)
    )) { $r=Run-Entry $f $p; Check ($r.code -ne 0 -and !$r.output.Contains('"helper"')) 'Contradiction reached helper' }
}
Case 'nonlocal config rejected before filesystem resolution' {
    $r=Run-Entry $f @('-Runtime17ConfigPath','\\must-not-be-resolved.invalid\config.json')
    Check ($r.code -ne 0 -and $r.output.Contains('Select local')) 'UNC config not rejected'
}
Case 'missing helper fails in selected checkout without fallback' {
    $x=New-Fixture; Remove-Item -LiteralPath $x.helper
    $before=Snapshot $x.root; $r=Run-Entry $x
    Check ($r.code -ne 0 -and $r.output.Contains('Selected checkout lacks the Skyrim 1.7 helper')) 'Missing helper silently redirected'
    Check ((Snapshot $x.root) -ceq $before) 'Failure changed files'
}
foreach ($failure in @(
    @{name='terminating helper error';body="throw 'fixture helper failure'"},
    @{name='explicit helper exit';body='exit 27'},
    @{name='native child exit masked by later output';body="& pwsh -NoProfile -Command 'exit 29'; Write-Host 'child returned'"},
    @{name='nonterminating helper error';body="Write-Error 'fixture error' -ErrorAction Continue"}
)) {
    Case "$($failure.name) propagates as nonzero entry exit" {
        $x=New-Fixture; Put $x.helper $failure.body
        $r=Run-Entry $x; Check ($r.code -ne 0) 'Helper/child error reported success'
    }
}
function Use-RealHelper($Fixture) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Invoke-StreRuntime17Dev.ps1') -Destination $Fixture.helper -Force
    Commit-Fixture $Fixture
}
Case 'actual wrapper and module WhatIf preserve every fixture byte and select script root' {
    $x=New-Fixture; Use-RealHelper $x
    $before=Snapshot $x.root; $r=Run-Entry $x @('-WhatIf','-Runtime17ExpectedCommit',$x.commit)
    Check ($r.code -eq 0 -and $r.output.Contains("commit=$($x.commit)")) $r.output
    Check ((Snapshot $x.root) -ceq $before) 'Real WhatIf wrote files'
}
foreach ($invalid in @(
    @{name='tracked dirty source';edit={param($x) Put (Join-Path $x.repo 'Code/common/BuildInfo.h.in') 'human tracked work'};pattern='Commit or isolate'},
    @{name='untracked dirty source';edit={param($x) Put (Join-Path $x.repo 'human-work.txt') 'human untracked work'};pattern='Commit or isolate'},
    @{name='detached HEAD';edit={param($x) $null=Invoke-FixtureGit $x.repo @('checkout','-q','--detach')};pattern='Wrong source branch'},
    @{name='unsupported branch';edit={param($x) $null=Invoke-FixtureGit $x.repo @('branch','-m','unreviewed/no')};pattern='Wrong source branch'},
    @{name='incorrect origin';edit={param($x) $null=Invoke-FixtureGit $x.repo @('remote','set-url','origin','https://example.invalid/wrong')};pattern='Unexpected source repository origin'},
    @{name='divergent submodule';edit={param($x) $null=Invoke-FixtureGit $x.sub @('commit','--allow-empty','-qm','divergent fixture')};pattern='Commit or isolate|Submodule commit/cleanliness'},
    @{name='old runtime';edit={param($x) Put (Join-Path $x.repo 'Code/common/Runtime/Compatibility.h') 'TargetVersion{1, 6, 1170, 0}'; Commit-Fixture $x};pattern='Wrong runtime/build identity'}
)) {
    Case "actual entry/wrapper refuse $($invalid.name) before build with bytes preserved" {
        $x=New-Fixture; Use-RealHelper $x; & $invalid.edit $x
        $before=Snapshot $x.root; $r=Run-Entry $x
        Check ($r.code -ne 0 -and $r.output -match $invalid.pattern) $r.output
        Check ((Snapshot $x.root) -ceq $before) 'Refusal changed source/configuration'
    }
}
Case 'actual entry preserves wrong ExpectedCommit rejection' {
    $x=New-Fixture; Use-RealHelper $x
    $r=Run-Entry $x @('-Runtime17ExpectedCommit',('0'*40))
    Check ($r.code -ne 0 -and $r.output.Contains('Source commit mismatch')) $r.output
}
@{utc=[datetime]::UtcNow;scope='LOCAL FIXTURES ONLY; actual entry copied byte-for-byte; inert helpers and local Git/pwsh only';
    entrySha256=(Get-FileHash -LiteralPath $EntryPath).Hash;count=$results.Count;cases=$results
} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $OutputRoot 'results.json') -Encoding utf8
Write-Host "$($results.Count) entry cases PASS. Evidence: $OutputRoot"
