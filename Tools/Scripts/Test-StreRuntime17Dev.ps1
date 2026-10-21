#requires -Version 7.2
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Stre.Runtime17Dev.psm1') -Force
$module = Get-Module Stre.Runtime17Dev
# All destinations and Git repositories below are newly created local fixtures.
# No configured personal destination, process guard, xmake or game is invoked.
& $module {
    param($OutputRoot)
    Assert-R17Local $OutputRoot
    Assert-R17Ordinary $OutputRoot -AllowMissing
    if (Test-Path -LiteralPath $OutputRoot) { throw 'Select a fresh test output directory.' }
    New-Item -ItemType Directory -Path $OutputRoot | Out-Null
    $script:Results = [Collections.Generic.List[object]]::new()
    function Check($Condition, $Message) { if (!$Condition) { throw $Message } }
    function Case([string]$Name, [scriptblock]$Body) {
        & $Body
        $script:Results.Add(@{case=$Name; result='PASS'})
        Write-Host "PASS $Name"
    }
    function Reject([scriptblock]$Body, [string]$Pattern) {
        try { & $Body | Out-Null } catch {
            if ($_.Exception.Message -notmatch $Pattern) { throw "Unexpected rejection, expected '$Pattern': $($_.Exception.Message)" }
            return
        }
        throw "Expected rejection: $Pattern"
    }
    function Put([string]$Path, [string]$Text) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
        [IO.File]::WriteAllText($Path,$Text)
    }
    function Invoke-FixtureGit([string]$Repo, [string[]]$ArgsList) { $null=Invoke-R17Git $Repo $ArgsList }
    function New-Fixture {
        $root = Join-Path $OutputRoot ([guid]::NewGuid().ToString('N'))
        $repo = Join-Path $root 'repo'; New-Item -ItemType Directory -Path $repo -Force | Out-Null
        Invoke-FixtureGit $repo @('init','-q','-b','feat/skyrim-1.7-runtime')
        Invoke-FixtureGit $repo @('config','core.autocrlf','false')
        Invoke-FixtureGit $repo @('config','user.name','STRE local fixture'); Invoke-FixtureGit $repo @('config','user.email','fixture@example.invalid')
        Invoke-FixtureGit $repo @('remote','add','origin','https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced')
        Put (Join-Path $repo 'Code/common/Runtime/Compatibility.h') 'TargetVersion{1, 7, 104, 0}; RequiredSkseFileVersion{0, 2, 3, 1}; skse64_1_7_104.dll'
        Put (Join-Path $repo 'Code/common/BuildInfo.h.in') '#define BUILD_COMMIT "runtime17-${GIT_COMMIT_LONG}"'
        Put (Join-Path $repo 'GameFiles/Skyrim/STRE_AlternateStart.esp') 'repository content'
        $sub = Join-Path $repo 'Libraries/Fixture'; New-Item -ItemType Directory -Path $sub -Force | Out-Null
        Invoke-FixtureGit $sub @('init','-q'); Invoke-FixtureGit $sub @('config','user.name','STRE local fixture'); Invoke-FixtureGit $sub @('config','user.email','fixture@example.invalid')
        Invoke-FixtureGit $sub @('remote','add','origin','https://github.com/TiltedPhoques/Fixture')
        Put (Join-Path $sub 'fixture.txt') 'submodule'; Invoke-FixtureGit $sub @('add','.'); Invoke-FixtureGit $sub @('commit','-qm','fixture')
        $sha = (Invoke-R17Git $sub @('rev-parse','HEAD')) -join ''
        Put (Join-Path $repo '.gitmodules') "[submodule `"Libraries/Fixture`"]`npath = Libraries/Fixture`nurl = https://github.com/TiltedPhoques/Fixture`n"
        Invoke-FixtureGit $repo @('add','Code','GameFiles','.gitmodules')
        Invoke-FixtureGit $repo @('update-index','--add','--cacheinfo',"160000,$sha,Libraries/Fixture")
        Invoke-FixtureGit $repo @('commit','-qm','fixture')
        $commit = (Invoke-R17Git $repo @('rev-parse','HEAD')) -join ''
        $targets = @(foreach ($id in @('A','B')) {
            $staging = Join-Path $root "$id/staging"; $game = Join-Path $root "$id/game"
            foreach ($base in @($staging,(Join-Path $game 'Data'))) {
                Put (Join-Path $base 'STRE_AlternateStart.esp') 'accepted installed content'
                Put (Join-Path $base 'SkyrimTogetherReborn/SkyrimTogether.exe') 'old native bytes'
                Put (Join-Path $base 'SkyrimTogetherReborn/UI/old.js') 'old UI'
                Put (Join-Path $base 'SkyrimTogetherReborn/STServer.ini') 'sRequiredNativePlugins=BetterGrabbing.dll'
            }
            Put (Join-Path $game 'SkyrimSE.exe') 'fixture Skyrim (never executed)'
            Put (Join-Path $game 'skse64_1_7_104.dll') 'fixture SKSE'
            Put (Join-Path $game 'Data/SKSE/Plugins/BetterGrabbing.dll') 'fixture BG'
            Put (Join-Path $game 'Data/SKSE/Plugins/versionlib-1-7-104-0.bin') 'fixture AL'
            @{id=$id; stagingRoot=$staging; gameRoot=$game}
        })
        $content = Join-Path $targets[0].stagingRoot 'STRE_AlternateStart.esp'
        $sourceContent = Join-Path $repo 'GameFiles/Skyrim/STRE_AlternateStart.esp'
        $reference = @{schema=1; runtime='1.7.104.0'; compiledCommit='0'*40;
            contentFiles=@(@{path='STRE_AlternateStart.esp'; bytes=(Get-Item $content).Length; sha256=Get-R17Hash $content; repositoryBytes=(Get-Item $sourceContent).Length; repositorySha256=Get-R17Hash $sourceContent});
            dependencies=@(foreach ($rel in @('SkyrimSE.exe','skse64_1_7_104.dll','Data/SKSE/Plugins/BetterGrabbing.dll','Data/SKSE/Plugins/versionlib-1-7-104-0.bin')) {
                $path=Join-Path $targets[0].gameRoot $rel
                @{path=$rel; bytes=(Get-Item $path).Length; sha256=Get-R17Hash $path}
            }); nativeFiles=@(Get-R17Rows (Join-Path $targets[0].stagingRoot 'SkyrimTogetherReborn'))}
        $refPath=Join-Path $root 'reference.json'; Write-R17Json $reference $refPath
        $receipts=Join-Path $root 'receipts'; New-Item -ItemType Directory -Path $receipts | Out-Null
        $config=@{schema=1; repository=$repo; receipts=$receipts; referencePath=$refPath; referenceSha256=Get-R17Hash $refPath; targets=$targets}
        $configPath=Join-Path $root 'config.json'; Write-R17Json $config $configPath
        @{root=$root; repo=$repo; commit=$commit; config=$config; configPath=$configPath; reference=$reference}
    }
    function New-FakeBuild($Repo,$Run,$Jobs) {
        $head=(Invoke-R17Git $Repo @('rev-parse','HEAD')) -join ''
        Put (Join-Path $Run 'build/BuildInfo.h') "runtime17-$head"
        foreach ($name in $script:Required) { Put (Join-Path $Run "distrib/bin/$name") "fixture runtime17-$head $name" }
        Put (Join-Path $Run 'distrib/bin/UI/new.js') 'new UI bundle'
        Put (Join-Path $Run 'distrib/bin/TPTests.exe') 'excluded test artifact'
    }
    $f=New-Fixture
    Case 'source branch, HEAD, runtime and submodule provenance' {
        $source=Get-R17Source $f.repo $f.commit
        Check ($source.submodules.Count -eq 1) 'Missing submodule evidence'
    }
    Case 'wrong branch rejected' {
        Invoke-FixtureGit $f.repo @('branch','-m','main')
        try { Reject { Get-R17Source $f.repo } 'Wrong source branch' } finally { Invoke-FixtureGit $f.repo @('branch','-m','feat/skyrim-1.7-runtime') }
    }
    Case 'wrong expected commit rejected' { Reject { Get-R17Source $f.repo ('0'*40) } 'Source commit mismatch' }
    Case 'dirty source rejected' {
        Put (Join-Path $f.repo 'untracked.cpp') 'uncommitted'
        try { Reject { Get-R17Source $f.repo } 'Commit or isolate' } finally { Remove-Item -LiteralPath (Join-Path $f.repo 'untracked.cpp') }
    }
    Case 'submodule origin mismatch rejected' {
        $sub=Join-Path $f.repo 'Libraries/Fixture'; Invoke-FixtureGit $sub @('remote','set-url','origin','https://example.invalid/wrong')
        try { Reject { Get-R17Source $f.repo } 'Submodule origin' } finally { Invoke-FixtureGit $sub @('remote','set-url','origin','https://github.com/TiltedPhoques/Fixture') }
    }
    Case '1.6 runtime contract rejected even on correct branch' {
        $oldRuntime=New-Fixture
        Put (Join-Path $oldRuntime.repo 'Code/common/Runtime/Compatibility.h') 'TargetVersion{1, 6, 1170, 0}; skse64_1_6_1170.dll'
        Invoke-FixtureGit $oldRuntime.repo @('add','Code'); Invoke-FixtureGit $oldRuntime.repo @('commit','-qm','wrong runtime fixture')
        Reject { Get-R17Source $oldRuntime.repo } 'Wrong runtime/build identity'
    }
    Case 'accepted source/installed content divergence preserved' {
        $ref=Get-R17Reference $f.config
        $null=Get-R17Preflight $f.config $ref
    }
    Case 'missing actual game path rejected before any destination IO' {
        $bad=Read-R17Json $f.configPath; $bad.targets[1].gameRoot=''
        Reject { Get-R17Preflight $bad $f.reference } 'Configure the actual gameRoot'
    }
    Case 'unavailable secondary destination rejected before any write' {
        $bad=Read-R17Json $f.configPath; $bad.targets[1].stagingRoot=Join-Path $f.root 'missing'
        Reject { Get-R17Preflight $bad $f.reference } 'Missing or inaccessible'
        Assert-R17Files (Join-Path $f.config.targets[0].stagingRoot 'SkyrimTogetherReborn') $f.reference.nativeFiles
    }
    Case 'aliased destinations rejected' {
        $bad=Read-R17Json $f.configPath; $bad.targets[1].stagingRoot=$bad.targets[0].stagingRoot
        Reject { Get-R17Preflight $bad $f.reference } 'Overlapping/aliased'
    }
    Case 'CK content divergence rejected' {
        $path=Join-Path $f.config.targets[1].stagingRoot 'STRE_AlternateStart.esp'; Put $path 'unexpected CK'
        try { Reject { Get-R17Preflight $f.config $f.reference } 'Hash/size mismatch' } finally { Put $path 'accepted installed content' }
    }
    Case 'old Better Grabbing dependency rejected' {
        $path=Join-Path $f.config.targets[1].gameRoot 'Data/SKSE/Plugins/BetterGrabbing.dll'; Put $path '1.17'
        try { Reject { Get-R17Preflight $f.config $f.reference } 'Hash/size mismatch' } finally { Put $path 'fixture BG' }
    }
    Case 'manifest traversal, duplicates and protected files rejected' {
        foreach ($rel in @('../SkyrimSE.exe','UI/x:stream','UI/a.psc','BetterGrabbing.dll','SKSE/Plugins/x.dll')) {
            Reject { Assert-R17Rows @(@{path=$rel; bytes=1; sha256='a'*64}) -Native } 'Unsafe|Protected|Unreviewed'
        }
        Reject { Assert-R17Rows @($f.reference.nativeFiles[0],$f.reference.nativeFiles[0]) -Native } 'Duplicate'
    }
    $run=Join-Path $f.root 'candidate'; New-FakeBuild $f.repo $run 1
    $source=Get-R17Source $f.repo
    $candidate=New-R17Candidate $source $run
    Case 'fresh variable-size package; excludes test executables' {
        Check ($candidate.files.Count -eq ($script:Required.Count+1)) 'Incorrect dynamic count'
        Check ($candidate.files.path -notcontains 'TPTests.exe') 'Test binary packaged'
        Assert-R17Exact (Join-Path $run 'distrib/bin') $candidate.files -Distribution
    }
    Case 'stale artifact identity rejected' {
        $path=Join-Path $run 'distrib/bin/STServer.dll'; $old=[IO.File]::ReadAllText($path); Put $path 'runtime17-old'
        try { Reject { New-R17Candidate $source $run } 'Stale binary identity' } finally { Put $path $old }
    }
    Case 'invalid manifest hash rejected' {
        $bad=@(foreach ($row in $candidate.files) { $copy=$row.Clone(); $copy }); $bad[0].sha256='0'*64
        Reject { Assert-R17Exact (Join-Path $run 'distrib/bin') $bad -Distribution } 'Hash/size mismatch'
    }
    Case 'missing required CEF library rejected' {
        $path=Join-Path $run 'distrib/bin/libEGL.dll'; $old=[IO.File]::ReadAllText($path)
        Remove-Item -LiteralPath $path
        try { Reject { New-R17Candidate $source $run } 'Missing required artifact' } finally { Put $path $old }
    }
    Case 'unmanifested extra payload rejected' {
        Put (Join-Path $run 'distrib/bin/UI/unexpected.js') 'extra'
        try { Reject { Assert-R17Exact (Join-Path $run 'distrib/bin') $candidate.files -Distribution } 'file set mismatch' } finally { Remove-Item -LiteralPath (Join-Path $run 'distrib/bin/UI/unexpected.js') }
    }
    Case 'staging breaks hardlinks and leaves actual installation untouched' {
        $stage=Join-Path $f.config.targets[0].stagingRoot 'SkyrimTogetherReborn'
        $installed=Join-Path $f.config.targets[0].gameRoot 'Data/SkyrimTogetherReborn'
        Remove-Item -LiteralPath (Join-Path $stage 'SkyrimTogether.exe')
        New-Item -ItemType HardLink -Path (Join-Path $stage 'SkyrimTogether.exe') -Target (Join-Path $installed 'SkyrimTogether.exe') | Out-Null
        Set-R17Payload $stage (Join-Path $run 'distrib/bin') $f.reference.nativeFiles $candidate.files
        Assert-R17Exact $installed $f.reference.nativeFiles
        Check (!(Test-Path -LiteralPath (Join-Path $stage 'UI/old.js'))) 'Old UI was retained'
    }
    $script:RealBuild=(Get-Item Function:Invoke-R17Build).ScriptBlock
    Case 'xmake routing uses selected worktree and fresh output paths' {
        $script:Calls=@()
        function script:xmake { $script:Calls += ,@($args); Check ((Get-Location).Path -eq $f.repo) 'Wrong build working directory'; $global:LASTEXITCODE=0 }
        try {
            & $script:RealBuild $f.repo $run 3
            Check ($script:Calls.Count -eq 3) 'Wrong xmake invocation count'
            foreach ($call in $script:Calls) { Check ($call[1] -eq '-P' -and $call[2] -eq '.') 'Missing explicit -P .' }
            Check ($script:Calls[0][-1] -eq (Join-Path $run 'build')) 'Wrong build path'
            Check ($script:Calls[2][-1] -eq (Join-Path $run 'distrib')) 'Wrong install path'
            function script:xmake { $global:LASTEXITCODE=42 }
            Reject { & $script:RealBuild $f.repo $run 3 } 'config failed: 42'
        } finally { Remove-Item Function:xmake }
    }
    # Integration uses real filesystem/hash/backup/restore/Git checks. Only the
    # compiler, local process enumeration and human handoff are simulated.
    function script:Invoke-R17Build { param($Repo,$Run,$Jobs) New-FakeBuild $Repo $Run $Jobs }
    function script:Assert-R17Stopped { }
    $script:FailSecond=$false
    $script:RealSet=(Get-Item Function:Set-R17Payload).ScriptBlock
    function script:Set-R17Payload {
        param($Destination,$Source,$Before,$After)
        if ($script:FailSecond -and $Destination -eq (Join-Path $script:Active.config.targets[1].stagingRoot 'SkyrimTogetherReborn')) {
            # A real partial second-copy state, after A already succeeded.
            Remove-Item -LiteralPath (Join-Path $Destination 'SkyrimTogether.exe')
            Remove-Item -LiteralPath (Join-Path $Destination 'UI/old.js')
            throw 'Injected secondary copy failure'
        }
        & $script:RealSet $Destination $Source $Before $After
    }
    $script:DoDeploy=$true
    function script:Read-Host {
        param($Prompt)
        if ($Prompt -like '*Type READY') { return 'READY' }
        if (!$script:DoDeploy) { return 'LATER' }
        foreach ($target in $script:Active.config.targets) {
            $stage=Join-Path $target.stagingRoot 'SkyrimTogetherReborn'; $installed=Join-Path $target.gameRoot 'Data/SkyrimTogetherReborn'
            # Simulate the human Vortex handoff only inside this fixture.
            $rows=@(Get-R17Rows $stage)
            foreach ($old in Get-R17Rows $installed) { Remove-Item -LiteralPath (Get-R17Child $installed $old.path) }
            foreach ($row in $rows) {
                $dest=Get-R17Child $installed $row.path
                New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
                Copy-Item -LiteralPath (Get-R17Child $stage $row.path) -Destination $dest
            }
            Assert-R17Exact $installed $rows
        }
        return 'DEPLOYED'
    }
    Case 'end-to-end simulated A/B deploy verifies actual Data and protects content' {
        $script:Active=New-Fixture
        Invoke-R17Workflow -ConfigPath $script:Active.configPath
        $latest=@(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory)[0].FullName
        $receipt=Read-R17Json (Join-Path $latest 'receipt.json')
        Check ($receipt.status -eq 'INSTALLED_VERIFIED') 'False final status'
        Check ((Read-R17Json (Join-Path $latest 'verified-B.json')).status -eq 'INSTALLED_VERIFIED') 'B receipt missing'
        Assert-R17Content (Join-Path $script:Active.repo 'GameFiles/Skyrim') $script:Active.reference.contentFiles -Repository
    }
    Case 'later documentation HEAD does not relabel the compiled candidate' {
        $latest=@(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory)[0].FullName
        $before=Read-R17Json (Join-Path $latest 'receipt.json')
        Put (Join-Path $script:Active.repo 'README.md') 'later documentation only'
        Invoke-FixtureGit $script:Active.repo @('add','README.md'); Invoke-FixtureGit $script:Active.repo @('commit','-qm','documentation fixture')
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode Verify -ReceiptPath $latest
        $verified=Read-R17Json (Join-Path $latest 'verified-A.json')
        Check ($verified.candidateCommit -eq $before.candidate.source.commit -and $verified.verificationHead -ne $verified.candidateCommit) 'Compiled commit was relabelled'
    }
    Case 'staging alone cannot be declared installed; Verify resumes same candidate' {
        $script:Active=New-Fixture; $script:DoDeploy=$false
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath } 'Staged only'
        $latest=@(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory)[0].FullName
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode Verify -ReceiptPath $latest } 'Hash/size mismatch|Missing or inaccessible'
        $script:DoDeploy=$true; $null=Read-Host 'fixture human deploy'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode Verify -ReceiptPath $latest
    }
    Case 'partial secondary failure stops; both backups restore; later human redeploy verified' {
        $script:Active=New-Fixture; $script:FailSecond=$true
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath } 'Injected secondary copy failure'
        $script:FailSecond=$false
        $latest=@(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory)[0].FullName
        $receipt=Read-R17Json (Join-Path $latest 'receipt.json'); Check ($receipt.status -eq 'STOPPED') 'Missing failure receipt'
        Check (!(Test-Path -LiteralPath (Join-Path $latest 'staged-B.json'))) 'False B success'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest
        $null=Read-Host 'fixture human deploy'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode VerifyRestored -ReceiptPath $latest
        Check ((Read-R17Json (Join-Path $latest 'receipt.json')).status -eq 'RESTORED_VERIFIED') 'Missing restoration verification'
    }
    Case 'restore refuses unrelated intervening staging bytes before writes' {
        $latest=@(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory)[0].FullName
        $target=$script:Active.config.targets[1]
        $path=Join-Path $target.stagingRoot 'SkyrimTogetherReborn/SkyrimTogether.exe'
        $old=[IO.File]::ReadAllText($path); Put $path 'unrelated user change'
        try { Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest } 'unrelated changes' } finally { Put $path $old }
    }
    Case 'restore refuses corrupted backup before writing either PC' {
        $latest=@(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory)[0].FullName
        $path=Join-Path $latest 'backup/B/staging/SkyrimTogether.exe'
        $old=[IO.File]::ReadAllText($path); Put $path 'corrupt backup'
        try { Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest } 'Hash/size mismatch' } finally { Put $path $old }
        foreach ($target in $script:Active.config.targets) { Assert-R17Exact (Join-Path $target.stagingRoot 'SkyrimTogetherReborn') $script:Active.reference.nativeFiles }
    }
    Case 'WhatIf cannot touch destinations, enumerate processes or build' {
        $plan=Read-R17Json $f.configPath
        # Deliberately invalid UNC strings. They are displayed, never resolved.
        $plan.targets[0].gameRoot='\\must-not-be-resolved.invalid\game'
        $plan.targets[1].gameRoot=''
        $plan.targets[1].stagingRoot='\\must-not-be-resolved.invalid\stage'
        $path=Join-Path $f.root 'plan.json'; Write-R17Json $plan $path
        function script:Get-R17Preflight { throw 'Unexpected destination IO' }
        function script:Assert-R17Stopped { throw 'Unexpected process enumeration' }
        function script:Invoke-R17Build { throw 'Unexpected compilation' }
        $WhatIfPreference=$true
        try { $result=Invoke-R17Workflow -ConfigPath $path -PlanOnly } finally { $WhatIfPreference=$false }
        Check (!$result.destinationIO -and $result.missingGameRoots -contains 'B') 'Plan is not inert'
    }
    Write-R17Json @{utc=[datetime]::UtcNow; scope='LOCAL FIXTURES ONLY; no native compilation or real deployment'; cases=$script:Results; count=$script:Results.Count} (Join-Path $OutputRoot 'results.json')
    Write-Host "$($script:Results.Count) local cases PASS. Evidence: $OutputRoot"
} $OutputRoot
