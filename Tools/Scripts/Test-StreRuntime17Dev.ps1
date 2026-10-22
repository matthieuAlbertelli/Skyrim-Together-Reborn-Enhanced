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
    $script:BuildFixtureRoot=Join-Path ([IO.Path]::GetTempPath()) ('r17 tests '+[guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $script:BuildFixtureRoot | Out-Null
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
    $script:Wave='Sound/Voice/STRE_AlternateStart.esp/MaleWarlock/stre_quest_stre_dial_valen_000d6b19_1.wav'
    function New-Fixture {
        $root = Join-Path $OutputRoot ([guid]::NewGuid().ToString('N'))
        $repo = Join-Path $root 'repo'; New-Item -ItemType Directory -Path $repo -Force | Out-Null
        Invoke-FixtureGit $repo @('init','-q','-b','feat/skyrim-1.7-runtime')
        Invoke-FixtureGit $repo @('config','core.autocrlf','false')
        Invoke-FixtureGit $repo @('config','core.longpaths','true')
        Invoke-FixtureGit $repo @('config','user.name','STRE local fixture'); Invoke-FixtureGit $repo @('config','user.email','fixture@example.invalid')
        Invoke-FixtureGit $repo @('remote','add','origin','https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced')
        Put (Join-Path $repo 'Code/common/Runtime/Compatibility.h') 'TargetVersion{1, 7, 104, 0}; RequiredSkseFileVersion{0, 2, 3, 1}; skse64_1_7_104.dll'
        Put (Join-Path $repo 'Code/common/BuildInfo.h.in') '#define BUILD_COMMIT "runtime17-${GIT_COMMIT_LONG}"'
        Put (Join-Path $repo 'GameFiles/Skyrim/STRE_AlternateStart.esp') 'repository content'
        Put (Join-Path $repo "GameFiles/Skyrim/$script:Wave") 'Git WAV bytes'
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
                Put (Join-Path $base $script:Wave) "original $id $([IO.Path]::GetFileName($base)) WAV"
                foreach ($extra in @('Saves/player.ess','Saves/player.skse','SkyrimTogetherReborn/campaigns/state.db','SkyrimTogetherReborn/identities/local.json','SkyrimTogetherReborn/logs/run.log','SkyrimTogetherReborn/cache/blob','unmanaged.txt')) { Put (Join-Path $base $extra) "private $id $base $extra" }
                Put (Join-Path $base 'STRE_AlternateStart.esp') 'accepted installed content'
                Put (Join-Path $base 'SkyrimTogetherReborn/SkyrimTogether.exe') 'old native bytes'
                Put (Join-Path $base 'SkyrimTogetherReborn/UI/old.js') 'old UI'
                Put (Join-Path $base 'SkyrimTogetherReborn/config/STServer.ini') "[ModPolicy]`nsRequiredNativePlugins=BetterGrabbing.dll"
            }
            Put (Join-Path $game 'SkyrimSE.exe') 'fixture Skyrim (never executed)'
            Put (Join-Path $game 'skse64_1_7_104.dll') 'fixture SKSE'
            Put (Join-Path $game 'Data/SKSE/Plugins/BetterGrabbing.dll') 'fixture BG'
            Put (Join-Path $game 'Data/SKSE/Plugins/versionlib-1-7-104-0.bin') 'fixture AL'
            @{id=$id; stagingRoot=$staging; gameRoot=$game}
        })
        $reference = @{schema=1; runtime='1.7.104.0'; compiledCommit='0'*40;
            contentFiles=@(foreach ($rel in @('STRE_AlternateStart.esp',$script:Wave)) {
                $content=Join-Path $targets[0].gameRoot "Data/$rel"
                $sourceContent=Join-Path $repo "GameFiles/Skyrim/$rel"
                @{path=$rel; bytes=(Get-Item $content).Length; sha256=Get-R17Hash $content; repositoryBytes=(Get-Item $sourceContent).Length; repositorySha256=Get-R17Hash $sourceContent}
            });
            dependencies=@(foreach ($rel in @('SkyrimSE.exe','skse64_1_7_104.dll','Data/SKSE/Plugins/BetterGrabbing.dll','Data/SKSE/Plugins/versionlib-1-7-104-0.bin')) {
                $path=Join-Path $targets[0].gameRoot $rel
                @{path=$rel; bytes=(Get-Item $path).Length; sha256=Get-R17Hash $path}
            }); nativeFiles=@(Get-R17Rows (Join-Path $targets[0].stagingRoot 'SkyrimTogetherReborn'))}
        $refPath=Join-Path $root 'reference.json'; Write-R17Json $reference $refPath
        $receipts=Join-Path $root 'receipts'; New-Item -ItemType Directory -Path $receipts | Out-Null
        $buildRoot=Join-Path $script:BuildFixtureRoot ([guid]::NewGuid().ToString('N').Substring(0,8))
        New-Item -ItemType Directory -Path $buildRoot | Out-Null
        $config=@{schema=1; repository=$repo; receipts=$receipts; buildRoot=$buildRoot; referencePath=$refPath; referenceSha256=Get-R17Hash $refPath; targets=$targets}
        $configPath=Join-Path $root 'config.json'; Write-R17Json $config $configPath
        @{root=$root; repo=$repo; commit=$commit; config=$config; configPath=$configPath; reference=$reference}
    }
    function New-FakeBuild($Repo,$Run,$Jobs) {
        $head=(Invoke-R17Git $Repo @('rev-parse','HEAD')) -join ''
        Put (Join-Path (Get-R17BuildDirectory $Run) 'BuildInfo.h') "runtime17-$head"
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
        $null=Get-R17Preflight $f.config $ref $ref.contentFiles
    }
    Case 'canonical INI under config passes with no root INI' {
        $state=@(Get-R17ProtectedState $f.config $f.reference)
        Check ($state.Count -eq 4) 'Missing protected configuration entries'
        foreach ($row in $state) {
            Check ($row.presence -eq 'present' -and $row.path -like '*\config\STServer.ini') 'Wrong canonical INI state'
            Check (!(Test-Path -LiteralPath (Join-Path (Split-Path -Parent (Split-Path -Parent $row.path)) 'STServer.ini'))) 'Fixture unexpectedly has a root INI'
        }
    }
    Case 'installed configuration alone passes and absent staging is recorded' {
        $stageIni=Join-Path $f.config.targets[0].stagingRoot 'SkyrimTogetherReborn/config/STServer.ini'
        $old=[IO.File]::ReadAllBytes($stageIni); Remove-Item -LiteralPath $stageIni
        try {
            $state=(Get-R17Preflight $f.config $f.reference $f.reference.contentFiles).protected
            $absent=@($state | Where-Object presence -eq 'absent')
            Check ($absent.Count -eq 1 -and $absent[0].path -eq $stageIni -and $null -eq $absent[0].sha256) 'Staging absence was not recorded'
            Assert-R17Protected $f.config $f.reference $state
            [IO.File]::WriteAllBytes($stageIni,$old)
            Reject { Assert-R17Protected $f.config $f.reference $state } 'presence/hash changed'
        } finally { [IO.File]::WriteAllBytes($stageIni,$old) }
    }
    Case 'initially present staging configuration cannot disappear' {
        $state=@(Get-R17ProtectedState $f.config $f.reference)
        $path=Join-Path $f.config.targets[0].stagingRoot 'SkyrimTogetherReborn/config/STServer.ini'
        $old=[IO.File]::ReadAllBytes($path); Remove-Item -LiteralPath $path
        try { Reject { Assert-R17Protected $f.config $f.reference $state } 'presence/hash changed' } finally { [IO.File]::WriteAllBytes($path,$old) }
    }
    Case 'missing installed INI remains blocking despite a staged copy' {
        $path=Join-Path $f.config.targets[0].gameRoot 'Data/SkyrimTogetherReborn/config/STServer.ini'
        $old=[IO.File]::ReadAllBytes($path); Remove-Item -LiteralPath $path
        try { Reject { Get-R17Preflight $f.config $f.reference $f.reference.contentFiles } 'Required installed server configuration is absent' } finally { [IO.File]::WriteAllBytes($path,$old) }
    }
    Case 'Better Grabbing policy and configuration hashes remain protected' {
        $path=Join-Path $f.config.targets[0].gameRoot 'Data/SkyrimTogetherReborn/config/STServer.ini'
        $old=[IO.File]::ReadAllBytes($path); $state=@(Get-R17ProtectedState $f.config $f.reference)
        try {
            Put $path "[ModPolicy]`nsRequiredNativePlugins="
            Reject { Get-R17ProtectedState $f.config $f.reference } 'Required native plugin policy mismatch'
            Put $path "[ModPolicy]`nsRequiredNativePlugins=BetterGrabbing.dll`n; changed configuration"
            Reject { Assert-R17Protected $f.config $f.reference $state } 'presence/hash changed'
        } finally { [IO.File]::WriteAllBytes($path,$old) }
    }
    Case 'optional INI absence differs from denied metadata or read access' {
        $missing=Join-Path $f.root 'missing-config/STServer.ini'
        Check ((Get-R17IniState $missing).presence -eq 'absent') 'Missing optional INI should be recorded'
        $badParent=Split-Path -Parent $missing; Put $badParent 'not a directory'
        try { Reject { Get-R17IniState $missing } 'ancestor is not a directory' } finally { Remove-Item -LiteralPath $badParent }
        $script:RealIniAttributes=(Get-Item Function:Get-R17IniAttributes).ScriptBlock
        $script:RealIniBytes=(Get-Item Function:Read-R17IniBytes).ScriptBlock
        $path=Join-Path $f.config.targets[0].stagingRoot 'SkyrimTogetherReborn/config/STServer.ini'
        try {
            # Deterministic OS exception injection, without changing any ACL.
            function script:Get-R17IniAttributes { param($Path) throw [UnauthorizedAccessException]::new('fixture access denial') }
            Reject { Get-R17IniState $missing } 'Access denied inspecting server configuration'
            Set-Item Function:script:Get-R17IniAttributes $script:RealIniAttributes
            function script:Read-R17IniBytes { param($Path) throw [UnauthorizedAccessException]::new('fixture access denial') }
            Reject { Get-R17IniState $path } 'Access denied reading server configuration'
            function script:Read-R17IniBytes { param($Path) throw [IO.IOException]::new('fixture sharing violation') }
            Reject { Get-R17IniState $path } 'IO failure reading server configuration'
        } finally {
            Set-Item Function:script:Get-R17IniAttributes $script:RealIniAttributes
            Set-Item Function:script:Read-R17IniBytes $script:RealIniBytes
        }
    }
    Case 'missing actual game path rejected before any destination IO' {
        $bad=Read-R17Json $f.configPath; $bad.targets[1].gameRoot=''
        Reject { Get-R17Preflight $bad $f.reference $f.reference.contentFiles } 'Configure the actual gameRoot'
    }
    Case 'unavailable secondary destination rejected before any write' {
        $bad=Read-R17Json $f.configPath; $bad.targets[1].stagingRoot=Join-Path $f.root 'missing'
        Reject { Get-R17Preflight $bad $f.reference $f.reference.contentFiles } 'Missing or inaccessible'
        Assert-R17Files (Join-Path $f.config.targets[0].stagingRoot 'SkyrimTogetherReborn') $f.reference.nativeFiles
    }
    Case 'aliased destinations rejected' {
        $bad=Read-R17Json $f.configPath; $bad.targets[1].stagingRoot=$bad.targets[0].stagingRoot
        Reject { Get-R17Preflight $bad $f.reference $f.reference.contentFiles } 'Overlapping/aliased'
    }
    Case 'initial content divergence plans replacement instead of rejection' {
        $path=Join-Path $f.config.targets[1].stagingRoot 'STRE_AlternateStart.esp'; Put $path 'unexpected CK'
        try { $plan=Get-R17Preflight $f.config $f.reference $f.reference.contentFiles; Check (($plan.targets[1].stagingBefore | Where-Object path -eq 'STRE_AlternateStart.esp').action -eq 'replace') 'Divergent content not scheduled for replacement' } finally { Put $path 'accepted installed content' }
    }
    Case 'old Better Grabbing dependency rejected' {
        $path=Join-Path $f.config.targets[1].gameRoot 'Data/SKSE/Plugins/BetterGrabbing.dll'; Put $path '1.17'
        try { Reject { Get-R17Preflight $f.config $f.reference $f.reference.contentFiles } 'Hash/size mismatch' } finally { Put $path 'fixture BG' }
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
    Case 'candidate captures installed A WAV independently without changing Git or A' {
        $installedWave=Join-Path $f.config.targets[0].gameRoot "Data/$script:Wave"
        $before=Get-R17Hash $installedWave
        $candidate=Add-R17InstalledContent $candidate $f.config $f.reference $run
        $wave=@($candidate.contentFiles | Where-Object path -eq $script:Wave)[0]
        Check ($wave.sha256 -eq $before -and $wave.provenance -eq 'installed main PC' -and $wave.sourcePath -eq $installedWave) 'Wrong WAV provenance'
        Check ($before -ne (Get-R17Hash (Join-Path $f.repo "GameFiles/Skyrim/$script:Wave"))) 'Fixture does not differ from Git'
        Check ((Get-R17Hash $installedWave) -eq $before) 'Source was overwritten'
        Assert-R17Content (Join-Path $f.repo 'GameFiles/Skyrim') $f.reference.contentFiles -Repository
        $saved=[IO.File]::ReadAllBytes($installedWave)
        try {
            Put $installedWave 'later source edit'
            Check ((Get-R17Hash (Join-Path $run "package/$script:Wave")) -eq $before) 'Snapshot is linked to source'
        } finally { [IO.File]::WriteAllBytes($installedWave,$saved) }
    }
    Case 'preflight plans replace/create/identical; both staging WAVs use one candidate' {
        $rows=@(Get-R17DeploymentRows $candidate)
        $plan=Get-R17Preflight $f.config $f.reference $rows
        foreach ($target in $plan.targets) {
            Check (($target.stagingBefore | Where-Object path -eq $script:Wave).action -eq 'replace') 'Divergent staging WAV rejected or missed'
            Check (($target.stagingBefore | Where-Object path -eq 'SkyrimTogetherReborn/libcef.dll').action -eq 'create') 'Absent native file not planned'
            Check (($target.stagingBefore | Where-Object path -eq 'STRE_AlternateStart.esp').action -eq 'identical') 'Identical file not recognized'
        }
    }
    Case 'staging breaks native and WAV hardlinks; outside-manifest old UI survives' {
        $stage=$f.config.targets[0].stagingRoot
        $installed=Join-Path $f.config.targets[0].gameRoot 'Data'
        $rows=@(Get-R17DeploymentRows $candidate)
        # Simulate a different installed version after snapshot, with Vortex links.
        Put (Join-Path $installed $script:Wave) 'old linked WAV'
        foreach ($rel in @('SkyrimTogetherReborn/SkyrimTogether.exe',$script:Wave)) {
            Remove-Item -LiteralPath (Join-Path $stage $rel)
            New-Item -ItemType HardLink -Path (Join-Path $stage $rel) -Target (Join-Path $installed $rel) | Out-Null
        }
        $old=@(Get-R17Plan $installed $rows)
        Set-R17Payload $stage (Join-Path $run 'package') @(Get-R17Plan $stage $rows) $rows
        Assert-R17State $installed $old 'hardlink test old Data'
        Check (Test-Path -LiteralPath (Join-Path $stage 'SkyrimTogetherReborn/UI/old.js')) 'Outside-manifest old UI was deleted'
    }
    Case 'missing installed source blocks without Git fallback' {
        $x=New-Fixture; $r=Join-Path $x.root 'capture'; New-FakeBuild $x.repo $r 1
        Remove-Item -LiteralPath (Join-Path $x.config.targets[0].gameRoot "Data/$script:Wave")
        Reject { Add-R17InstalledContent (New-R17Candidate (Get-R17Source $x.repo) $r) $x.config $x.reference $r } 'Required installed main PC source absent'
    }
    Case 'capture detects source changing during the copy interval' {
        $x=New-Fixture; $r=Join-Path $x.root 'capture'; New-FakeBuild $x.repo $r 1
        $script:OriginalCopy=(Get-Item Function:Copy-R17IndependentFile).ScriptBlock
        $script:ChangingSource=Join-Path $x.config.targets[0].gameRoot "Data/$script:Wave"
        function script:Copy-R17IndependentFile {
            param($Source,$Destination)
            & $script:OriginalCopy $Source $Destination
            if ($Source -eq $script:ChangingSource) { Put $Source 'changed during capture' }
        }
        try { Reject { Add-R17InstalledContent (New-R17Candidate (Get-R17Source $x.repo) $r) $x.config $x.reference $r } 'source changed during capture' }
        finally { Set-Item Function:script:Copy-R17IndependentFile $script:OriginalCopy }
    }
    Case 'altered frozen snapshot blocks before staging' {
        $p=Join-Path $run "package/$script:Wave"; $bytes=[IO.File]::ReadAllBytes($p)
        try { Put $p 'corruption'; Reject { Assert-R17Package (Join-Path $run 'package') @(Get-R17DeploymentRows $candidate) } 'frozen candidate integrity.*mismatch' }
        finally { [IO.File]::WriteAllBytes($p,$bytes) }
    }
    Case 'managed absence is distinct from denied access and IO errors' {
        Check ((Get-R17FileState $f.root 'absent.wav').presence -eq 'absent') 'Absence lost'
        $script:OriginalAttributes=(Get-Item Function:Get-R17FileAttributes).ScriptBlock
        try {
            function script:Get-R17FileAttributes { param($Path) throw [UnauthorizedAccessException]::new('fixture') }
            Reject { Get-R17FileState $f.root 'absent.wav' } 'Access denied'
            function script:Get-R17FileAttributes { param($Path) throw [IO.IOException]::new('fixture') }
            Reject { Get-R17FileState $f.root 'absent.wav' } 'IO failure'
        } finally { Set-Item Function:script:Get-R17FileAttributes $script:OriginalAttributes }
    }
    Case 'personal paths and native/content collisions are outside content scope' {
        foreach ($rel in @('Saves/player.ess','SKSE/Plugins/BetterGrabbing.dll','SkyrimTogetherReborn/UI/index.html','SkyrimTogetherReborn/config/STServer.ini','STRE/state.db','../STRE_AlternateStart.esp')) {
            Reject { Assert-R17ContentPath $rel } 'Protected|Unsafe'
        }
    }
    $script:RealBuild=(Get-Item Function:Invoke-R17Build).ScriptBlock
    Case 'short build roots with spaces allocate unique linked outputs and preserve receipts' {
        $r1=Join-Path $f.root 'layout-one';$r2=Join-Path $f.root 'layout-two'
        New-Item -ItemType Directory -Path $r1,$r2 | Out-Null
        $one=New-R17BuildLayout $f.config $r1 $source
        $two=New-R17BuildLayout $f.config $r2 $source
        Check ($one.root -ne $two.root -and $one.buildDirectory.Length -le 100) 'Build directories not short/unique'
        Check ($one.buildDirectory.Contains(' ')) 'Space-containing build root not covered'
        Check ((Get-R17BuildDirectory $r1 $source) -eq $one.buildDirectory) 'Receipt/build linkage lost'
        $badSource=$source.Clone();$badSource.commit='0'*40
        Reject { Get-R17BuildDirectory $r1 $badSource } 'source commit mismatch'
        $link=Read-R17Json (Join-Path $one.root 'receipt-link.json');$link.receiptRoot=$r2
        Write-R17Json $link (Join-Path $one.root 'receipt-link.json')
        Reject { Get-R17BuildDirectory $r1 $source } 'reverse link mismatch'
    }
    Case 'oversized or nonlocal compiler roots are rejected without compiling' {
        $bad=$f.config.Clone();$bad.buildRoot=$f.root
        Reject { New-R17BuildLayout $bad $run $source } 'Short compiler build directory'
        $bad.buildRoot='\\must-not-be-resolved.invalid\build'
        Reject { New-R17BuildLayout $bad $run $source } 'fully qualified local path'
    }
    Case 'xmake routing uses selected worktree and fresh output paths' {
        $layout=New-R17BuildLayout $f.config $run $source
        $script:Calls=@()
        function script:xmake { $script:Calls += ,@($args); Check ((Get-Location).Path -eq $f.repo) 'Wrong build working directory'; $global:LASTEXITCODE=0 }
        try {
            & $script:RealBuild $f.repo $run 3
            Check ($script:Calls.Count -eq 3) 'Wrong xmake invocation count'
            foreach ($call in $script:Calls) { Check ($call[1] -eq '-P' -and $call[2] -eq '.') 'Missing explicit -P .' }
            Check ($script:Calls[0][-1] -eq $layout.buildDirectory) 'Wrong short build path'
            Check ($script:Calls[2][-1] -eq (Join-Path $run 'distrib')) 'Wrong install path'
            function script:xmake { $global:LASTEXITCODE=42 }
            Reject { & $script:RealBuild $f.repo $run 3 } 'config failed: 42'
        } finally { Remove-Item Function:xmake }
    }
    # Integration uses real filesystem/hash/backup/restore/Git checks. Only the
    # compiler, local process enumeration and human handoff are simulated.
    function script:Invoke-R17Build { param($Repo,$Run,$Jobs) New-FakeBuild $Repo $Run $Jobs }
    function script:Assert-R17Stopped { }
    $script:FailSecond=$false; $script:CorruptFinal=$false
    $script:RealCopy=(Get-Item Function:Copy-R17IndependentFile).ScriptBlock
    function script:Copy-R17IndependentFile {
        param($Source,$Destination)
        if ($script:FailSecond -and $Destination -eq (Join-Path $script:Active.config.targets[1].stagingRoot $script:Wave)) {
            Put $Destination 'partial failed copy bytes'
            throw 'Injected secondary copy failure'
        }
        & $script:RealCopy $Source $Destination
    }
    $script:DoDeploy=$true
    function Get-LatestRun { @(Get-ChildItem -LiteralPath $script:Active.config.receipts -Directory | Sort-Object Name)[-1].FullName }
    function script:Read-Host {
        param($Prompt)
        if ($Prompt -like '*Type READY') { return 'READY' }
        if (!$script:DoDeploy) { return 'LATER' }
        $receipt=Read-R17Json (Join-Path (Get-LatestRun) 'receipt.json')
        foreach ($target in $receipt.preflight.targets) {
            $rows=if ($receipt.status -eq 'STAGING_RESTORED') {$target.installedBefore} else {@(Get-R17DeploymentRows $receipt.candidate)}
            # Human Vortex simulation touches only manifest paths in local fixtures.
            foreach ($row in $rows) {
                $dest=Get-R17Child $target.installed $row.path
                if (Test-Path -LiteralPath $dest) { Remove-Item -LiteralPath $dest }
                if (!$row.ContainsKey('presence') -or $row.presence -eq 'present') {
                    Copy-R17IndependentFile (Get-R17Child $target.staging $row.path) $dest
                }
            }
        }
        if ($script:CorruptFinal) { Put (Join-Path $script:Active.config.targets[1].gameRoot "Data/$script:Wave") 'bad Vortex installed hash' }
        return 'DEPLOYED'
    }
    function Get-PrivateRows($Fixture) {
        @(foreach ($target in $Fixture.config.targets) {
            foreach ($base in @($target.stagingRoot,(Join-Path $target.gameRoot 'Data'))) {
                foreach ($rel in @('SkyrimTogetherReborn/config/STServer.ini','Saves/player.ess','Saves/player.skse','SkyrimTogetherReborn/campaigns/state.db','SkyrimTogetherReborn/identities/local.json','SkyrimTogetherReborn/logs/run.log','SkyrimTogetherReborn/cache/blob','unmanaged.txt','SkyrimTogetherReborn/UI/old.js')) {
                    @{root=$base; state=Get-R17FileState $base $rel 'private fixture evidence'}
                }
            }
        })
    }
    Case 'real workflow build failure stops before copies and gives phase-appropriate recovery' {
        $x=New-Fixture
        $script:FixtureBuild=(Get-Item Function:Invoke-R17Build).ScriptBlock
        function script:Invoke-R17Build { throw 'fixture compiler C1083' }
        try {
            try { Invoke-R17Workflow -ConfigPath $x.configPath; throw 'Expected failure' }
            catch {
                Check ($_.Exception.Message -match 'Stopped before staging writes' -and $_.Exception.Message -notmatch 'Use Verify|RestoreStaging') 'Build failure suggested destination recovery'
            }
        } finally { Set-Item Function:script:Invoke-R17Build $script:FixtureBuild }
        $failed=@(Get-ChildItem -LiteralPath $x.config.receipts -Directory)[0].FullName
        $receipt=Read-R17Json (Join-Path $failed 'receipt.json')
        Check ($receipt.failedPhase -eq 'BUILDING' -and $null -eq $receipt.preflight -and $null -eq $receipt.candidate) 'Wrong failed phase'
        Check (!(Test-Path (Join-Path $failed 'backup')) -and !(Test-Path (Join-Path $failed 'copy-A.json'))) 'Build failure entered destination writes'
        Check ((Get-R17BuildDirectory $failed $receipt.source) -eq $receipt.build.buildDirectory) 'Missing failed-build provenance'
    }
    Case 'end-to-end A/B sync uses installed A content and preserves private files and Git' {
        $script:Active=New-Fixture
        Remove-Item -LiteralPath (Join-Path $script:Active.config.targets[0].stagingRoot 'SkyrimTogetherReborn/config/STServer.ini')
        $private=Get-PrivateRows $script:Active
        $expected=Get-R17Hash (Join-Path $script:Active.config.targets[0].gameRoot "Data/$script:Wave")
        Invoke-R17Workflow -ConfigPath $script:Active.configPath
        $latest=Get-LatestRun; $receipt=Read-R17Json (Join-Path $latest 'receipt.json')
        Check ($receipt.status -eq 'INSTALLED_VERIFIED') 'False final status'
        Check (@($receipt.preflight.protected | Where-Object presence -eq 'absent').Count -eq 1) 'Staging INI absence lost'
        Check ((Read-R17Json (Join-Path $latest 'verified-B.json')).status -eq 'INSTALLED_VERIFIED') 'B receipt missing'
        foreach ($target in $script:Active.config.targets) {
            Check ((Get-R17Hash (Join-Path $target.gameRoot "Data/$script:Wave")) -eq $expected) 'Different WAV installed'
        }
        foreach ($item in $private) { Assert-R17State $item.root @($item.state) 'private files preserved' }
        Assert-R17Content (Join-Path $script:Active.repo 'GameFiles/Skyrim') $script:Active.reference.contentFiles -Repository
    }
    Case 'later documentation HEAD does not relabel the compiled candidate' {
        $latest=Get-LatestRun; $before=Read-R17Json (Join-Path $latest 'receipt.json')
        Put (Join-Path $script:Active.repo 'README.md') 'later documentation only'
        Invoke-FixtureGit $script:Active.repo @('add','README.md'); Invoke-FixtureGit $script:Active.repo @('commit','-qm','documentation fixture')
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode Verify -ReceiptPath $latest
        $verified=Read-R17Json (Join-Path $latest 'verified-A.json')
        Check ($verified.candidateCommit -eq $before.candidate.source.commit -and $verified.verificationHead -ne $verified.candidateCommit) 'Compiled commit relabelled'
    }
    Case 'staging alone cannot pass; Verify resumes the same frozen candidate' {
        $script:Active=New-Fixture; $script:DoDeploy=$false
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath } 'Staged only'
        $latest=Get-LatestRun
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode Verify -ReceiptPath $latest } 'Hash/size/presence mismatch'
        Check ((Read-R17Json (Join-Path $latest 'receipt.json')).status -eq 'STOPPED') 'False PASS after verify failure'
        $script:DoDeploy=$true; $null=Read-Host 'fixture human deploy'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode Verify -ReceiptPath $latest
    }
    Case 'partial B copy failure records exact progress; installed backups restore despite divergent staging' {
        $script:Active=New-Fixture; $script:FailSecond=$true
        $private=Get-PrivateRows $script:Active
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath } 'Injected secondary copy failure'
        $script:FailSecond=$false
        $latest=Get-LatestRun; $receipt=Read-R17Json (Join-Path $latest 'receipt.json')
        Check ($receipt.status -eq 'STOPPED' -and $receipt.failedPhase -eq 'STAGING_B') 'Missing precise failure phase'
        Check ((Test-Path (Join-Path $latest 'staged-A.json')) -and !(Test-Path (Join-Path $latest 'staged-B.json'))) 'False staging success'
        $journal=Read-R17Json (Join-Path $latest 'copy-B.json')
        Check (($journal.files | Where-Object path -eq $script:Wave).status -eq 'FAILED') 'Failed file not recorded'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest
        $null=Read-Host 'fixture human deploy'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode VerifyRestored -ReceiptPath $latest
        Check ((Read-R17Json (Join-Path $latest 'receipt.json')).status -eq 'RESTORED_VERIFIED') 'Missing restoration verification'
        foreach ($item in $private) { Assert-R17State $item.root @($item.state) 'private files after restoration' }
    }
    Case 'restore refuses unrelated intervening staging bytes before writes' {
        $latest=Get-LatestRun; $target=$script:Active.config.targets[1]
        $path=Join-Path $target.stagingRoot 'SkyrimTogetherReborn/SkyrimTogether.exe'
        $old=[IO.File]::ReadAllText($path); Put $path 'unrelated user change'
        try { Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest } 'unrelated changes' } finally { Put $path $old }
    }
    Case 'restore refuses corrupted backup before writing either PC' {
        $latest=Get-LatestRun; $path=Join-Path $latest 'backup/B/staging/SkyrimTogetherReborn/SkyrimTogether.exe'
        $old=[IO.File]::ReadAllText($path); Put $path 'corrupt backup'
        try { Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest } 'Hash/size/presence mismatch' } finally { Put $path $old }
    }
    Case 'incorrect final B hash cannot pass; A result and failure phase retained; rollback works' {
        $script:Active=New-Fixture; $script:CorruptFinal=$true
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath } 'Verify B installed Data.*mismatch'
        $script:CorruptFinal=$false
        $latest=Get-LatestRun; $receipt=Read-R17Json (Join-Path $latest 'receipt.json')
        Check ($receipt.status -eq 'STOPPED' -and $receipt.failedPhase -eq 'Verify_B') 'Final failure not precisely recorded'
        Check ((Read-R17Json (Join-Path $latest 'verified-A.json')).status -eq 'INSTALLED_VERIFIED') 'A verification evidence missing'
        Check (!(Test-Path (Join-Path $latest 'verified-B.json'))) 'False B final PASS'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest
        $null=Read-Host 'fixture human deploy'
        Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode VerifyRestored -ReceiptPath $latest
    }
    Case 'late unrelated edit before a replacement cannot become recoverable partial-copy evidence' {
        $script:Active=New-Fixture
        $script:OriginalState=(Get-Item Function:Assert-R17State).ScriptBlock
        function script:Assert-R17State {
            param($Root,$Rows,$Phase)
            if ($Phase -eq 'before individual replacement' -and $Root -eq $script:Active.config.targets[1].stagingRoot -and $Rows[0].path -eq $script:Wave) {
                Put (Join-Path $Root $script:Wave) 'unrelated late user edit'
            }
            & $script:OriginalState $Root $Rows $Phase
        }
        try { Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath } 'before individual replacement.*mismatch' }
        finally { Set-Item Function:script:Assert-R17State $script:OriginalState }
        $latest=Get-LatestRun
        $entry=@((Read-R17Json (Join-Path $latest 'copy-B.json')).files | Where-Object path -eq $script:Wave)[0]
        Check (!$entry.mutationStarted -and $null -eq $entry.observed) 'Unrelated edit was labelled partial-copy evidence'
        Reject { Invoke-R17Workflow -ConfigPath $script:Active.configPath -Mode RestoreStaging -ReceiptPath $latest } 'unrelated changes'
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
