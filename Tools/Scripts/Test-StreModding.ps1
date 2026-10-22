#requires -Version 7.2
# No Pester, game, AutoMod, xEdit or network needed. All writes use a disposable Git repo.
param(
    [switch]$RealTools,
    [string]$AutoModRoot = $env:STRE_AUTOMOD_ROOT,
    [string]$XEditPath = $env:STRE_XEDIT_PATH,
    [string]$DataRoot = $env:STRE_SKYRIM_DATA,
    # Optional durable SYNTHETIC copy, never a receipt or an importable session.
    [string]$HumanFixtureOutput
)
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'Run these toolchain tests on Windows.' }
$module = Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('stre-modding-tests-' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $testRoot
try {
    & $module {
        param($TestRoot, $RealTools, $AutoModRoot, $XEditPath, $DataRoot, $SourceRepo, $HumanFixtureOutput)
        $script:Passed = 0
        $script:RealAutoMod = ${function:Invoke-StreAutoMod}
        $script:RealPreflight = ${function:Assert-StrePreflight}
        $script:RealGuard = ${function:Assert-GameProcessesStopped}
        function Assert($Condition, $Message = 'Assertion failed') { if (-not $Condition) { throw $Message } }
        function Reject([scriptblock]$Body, [string]$Pattern) {
            try { & $Body | Out-Null } catch {
                if ($_.Exception.Message -notmatch $Pattern) { throw "Expected '$Pattern', received: $($_.Exception.Message)" }
                return
            }
            throw "Expected rejection: $Pattern"
        }
        function Test([string]$Name, [scriptblock]$Body) {
            & $Body
            $script:Passed++
            Write-Host "PASS $Name"
        }
        function Set-Header([string]$Path, [string[]]$Masters = @('Skyrim.esm','Update.esm','Dragonborn.esm'), [uint32]$Flags = 0) {
            $payload = [IO.MemoryStream]::new()
            $writer = [IO.BinaryWriter]::new($payload)
            $writer.Write([Text.Encoding]::ASCII.GetBytes('HEDR')); $writer.Write([uint16]12)
            $writer.Write([single]1.7); $writer.Write([uint32]0); $writer.Write([uint32]0x800)
            foreach ($master in $Masters) {
                $bytes = [Text.Encoding]::ASCII.GetBytes($master + [char]0)
                $writer.Write([Text.Encoding]::ASCII.GetBytes('MAST')); $writer.Write([uint16]$bytes.Length); $writer.Write($bytes)
                $writer.Write([Text.Encoding]::ASCII.GetBytes('DATA')); $writer.Write([uint16]8); $writer.Write([uint64]0)
            }
            $content = $payload.ToArray(); $writer.Dispose()
            $file = [IO.BinaryWriter]::new([IO.File]::Create($Path))
            try {
                $file.Write([Text.Encoding]::ASCII.GetBytes('TES4')); $file.Write([uint32]$content.Length)
                $file.Write($Flags); $file.Write([uint32]0); $file.Write([uint32]0); $file.Write([uint16]44); $file.Write([uint16]0)
                $file.Write($content)
            } finally { $file.Dispose() }
        }
        function New-Fixture {
            $repo = Join-Path $TestRoot ('fixture with spaces ' + [guid]::NewGuid().ToString('N'))
            $null = New-Item -ItemType Directory -Path "$repo/GameFiles/Skyrim", "$repo/game/Data", "$repo/tools"
            Set-Header "$repo/GameFiles/Skyrim/STRE_AlternateStart.esp"
            Copy-Item "$repo/GameFiles/Skyrim/STRE_AlternateStart.esp" "$repo/game/Data/STRE_AlternateStart.esp"
            Set-Header "$repo/game/Data/Skyrim.esm" @() 1
            Set-Header "$repo/game/Data/Update.esm" @('Skyrim.esm') 1
            Set-Header "$repo/game/Data/Dragonborn.esm" @('Skyrim.esm','Update.esm') 1
            [IO.File]::WriteAllText("$repo/.gitignore", "_audit/`ngame/`ntools/`n")
            [IO.File]::WriteAllText("$repo/tools/xEdit.exe", 'fixture tool identity')
            [IO.File]::WriteAllText("$repo/tools/automod.dll", 'fixture tool identity')
            @'
param([string[]]$RelativePath, [string]$SourceDataRoot)
$ErrorActionPreference = 'Stop'
foreach ($relative in $RelativePath) {
    Copy-Item -LiteralPath (Join-Path $SourceDataRoot $relative) -Destination (Join-Path $PSScriptRoot "GameFiles/Skyrim/$relative") -Force
}
'@ | Set-Content "$repo/import-from-ck-dev.ps1"
            $script:Config = @{
                RepoRoot = $repo; DataRoot = "$repo/game/Data"; AutoModRoot = "$repo/tools"
                AutoModProject = "$repo/tools/automod.csproj"; AutoModDll = "$repo/tools/automod.dll"
                XEditPath = "$repo/tools/xEdit.exe"; Runtime = "$repo/game/SkyrimSE.exe"
                Dotnet = 'dotnet'; ImportScript = "$repo/import-from-ck-dev.ps1"
                # Explicit synthetic identity, never an observation of a Skyrim executable.
                RuntimeIdentity = @{ Path = "$repo/game/SkyrimSE.exe"; Version = '1.7.104.0'; Sha256 = 'synthetic-runtime' }
            }
            $null = Invoke-StreGit $Config @('init', '-b', 'feat/fixture')
            $null = Invoke-StreGit $Config @('config', 'user.email', 'fixture@example.invalid')
            $null = Invoke-StreGit $Config @('config', 'user.name', 'Toolchain fixture')
            $null = Invoke-StreGit $Config @('add', '.')
            $null = Invoke-StreGit $Config @('commit', '-m', 'test: synthetic fixture')
            $null = Invoke-StreGit $Config @('update-ref', 'refs/remotes/origin/main', 'HEAD')
            $script:Request = @{ Operation = 'AddGlobal'; EditorId = 'STRE_ToolchainFixture'; Name = 'Fixture'; Priority = 50; Value = 7.25 }
            $script:Calls = [Collections.Generic.List[string]]::new()
            $script:Fault = ''
            $script:Before = Get-StreHash "$repo/game/Data/STRE_AlternateStart.esp"
        }
        function script:Assert-GameProcessesStopped { }
        function script:Assert-StrePreflight($Config) { @{ Branch = 'feat/fixture' } }
        function script:Invoke-StreAutoMod($Config, [string[]]$Arguments) {
            $script:Calls.Add(($Arguments -join ' '))
            $operation = $Arguments[1]
            if ($operation -eq 'list-records') {
                if ($script:Fault -eq 'duplicate') { return @{ success = $true; result = @(@{ EditorId = $script:Request.EditorId }) } }
                return @{ success = $true; result = @() }
            }
            if ($operation -in @('add-global','add-quest')) {
                $dry = '--dry-run' -in $Arguments
                if ($script:Fault -eq 'failure' -or ($script:Fault -eq 'write-failure' -and -not $dry)) {
                    return ConvertFrom-StreAutoModJson '{"success":false,"error":"fixture failure"}'
                }
                if (-not $dry -or $script:Fault -eq 'dry-write') {
                    Set-Header $Arguments[2] @('Dragonborn.esm','Skyrim.esm','Update.esm')
                }
                return @{ success = $true; result = @{ dryRun = $dry; editorId = $script:Request.EditorId } }
            }
            if ($operation -eq 'view-record') {
                $id = if ($script:Fault -eq 'wrong-record') { 'STRE_Wrong' } else { $script:Request.EditorId }
                $type = if ($script:Fault -eq 'wrong-type') { 'GlobalShort' } elseif ($script:Request.Operation -eq 'AddQuest') { 'QuestBinaryOverlay' } else { 'GlobalFloatBinaryOverlay' }
                return @{ success = $true; result = @{ EditorId = $id; RecordType = $type; FormKey = '000800:STRE_AlternateStart.esp'; Properties = @{ Data = '7.25'; Name = 'Fixture'; Priority = 50 } } }
            }
            if ($operation -eq 'list-masters') { return @{ success = $true; result = @{ masters = (Get-StreHeader $Arguments[2]).Masters } } }
            throw "Unexpected fixture call: $Arguments"
        }
        function Prepare {
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'HUMAN_XEDIT_REQUIRED'
            $script:Session = (Get-ChildItem "$($Config.RepoRoot)/_audit/modding" -Directory | Select-Object -First 1).FullName
        }
        function Attest-Fixture {
            # Simulated human receipt for synthetic files ONLY. Never real xEdit evidence.
            $candidate = "$Session/Data/STRE_AlternateStart.esp"
            Set-Header $candidate
            [IO.File]::AppendAllText($candidate, 'synthetic mutation')
            [IO.File]::WriteAllText("$Session/xedit-validation.log", 'SIMULATED FIXTURE; not an xEdit result')
            Write-StreJson "$Session/validation.json" @{
                Schema = 1; Reviewer = 'synthetic fixture'; SortMasters = $true; CheckForErrors = $true; SavedAndClosed = $true
                PluginSha256 = Get-StreHash $candidate; XEditSha256 = Get-StreHash $Config.XEditPath
                LogSha256 = Get-StreHash "$Session/xedit-validation.log"
            }
        }
        function Import-Fixture { Invoke-StreModding -Config $Config -Action ImportValidated -Session $Session }
        function Assert-Originals {
            Assert ((Get-StreHash "$($Config.DataRoot)/STRE_AlternateStart.esp") -eq $Before) 'Live fixture changed early'
            Assert ((Get-StreHash "$($Config.RepoRoot)/GameFiles/Skyrim/STRE_AlternateStart.esp") -eq $Before) 'Git fixture changed early'
        }

        Test 'JSON requires literal true and parses the result' {
            Assert ((ConvertFrom-StreAutoModJson '{"success":true,"result":{"value":42}}').result.value -eq 42)
        }
        foreach ($json in @('{"success":false}', '{"error":"no success"}', '{"success":"true"}', '{"success":1}', 'warning {"success":true}', '[{"success":true}]', '{"success":true}{"success":true}')) {
            Test "reject invalid/failure JSON: $json" { Reject { ConvertFrom-StreAutoModJson $json } 'AutoMod' }
        }
        Test 'nonzero exit rejects even success JSON' { Reject { ConvertFrom-StreAutoModJson '{"success":true}' 7 } 'code 7' }
        Test 'native runner preserves spaces and separates stderr' {
            $result = Invoke-StreProcess (Join-Path $PSHOME 'pwsh.exe') @('-NoProfile','-Command','[Console]::Out.Write("a b"); [Console]::Error.Write("err"); exit 9') $TestRoot
            Assert ($result.Out -eq 'a b' -and $result.Err -eq 'err' -and $result.ExitCode -eq 9)
        }
        Test 'config environment discovery and explicit override' {
            $saved = @($env:STRE_AUTOMOD_ROOT,$env:STRE_XEDIT_PATH,$env:STRE_SKYRIM_DATA)
            try {
                $env:STRE_AUTOMOD_ROOT = "$TestRoot/auto"; $env:STRE_XEDIT_PATH = "$TestRoot/x.exe"; $env:STRE_SKYRIM_DATA = "$TestRoot/Data"
                $c = Resolve-StreConfiguration -RepoRoot $TestRoot
                Assert ($c.AutoModRoot -eq [IO.Path]::GetFullPath("$TestRoot/auto"))
                $c = Resolve-StreConfiguration -RepoRoot $TestRoot -DataRoot "$TestRoot/override"
                Assert ($c.DataRoot -eq [IO.Path]::GetFullPath("$TestRoot/override"))
                Reject { Resolve-StreConfiguration -RepoRoot $TestRoot -AutoModRoot '' } 'Missing path'
            } finally { $env:STRE_AUTOMOD_ROOT,$env:STRE_XEDIT_PATH,$env:STRE_SKYRIM_DATA = $saved }
        }
        foreach ($branch in @('main','master','release/1','unsafe')) {
            Test "refuse mutation branch $branch" {
                New-Fixture
                $null = Invoke-StreGit $Config @('branch','-m',$branch)
                Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'Unsafe mutation branch'
                Assert ($Calls.Count -eq 0); Assert-Originals
            }
        }
        Test 'refuse detached HEAD' {
            New-Fixture; $null = Invoke-StreGit $Config @('checkout','--detach')
            Reject { Invoke-StreModding -Config $Config -Action DryRun -Request $Request } 'Unsafe mutation branch'
        }
        Test 'current origin/main ancestor permits a feature descendant' {
            New-Fixture
            $null = Invoke-StreGit $Config @('commit', '--allow-empty', '-m', 'test: feature descendant')
            $result = Invoke-StreModding -Config $Config -Action DryRun -Request $Request
            Assert ($result.State -eq 'DryRunOnly')
            Assert ($Calls.Count -eq 2); Assert-Originals
        }
        foreach ($diverged in @($false, $true)) {
            Test "refuse branch missing current origin/main (diverged=$diverged)" {
                New-Fixture
                $base = Invoke-StreGit $Config @('rev-parse', 'HEAD')
                $tree = Invoke-StreGit $Config @('rev-parse', 'HEAD^{tree}')
                if ($diverged) {
                    $null = Invoke-StreGit $Config @('commit', '--allow-empty', '-m', 'test: independent feature commit')
                }
                $upstream = Invoke-StreGit $Config @('commit-tree', $tree, '-p', $base, '-m', 'test: upstream advanced')
                $null = Invoke-StreGit $Config @('update-ref', 'refs/remotes/origin/main', $upstream)
                Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'origin/main must be an ancestor of HEAD'
                Assert ($Calls.Count -eq 0); Assert-Originals
                Assert (-not (Test-Path "$($Config.RepoRoot)/_audit/modding")) 'Rejected branch created a session'
            }
        }
        Test 'refuse missing origin/main even when a local main exists' {
            New-Fixture
            $null = Invoke-StreGit $Config @('branch', 'main')
            $null = Invoke-StreGit $Config @('update-ref', '-d', 'refs/remotes/origin/main')
            Reject { Invoke-StreModding -Config $Config -Action DryRun -Request $Request } 'origin/main is missing or invalid'
            Assert ($Calls.Count -eq 0); Assert-Originals
            Assert (-not (Test-Path "$($Config.RepoRoot)/_audit/modding")) 'Rejected branch created a session'
        }
        foreach ($plugin in @('Other.esp','SkyrimTogether.esp','skyrimtogether.esp','../STRE_AlternateStart.esp')) {
            Test "refuse write target $plugin" {
                New-Fixture
                Reject { Invoke-StreModding -Config $Config -Action Prepare -Plugin $plugin -Request $Request } 'allow-list|read-only|plain .esp'
                Assert ($Calls.Count -eq 0); Assert-Originals
            }
        }
        Test 'SkyrimTogether remains readable by policy' { Assert-StrePlugin 'SkyrimTogether.esp' }
        foreach ($stage in @($false, $true)) {
            Test "dirty Git ESP rejected (staged=$stage)" {
                New-Fixture; Add-Content "$($Config.RepoRoot)/GameFiles/Skyrim/STRE_AlternateStart.esp" 'dirty'
                if ($stage) { $null = Invoke-StreGit $Config @('add','GameFiles') }
                Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'Target Git ESP is dirty'
                Assert ($Calls.Count -eq 0)
            }
        }
        Test 'unrelated dirty checkout rejected' {
            New-Fixture; Set-Content "$($Config.RepoRoot)/unexpected.txt" 'dirty'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'working tree is dirty'
        }
        Test 'Data drift rejected before backup or tool call' {
            New-Fixture; Add-Content "$($Config.DataRoot)/STRE_AlternateStart.esp" 'drift'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'Data and repository ESP differ'
            Assert ($Calls.Count -eq 0)
        }
        Test 'prefix required; arbitrary operations refused' {
            New-Fixture; $Request.EditorId = 'Foreign'
            Reject { Get-StreMutationArguments $Request 'fixture.esp' } 'STRE_'
            $Request.EditorId = 'STRE_Ok'; $Request.Operation = 'delete'
            Reject { Get-StreMutationArguments $Request 'fixture.esp' } 'Unsupported operation'
        }
        Test 'duplicate EditorID rejected' {
            New-Fixture; $script:Fault = 'duplicate'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'already exists'
            Assert ($Calls.Count -eq 1); Assert-Originals
        }
        Test 'dry-run retains bytes and verified backups; cannot import' {
            New-Fixture
            $result = Invoke-StreModding -Config $Config -Action DryRun -Request $Request
            $script:Session = $result.Session
            Assert ($result.State -eq 'DryRunOnly')
            foreach ($file in @('before.esp','repository-before.esp','Data/STRE_AlternateStart.esp')) { Assert ((Get-StreHash "$Session/$file") -eq $Before) }
            Reject { Import-Fixture } 'not awaiting xEdit'
            Assert ($Calls.Count -eq 2); Assert-Originals
        }
        Test 'failure JSON stops before mutation, reread and import' {
            New-Fixture; $script:Fault = 'failure'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'success was not Boolean true'
            Assert ($Calls.Count -eq 2); Assert-Originals
        }
        Test 'dry-run hash violation stops before mutation' {
            New-Fixture; $script:Fault = 'dry-write'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'Dry-run response/identity/hash failed'
            Assert ($Calls.Count -eq 2); Assert-Originals
        }
        Test 'write failure stops before record reread' {
            New-Fixture; $script:Fault = 'write-failure'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'success was not Boolean true'
            Assert ($Calls.Count -eq 3); Assert-Originals
        }
        Test 'wrong reread record stops before xEdit and import' {
            New-Fixture; $script:Fault = 'wrong-record'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'record identity'
            Assert ($Calls.Count -eq 4); Assert-Originals
        }
        Test 'prepare stops explicitly at human xEdit gate' {
            New-Fixture; Prepare
            Assert ($Calls.Count -eq 4)
            Reject { Import-Fixture } 'HUMAN_XEDIT_REQUIRED'
            Assert-Originals
        }
        Test 'master order is independently enforced after attestation' {
            New-Fixture; Prepare; Attest-Fixture
            Set-Header "$Session/Data/STRE_AlternateStart.esp" @('Dragonborn.esm','Skyrim.esm','Update.esm')
            $receipt = Get-Content "$Session/validation.json" -Raw | ConvertFrom-Json -AsHashtable
            $receipt.PluginSha256 = Get-StreHash "$Session/Data/STRE_AlternateStart.esp"; Write-StreJson "$Session/validation.json" $receipt
            Reject { Import-Fixture } 'Wrong master order'; Assert-Originals
        }
        Test 'stale saved-file receipt rejected' {
            New-Fixture; Prepare; Attest-Fixture; Add-Content "$Session/Data/STRE_AlternateStart.esp" 'later edit'
            Reject { Import-Fixture } 'exact saved plugin'; Assert-Originals
        }
        Test 'stale evidence log rejected' {
            New-Fixture; Prepare; Attest-Fixture; Add-Content "$Session/xedit-validation.log" 'changed'
            Reject { Import-Fixture } 'evidence missing or changed'; Assert-Originals
        }
        Test 'changed branch rejected on resume' {
            New-Fixture; Prepare; Attest-Fixture; $null = Invoke-StreGit $Config @('branch','-m','feat/other')
            Reject { Import-Fixture } 'Git Branch changed'; Assert-Originals
        }
        Test 'changed Data rejected on resume' {
            New-Fixture; Prepare; Attest-Fixture; Add-Content "$($Config.DataRoot)/STRE_AlternateStart.esp" 'new CK work'
            Reject { Import-Fixture } 'changed since preparation'
        }
        Test 'ESL flag and format migration refused' {
            New-Fixture; $path = "$($Config.DataRoot)/STRE_AlternateStart.esp"; $original = Get-StreHeader $path
            Set-Header $path -Flags 0x200; Reject { Get-StreHeader $path } 'ESL flag'
            $changed = $original.Clone(); $changed.FormVersion = 43
            Reject { Assert-StreFormat $changed $original } 'format changed'
        }
        Test 'successful synthetic promotion uses importer and cannot replay' {
            New-Fixture; Prepare; Attest-Fixture
            $result = Import-Fixture
            Assert ($result.State -eq 'Imported')
            Assert ((Get-StreHash "$($Config.DataRoot)/STRE_AlternateStart.esp") -eq $result.Sha256)
            Assert ((Get-StreHash "$($Config.RepoRoot)/GameFiles/Skyrim/STRE_AlternateStart.esp") -eq $result.Sha256)
            Assert ((Get-StreHash "$Session/before.esp") -eq $Before)
            Reject { Import-Fixture } 'not awaiting xEdit'
        }
        Test 'importer nonzero exit leaves terminal stop and backups' {
            New-Fixture
            Set-Content $Config.ImportScript 'exit 23'
            $null = Invoke-StreGit $Config @('add','.'); $null = Invoke-StreGit $Config @('commit','-m','test: failing importer')
            Prepare; Attest-Fixture
            Reject { Import-Fixture } 'Existing CK import failed'
            Assert ((Get-StreHash "$Session/before.esp") -eq $Before)
            Assert ((Get-StreHash "$($Config.RepoRoot)/GameFiles/Skyrim/STRE_AlternateStart.esp") -eq $Before)
            Reject { Import-Fixture } 'not awaiting xEdit'
        }
        Test 'concurrent wrapper lock prevents a second pipeline' {
            New-Fixture; $null = New-Item -ItemType Directory "$($Config.RepoRoot)/_audit"
            $handle = [IO.File]::Open("$($Config.RepoRoot)/_audit/modding-operation.lock", 'OpenOrCreate','ReadWrite','None')
            try { Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'used by another process' }
            finally { $handle.Dispose() }
            Assert ($Calls.Count -eq 0)
        }
        Test 'snapshot refuses an existing destination' {
            New-Fixture
            Reject { Copy-StreVerified "$($Config.DataRoot)/STRE_AlternateStart.esp" "$($Config.RepoRoot)/GameFiles/Skyrim/STRE_AlternateStart.esp" } 'already exists'
            Assert-Originals
        }
        Test 'missing tools fail preflight without execution' {
            New-Fixture
            Reject { & $script:RealPreflight $Config } 'Required tool/file missing'
        }
        Test 'AutoMod invocation always includes --json and separates CLI arguments' {
            $native = ${function:Invoke-StreProcess}
            try {
                function script:Invoke-StreProcess($File, $Arguments, $Directory, $Environment) {
                    $script:NativeArguments = $Arguments
                    $script:NativeEnvironment = $Environment
                    @{ ExitCode = 0; Out = '{"success":true}'; Err = '' }
                }
                $null = & $script:RealAutoMod $Config @('esp','add-global','path with spaces.esp','STRE_Test','--dry-run')
                Assert (('--json' -in $NativeArguments) -and $NativeArguments[0] -ceq $Config.AutoModDll)
                Assert ('run' -notin $NativeArguments -and '--project' -notin $NativeArguments)
                Assert ($NativeArguments[-3] -eq 'path with spaces.esp' -and $NativeArguments[-1] -eq '--dry-run')
                Assert ($NativeEnvironment.DOTNET_SYSTEM_GLOBALIZATION_INVARIANT -eq '1')
            } finally { Set-Item Function:script:Invoke-StreProcess $native }
        }
        Test 'AutoMod stderr blocks even Boolean success' {
            $native = ${function:Invoke-StreProcess}
            try {
                function script:Invoke-StreProcess { @{ ExitCode = 0; Out = '{"success":true}'; Err = 'unexpected warning' } }
                Reject { & $script:RealAutoMod $Config @('papyrus','status') } 'AutoMod stderr'
            } finally { Set-Item Function:script:Invoke-StreProcess $native }
        }
        Test 'UNC and device paths rejected before filesystem lookup' {
            function script:Get-Item { throw 'FORBIDDEN filesystem lookup' }
            try {
                foreach ($path in @('\\example.invalid\share\file','//example.invalid/share/file','\\?\C:\file')) {
                    Reject { Resolve-StrePath $path } 'Only local filesystem'
                }
            } finally {
                function script:Get-Item { [CmdletBinding()]param([string]$LiteralPath,[switch]$Force) Microsoft.PowerShell.Management\Get-Item @PSBoundParameters }
            }
        }
        Test 'missing file and simulated access denial remain distinct' {
            Reject { Get-StreToolIdentity "$TestRoot/absent.exe" } 'Required tool/file missing'
            function script:Get-Item { throw [UnauthorizedAccessException]::new('SIMULATED access denied; no ACL changed') }
            try { Reject { Get-StreToolIdentity "$TestRoot/denied.exe" } 'SIMULATED access denied' }
            finally {
                function script:Get-Item { [CmdletBinding()]param([string]$LiteralPath,[switch]$Force) Microsoft.PowerShell.Management\Get-Item @PSBoundParameters }
            }
        }
        Test 'reparse ancestor is rejected before looking up its child' {
            function script:Get-Item {
                [CmdletBinding()]param([string]$LiteralPath,[switch]$Force)
                if ($LiteralPath.EndsWith('synthetic-junction')) { return @{Attributes=[IO.FileAttributes]::ReparsePoint} }
                if ($LiteralPath.EndsWith('must-not-be-resolved.exe')) { throw 'FORBIDDEN descendant lookup' }
                Microsoft.PowerShell.Management\Get-Item @PSBoundParameters
            }
            try { Reject { Resolve-StrePath "$TestRoot/synthetic-junction/must-not-be-resolved.exe" } 'Reparse point is not permitted' }
            finally {
                function script:Get-Item { [CmdletBinding()]param([string]$LiteralPath,[switch]$Force) Microsoft.PowerShell.Management\Get-Item @PSBoundParameters }
            }
        }
        Test 'exact runtime, action prerequisites and tool identity' {
            New-Fixture
            $identity = ${function:Get-StreToolIdentity}; $native = ${function:Invoke-StreProcess}
            $script:ObservedVersion = '1.7.104.0'; $script:ObservedProduct = "Spooky's AutoMod Toolkit"
            $script:ObservedXEditProduct = 'xEdit'; $script:RuntimeListing = 'Microsoft.NETCore.App 8.0.32 [synthetic]'
            try {
                function script:Get-StreToolIdentity($Path) {
                    if (-not $Path) { throw 'Required tool/file missing: unconfigured xEdit' }
                    @{Path=$Path;Version=$script:ObservedVersion;Sha256='synthetic';Product=$(if($Path -eq $Config.XEditPath){$script:ObservedXEditProduct}else{$script:ObservedProduct})}
                }
                function script:Invoke-StreProcess { @{ExitCode=0;Err='';Out=$script:RuntimeListing} }
                $result = & $script:RealPreflight $Config
                Assert ($result.Runtime.Version -ceq '1.7.104.0')
                foreach ($version in @('1.6.1170.0','1.7.105.0','9.0.0.0','')) {
                    $script:ObservedVersion = $version
                    Reject { & $script:RealPreflight $Config } 'Expected Skyrim SE Steam runtime 1.7.104.0 at'
                }
                $script:ObservedVersion = '1.7.104.0'
                $script:ObservedProduct = 'Unrelated tool'
                Reject { & $script:RealPreflight $Config } 'Unexpected AutoMod binary identity'
                $script:ObservedProduct = "Spooky's AutoMod Toolkit"; $script:ObservedXEditProduct = 'Unrelated editor'
                Reject { & $script:RealPreflight $Config } 'official xEdit'
                $Config.XEditPath = $null
                foreach ($action in @('Inspect','PapyrusStatus')) {
                    $result = & $script:RealPreflight $Config $action
                    Assert ($null -eq $result.XEdit -and $result.Action -ceq $action)
                }
                foreach ($action in @('Preflight','DryRun','Prepare','ImportValidated','FinalizeInterrupted')) {
                    Reject { & $script:RealPreflight $Config $action } 'Required tool/file missing'
                }
                $script:RuntimeListing = 'Microsoft.NETCore.App 10.0.8 [synthetic]'
                Reject { & $script:RealPreflight $Config 'PapyrusStatus' } 'requires the installed .NET 8'
            } finally {
                Set-Item Function:script:Get-StreToolIdentity $identity
                Set-Item Function:script:Invoke-StreProcess $native
            }
        }
        Test 'historical or changed runtime receipts cannot be relabeled' {
            New-Fixture; Prepare
            $statePath = Join-Path $Session 'session.json'
            $state = Get-Content $statePath -Raw | ConvertFrom-Json -AsHashtable
            $state.Config.Remove('RuntimeIdentity')
            Write-StreJson $statePath $state
            Reject { Import-Fixture } 'Historical session has no runtime identity'
            $state.Config.RuntimeIdentity = $Config.RuntimeIdentity.Clone()
            $state.Config.RuntimeIdentity.Sha256 = 'different-runtime'
            Write-StreJson $statePath $state
            Reject { Import-Fixture } 'Session runtime identity changed: Sha256'
            Assert-Originals
        }
        Test 'matching EditorID and value cannot conceal a wrong record type' {
            New-Fixture; $script:Fault = 'wrong-type'
            Reject { Invoke-StreModding -Config $Config -Action Prepare -Request $Request } 'Written record type'
            Assert-Originals
        }
        Test 'Valen authoring requires an explicitly visible private window' {
            Reject { New-StreValenSession @{} 'STRE_AlternateStart.esp' @{ShowXEdit=$false} } 'requires explicit'
        }
        Test 'Valen operation refuses an arbitrary EditorID' {
            Reject { New-StreValenSession @{} 'STRE_AlternateStart.esp' @{ShowXEdit=$true; EditorId='STRE_Other'} } 'fixed EditorID'
        }
        Test 'Valen companion allow-list rejects traversal before any writes' {
            $record = @{result=@{QF='QF_Test'; TIF='TIF_Test'; NewsInfo=0x03000807; AcceptInfo=0x03000808}}
            Reject { Assert-StreValenCompanions @{} @{Companions=@{'../evil.psc'='bad'}} $TestRoot $record } 'Unexpected companion paths'
        }
        Test 'Valen companion freeze detects modified content' {
            $root = Join-Path $TestRoot 'valen-corruption'
            $null = New-Item -ItemType Directory -Path "$root/Data/Scripts", "$root/frozen"
            [IO.File]::WriteAllText("$root/Data/Scripts/TIF_Test.pex", 'fixture bytes')
            Reject { Copy-StreValenCompanions @{} @{Companions=@{'Scripts/TIF_Test.pex'='wrong hash'}} $root "$root/frozen" } 'changed while freezing'
        }
        Test 'Papyrus accepts actual zero-error zero-warning success format' {
            Assert-StrePapyrusCompilation @{ExitCode=0; Err=''; Out="0 error(s), 0 warning(s)`r`nCompilation succeeded.`nBatch compile of 1 files finished. 1 succeeded, 0 failed."}
        }
        Test 'Papyrus availability is Boolean and never substitutes another tool' {
            foreach ($available in @($false, 'true', 1, $null)) {
                Reject { Get-StrePapyrusInputs @{result=@{compilerAvailable=$available}} '' '' } 'Papyrus compiler unavailable'
            }
            $compiler = Join-Path $TestRoot 'PapyrusCompiler.exe'
            $flags = Join-Path $TestRoot 'TESV_Papyrus_Flags.flg'
            [IO.File]::WriteAllText($compiler, 'SYNTHETIC IDENTITY ONLY')
            $status = @{result=@{compilerAvailable=$true;compilerPath="$TestRoot/other/PapyrusCompiler.exe";decompilerAvailable=$false}}
            Reject { Get-StrePapyrusInputs $status $compiler $flags } 'differs from the compiler discovered'
            $status.result.compilerPath = $compiler
            Reject { Get-StrePapyrusInputs $status $compiler $flags } 'Required tool/file missing'
            [IO.File]::WriteAllText($flags, 'SYNTHETIC FLAGS ONLY')
            Reject { Get-StrePapyrusInputs $status $compiler $flags } 'Required tool/file missing.*PapyrusAssembler'
            [IO.File]::WriteAllText((Join-Path $TestRoot 'PapyrusAssembler.exe'), 'SYNTHETIC IDENTITY ONLY')
            $inputs = Get-StrePapyrusInputs $status $compiler $flags
            Assert ($inputs.Compiler.Path -ieq $compiler) 'Compiling must not require a decompiler'
        }
        Test 'Papyrus rejects usage output even with exit zero' {
            Reject { Assert-StrePapyrusCompilation @{ExitCode=0; Err=''; Out='Usage: PapyrusCompiler'} } 'compilation failed'
        }
        Test 'Papyrus rejects warnings despite successful compilation' {
            Reject { Assert-StrePapyrusCompilation @{ExitCode=0; Err=''; Out="0 error(s), 0 warning(s)`nwarning: unsafe input`n1 succeeded, 0 failed"} } 'compilation failed'
        }
        # Exercise the REAL guard against observed process objects, never actual game processes.
        Set-Item Function:script:Assert-GameProcessesStopped $script:RealGuard
        foreach ($name in @('CreationKit','SkyrimSE','SkyrimTogether','SkyrimTogetherServer','Vortex','xTESEdit','xTESEdit64','SSEEdit','SSEEdit64','xSSEEdit','xSSEEdit64')) {
            $script:ProcessName = $name
            function script:Get-Process { [pscustomobject]@{ ProcessName = $script:ProcessName } }
            Test "process guard blocks $name" { Reject { Assert-GameProcessesStopped } 'Close game/CK/Vortex/xEdit' }
        }
        function script:Get-Process { throw 'enumeration unavailable' }
        Test 'process enumeration failure never means stopped' { Reject { Assert-GameProcessesStopped } 'enumeration unavailable' }
        function script:Get-Process { Microsoft.PowerShell.Management\Get-Process -ErrorAction Stop }
        if ($RealTools) {
            Set-Item Function:script:Invoke-StreAutoMod $script:RealAutoMod
            Set-Item Function:script:Assert-StrePreflight $script:RealPreflight
            $production = @{}
            foreach ($root in @($DataRoot, (Join-Path $SourceRepo 'GameFiles/Skyrim'))) {
                foreach ($name in @('STRE_AlternateStart.esp','SkyrimTogether.esp')) {
                    $path = Join-Path $root $name
                    $production[$path] = Get-StreHash $path
                }
            }
            try {
                Test 'REAL AutoMod dry-run on a synthetic ESP keeps identical bytes' {
                    New-Fixture
                    $script:Config = Resolve-StreConfiguration -RepoRoot $Config.RepoRoot -DataRoot $Config.DataRoot -AutoModRoot $AutoModRoot -XEditPath $XEditPath
                    # The real runtime is inspected only. Every ESP path stays in the temporary fixture.
                    $Config.Runtime = Join-Path (Split-Path -Parent $DataRoot) 'SkyrimSE.exe'
                    $result = Invoke-StreModding -Config $Config -Action DryRun -Request $Request
                    Assert ($result.State -eq 'DryRunOnly')
                    Assert ((Get-StreHash "$($result.Session)/Data/STRE_AlternateStart.esp") -eq $Before)
                    Assert-Originals
                }
                Test 'REAL AutoMod writes and rereads a GLOB on a fixture; stops at human gate' {
                    Prepare
                    # Multiple sessions exist (dry-run then prepare), select the pending one explicitly.
                    $script:Session = @(Get-ChildItem "$($Config.RepoRoot)/_audit/modding" -Directory | Where-Object {
                        (Get-Content (Join-Path $_.FullName 'session.json') -Raw | ConvertFrom-Json).State -eq 'AwaitingHumanXEdit'
                    })[0].FullName
                    $record = Get-StreExpectedRecord $Config $Request "$Session/Data/STRE_AlternateStart.esp"
                    Assert ($record.result.EditorId -eq 'STRE_ToolchainFixture')
                    Assert ((Get-StreHash "$Session/Data/STRE_AlternateStart.esp") -ne $Before)
                    Reject { Import-Fixture } 'HUMAN_XEDIT_REQUIRED'
                    Assert-Originals
                    if ($HumanFixtureOutput) {
                        $output = Resolve-StrePath $HumanFixtureOutput
                        $allowed = (Resolve-StrePath (Join-Path $SourceRepo '_audit')).TrimEnd('\') + '\'
                        if (-not $output.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Human fixture must be a new local repository _audit subdirectory.' }
                        $null = New-Item -ItemType Directory -Path $output -ErrorAction Stop
                        foreach ($name in @('STRE_AlternateStart.esp') + $script:Masters) {
                            Copy-StreVerified (Join-Path $Session "Data/$name") (Join-Path $output $name)
                        }
                        Copy-StreVerified (Join-Path $Session 'before.esp') (Join-Path $output 'before.esp')
                        Write-StreJson (Join-Path $output 'synthetic-evidence.json') @{
                            Kind = 'SYNTHETIC TEST ONLY; not a production session or human attestation'
                            Record = $record; HeaderBefore = Get-StreHeader (Join-Path $Session 'before.esp')
                            HeaderAfter = Get-StreHeader (Join-Path $output 'STRE_AlternateStart.esp')
                            CandidateSha256 = Get-StreHash (Join-Path $output 'STRE_AlternateStart.esp')
                            Runtime = $Config.RuntimeIdentity; XEdit = Get-StreToolIdentity $Config.XEditPath
                            AutoModSha256 = Get-StreHash $Config.AutoModDll
                        }
                    }
                }
            }
            finally {
                foreach ($path in $production.Keys) { Assert ((Get-StreHash $path) -eq $production[$path]) "Production ESP changed: $path" }
            }
            Write-Host 'PASS all four production ESP hashes unchanged; no production importer or xEdit session executed.'
        }
        Write-Host "RESULT: $script:Passed passed, 0 failed (synthetic fixtures; no xEdit validation claimed)."
    } $testRoot $RealTools $AutoModRoot $XEditPath $DataRoot (Join-Path $PSScriptRoot '../..') $HumanFixtureOutput
}
finally {
    Remove-Module $module -Force
    # Delete only the exact newly-created temporary test root.
    $resolved = [IO.Path]::GetFullPath($testRoot)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (-not $resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or
        (Split-Path -Leaf $resolved) -notmatch '^stre-modding-tests-[a-f0-9]{32}$') { throw 'Unsafe test cleanup path.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
