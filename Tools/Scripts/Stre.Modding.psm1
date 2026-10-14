#requires -Version 7.2
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Stre.ProcessGuard.ps1')
$script:Masters = @('Skyrim.esm', 'Update.esm', 'Dragonborn.esm')

function Get-StreHash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256 -ErrorAction Stop).Hash
}

function Resolve-StrePath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'Missing path configuration; use parameters or STRE_* environment variables.' }
    $full = [IO.Path]::GetFullPath($Path)
    # Reject junction/symlink traversal, including ancestors of not-yet-created paths.
    for ($part = $full; $part; $part = Split-Path -Parent $part) {
        if ((Test-Path -LiteralPath $part) -and
            ((Get-Item -LiteralPath $part -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Reparse point is not permitted: $part"
        }
    }
    $full
}

function Invoke-StreProcess([string]$File, [string[]]$Arguments, [string]$Directory, [hashtable]$Environment = @{}) {
    $start = [Diagnostics.ProcessStartInfo]::new($File)
    $start.WorkingDirectory = $Directory
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($key in $Environment.Keys) { $start.Environment[$key] = $Environment[$key] }
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    try {
        if (-not $process.Start()) { throw "Could not start $File" }
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(120000)) {
            $process.Kill($true)
            $process.WaitForExit()
            throw "Tool timed out: $File"
        }
        @{ ExitCode = $process.ExitCode; Out = $stdout.GetAwaiter().GetResult(); Err = $stderr.GetAwaiter().GetResult() }
    }
    finally { $process.Dispose() }
}

function Invoke-StreGit($Config, [string[]]$Arguments) {
    $result = Invoke-StreProcess 'git' (@('-C', $Config.RepoRoot) + $Arguments) $Config.RepoRoot
    if ($result.ExitCode -ne 0) { throw "Git failed: $($result.Err)" }
    $result.Out.Trim()
}

function ConvertFrom-StreAutoModJson([string]$Text, [int]$ExitCode = 0) {
    if ($ExitCode -ne 0) { throw "AutoMod exited with code $ExitCode. $Text" }
    try { $parsed = ConvertFrom-Json -InputObject $Text -AsHashtable -NoEnumerate -ErrorAction Stop }
    catch { throw "AutoMod did not return one valid JSON document: $($_.Exception.Message)" }
    if ($parsed -isnot [Collections.IDictionary] -or -not $parsed.Contains('success') -or
        $parsed.success -isnot [bool] -or -not $parsed.success) {
        throw "AutoMod success was not Boolean true. $Text"
    }
    $parsed
}

function Invoke-StreAutoMod($Config, [string[]]$Arguments) {
    $result = Invoke-StreProcess $Config.Dotnet (@(
        'run', '--no-build', '--no-restore', '--project', $Config.AutoModProject, '--', '--json'
    ) + $Arguments) $Config.AutoModRoot @{
        # System.CommandLine parses floats using child culture; reflection also formats them.
        # Keep both deterministic on French and English Windows without changing the user's locale.
        DOTNET_SYSTEM_GLOBALIZATION_INVARIANT = '1'
        DOTNET_SYSTEM_GLOBALIZATION_PREDEFINED_CULTURES_ONLY = '0'
    }
    if ($result.Err.Trim()) { throw "AutoMod stderr; stopping: $($result.Err)" }
    ConvertFrom-StreAutoModJson $result.Out $result.ExitCode
}

function Resolve-StreConfiguration {
    param([string]$RepoRoot, [string]$AutoModRoot = $env:STRE_AUTOMOD_ROOT,
        [string]$XEditPath = $env:STRE_XEDIT_PATH, [string]$DataRoot = $env:STRE_SKYRIM_DATA)
    if (-not $IsWindows) { throw 'The STRE modding toolchain requires Windows.' }
    $config = @{
        RepoRoot = Resolve-StrePath $RepoRoot
        AutoModRoot = Resolve-StrePath $AutoModRoot
        XEditPath = Resolve-StrePath $XEditPath
        DataRoot = Resolve-StrePath $DataRoot
        Dotnet = (Get-Command dotnet -CommandType Application -ErrorAction Stop).Source
    }
    $config.AutoModProject = Join-Path $config.AutoModRoot 'src/SpookysAutomod.Cli/SpookysAutomod.Cli.csproj'
    $config.AutoModDll = Join-Path $config.AutoModRoot 'src/SpookysAutomod.Cli/bin/Debug/net8.0/spookys-automod.dll'
    $config.Runtime = Join-Path (Split-Path -Parent $config.DataRoot) 'SkyrimSE.exe'
    $config.ImportScript = Join-Path $config.RepoRoot 'import-from-ck-dev.ps1'
    $config
}

function Assert-StrePreflight($Config) {
    if (-not $IsWindows) { throw 'The STRE modding toolchain requires Windows.' }
    foreach ($path in @($Config.XEditPath, $Config.AutoModProject, $Config.AutoModDll, $Config.Runtime)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required tool/file missing: $path" }
    }
    if ((Get-Item -LiteralPath $Config.Runtime).VersionInfo.FileVersion -ne '1.6.1170.0') {
        throw 'Expected Skyrim SE Steam runtime 1.6.1170.0; no other runtime is accepted.'
    }
    if ((Get-Item -LiteralPath $Config.XEditPath).VersionInfo.ProductName -ne 'xEdit') {
        throw 'Expected the official xEdit executable (ProductName=xEdit). Verify its release provenance.'
    }
    $runtime = Invoke-StreProcess $Config.Dotnet @('--list-runtimes') $Config.AutoModRoot
    if ($runtime.ExitCode -ne 0 -or $runtime.Out -notmatch '(?m)^Microsoft.NETCore.App 8\.') {
        throw 'AutoMod requires the installed .NET 8 runtime and a prebuilt Debug CLI.'
    }
    Assert-GameProcessesStopped
    @{ Branch = Invoke-StreGit $Config @('branch', '--show-current'); Head = Invoke-StreGit $Config @('rev-parse', 'HEAD') }
}

function Assert-StrePlugin([string]$Plugin, [switch]$Writable) {
    if ($Plugin -notmatch '^[A-Za-z0-9_][A-Za-z0-9_. -]*\.esp$' -or $Plugin.Contains('..')) {
        throw 'Plugin must be a plain .esp filename, without path/option syntax.'
    }
    if ($Writable) {
        if ($Plugin -ieq 'SkyrimTogether.esp') { throw 'SkyrimTogether.esp is explicitly read-only.' }
        if ($Plugin -cne 'STRE_AlternateStart.esp') { throw "Plugin is outside the write allow-list: $Plugin" }
    }
}

function Assert-StreMutationGit($Config, [string]$Plugin) {
    Assert-StrePlugin $Plugin -Writable
    $branch = Invoke-StreGit $Config @('branch', '--show-current')
    if ($branch -eq 'main' -or $branch -notmatch '^(feat|fix|test|chore|codex)/[A-Za-z0-9][A-Za-z0-9._/-]*$') {
        throw "Unsafe mutation branch: '$branch'. Use a dedicated feature branch; main/detached HEAD are forbidden."
    }
    $target = "GameFiles/Skyrim/$Plugin"
    if ((Invoke-StreGit $Config @('ls-files', '--', $target)) -cne $target) { throw 'Target ESP must be tracked by Git.' }
    if (Invoke-StreGit $Config @('status', '--porcelain=v1', '--untracked-files=all', '--', $target)) {
        throw 'Target Git ESP is dirty; refusing to overwrite it. No AllowDirtyTarget bypass exists.'
    }
    # Whole feature checkout must be clean before preparing/importing; untracked plans go in _audit.
    if (Invoke-StreGit $Config @('status', '--porcelain=v1', '--untracked-files=all')) {
        throw 'Feature working tree is dirty; commit or resolve existing changes before mutation.'
    }
    foreach ($marker in @('MERGE_HEAD', 'CHERRY_PICK_HEAD', 'REVERT_HEAD', 'rebase-merge', 'rebase-apply')) {
        $path = Invoke-StreGit $Config @('rev-parse', '--git-path', $marker)
        if (-not [IO.Path]::IsPathRooted($path)) { $path = Join-Path $Config.RepoRoot $path }
        if (Test-Path -LiteralPath $path) { throw "Git operation in progress: $marker" }
    }
    @{ Branch = $branch; Head = Invoke-StreGit $Config @('rev-parse', 'HEAD') }
}

function Get-StreHeader([string]$Path) {
    $stream = [IO.File]::Open($Path, 'Open', 'Read', 'Read')
    $reader = [IO.BinaryReader]::new($stream)
    try {
        if ([Text.Encoding]::ASCII.GetString($reader.ReadBytes(4)) -cne 'TES4') { throw 'Not a TES4 plugin.' }
        $size = $reader.ReadUInt32()
        $flags = $reader.ReadUInt32()
        if ($size -gt 1MB -or $size + 24 -gt $stream.Length) { throw 'Invalid TES4 header size.' }
        $stream.Position = 20
        $formVersion = $reader.ReadUInt16()
        $stream.Position = 24
        $masters = @()
        $version = $null
        while ($stream.Position -lt ($size + 24)) {
            if ($stream.Position + 6 -gt $size + 24) { throw 'Truncated header subrecord.' }
            $sig = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(4))
            $length = $reader.ReadUInt16()
            if ($stream.Position + $length -gt $size + 24) { throw 'Invalid header subrecord size.' }
            $bytes = $reader.ReadBytes($length)
            if ($sig -eq 'HEDR') {
                if ($length -ne 12 -or $null -ne $version) { throw 'Invalid HEDR.' }
                $version = [BitConverter]::ToUInt32($bytes, 0)
            }
            if ($sig -eq 'MAST') { $masters += [Text.Encoding]::ASCII.GetString($bytes).TrimEnd([char]0) }
        }
        if ($null -eq $version -or ($flags -band 0x200)) { throw 'Missing HEDR or forbidden ESL flag.' }
        @{ Flags = $flags; FormVersion = $formVersion; VersionBits = $version; Masters = $masters }
    }
    finally { $reader.Dispose() }
}

function Assert-StreFormat($Header, $Baseline, [switch]$Normalized) {
    foreach ($field in @('Flags', 'FormVersion', 'VersionBits')) {
        if ($Header[$field] -ne $Baseline[$field]) { throw "Plugin format changed: $field" }
    }
    if ($Normalized -and ($Header.Masters -join '|') -cne ($script:Masters -join '|')) {
        throw "Wrong master order; require Skyrim.esm, Update.esm, Dragonborn.esm. Actual: $($Header.Masters -join ', ')"
    }
}

function Copy-StreVerified([string]$Source, [string]$Destination) {
    $null = Resolve-StrePath $Source
    $null = Resolve-StrePath $Destination
    Assert-GameProcessesStopped
    # Source remains locked against writes/deletion throughout snapshot and hashing.
    $inputFile = [IO.File]::Open($Source, 'Open', 'Read', 'Read')
    try {
        $outputFile = [IO.File]::Open($Destination, 'CreateNew', 'Write', 'None')
        try { $inputFile.CopyTo($outputFile) } finally { $outputFile.Dispose() }
        if ((Get-StreHash $Source) -ne (Get-StreHash $Destination)) { throw 'Snapshot hash mismatch.' }
    }
    finally { $inputFile.Dispose() }
}

function Write-StreJson([string]$Path, $Value) {
    $Value | ConvertTo-Json -Depth 32 | Set-Content -LiteralPath $Path -Encoding utf8
}

function Get-StreMutationArguments($Request, [string]$Path) {
    if ($Request.EditorId -cnotmatch '^STRE_[A-Za-z0-9_]+$') { throw 'New records require an STRE_ EditorID.' }
    switch ($Request.Operation) {
        'AddQuest' {
            if ([string]::IsNullOrWhiteSpace($Request.Name) -or $Request.Name.StartsWith('-')) { throw 'Quest requires a display name, not an option.' }
            @('esp', 'add-quest', $Path, $Request.EditorId, '--name', $Request.Name, '--priority', [string][byte]$Request.Priority)
        }
        'AddGlobal' {
            $value = [single]$Request.Value
            if ([single]::IsNaN($value) -or [single]::IsInfinity($value)) { throw 'Global value must be finite.' }
            @('esp', 'add-global', $Path, $Request.EditorId, '--type', 'float', '--value', $value.ToString([Globalization.CultureInfo]::InvariantCulture))
        }
        default { throw 'Unsupported operation. Add a reviewed typed adapter; no arbitrary command passthrough.' }
    }
}

function Get-StreExpectedRecord($Config, $Request, [string]$Path) {
    $type = if ($Request.Operation -eq 'AddQuest') { 'quest' } else { 'global' }
    $record = Invoke-StreAutoMod $Config @('esp', 'view-record', $Path, '--editor-id', $Request.EditorId, '--type', $type, '--include-raw')
    if ($record.result.EditorId -cne $Request.EditorId -or $record.result.FormKey -notmatch ':STRE_AlternateStart\.esp$') {
        throw 'Written record identity did not match the request.'
    }
    if ($type -eq 'quest') {
        if ($record.result.Properties.Name -cne $Request.Name -or $record.result.Properties.Priority -ne $Request.Priority) {
            throw 'Written quest properties did not match the request.'
        }
    }
    else {
        if ([single]$record.result.Properties.Data -ne [single]$Request.Value) { throw 'Written global value did not match the request.' }
    }
    $record
}

function New-StreSession($Config, [string]$Plugin, $Request, [switch]$DryRun) {
    $git = Assert-StreMutationGit $Config $Plugin
    if (-not (Test-Path -LiteralPath $Config.ImportScript -PathType Leaf)) { throw 'Existing local CK importer is required.' }
    $source = Resolve-StrePath (Join-Path $Config.DataRoot $Plugin)
    $repoPlugin = Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$Plugin")
    $sourceHash = Get-StreHash $source
    if ($sourceHash -ne (Get-StreHash $repoPlugin)) { throw 'Data and repository ESP differ; reconcile through the existing CK import first.' }
    $header = Get-StreHeader $source
    Assert-StreFormat $header $header -Normalized
    $null = Get-StreMutationArguments $Request $source
    $records = Invoke-StreAutoMod $Config @('esp', 'list-records', $source, '--limit', '2147483647')
    if (@($records.result | Where-Object { $_.EditorId -ieq $Request.EditorId }).Count) { throw 'EditorID already exists; creation cannot overwrite a record.' }
    $root = Resolve-StrePath (Join-Path $Config.RepoRoot ("_audit/modding/" + [guid]::NewGuid().ToString('N')))
    $null = New-Item -ItemType Directory -Path $root
    $data = Join-Path $root 'Data'
    $null = New-Item -ItemType Directory -Path $data
    Copy-StreVerified $source (Join-Path $root 'before.esp')
    Copy-StreVerified $repoPlugin (Join-Path $root 'repository-before.esp')
    $candidate = Join-Path $data $Plugin
    Copy-StreVerified (Join-Path $root 'before.esp') $candidate
    $state = @{
        Schema = 1; State = 'Started'; Plugin = $Plugin; Request = $Request; Git = $git
        Config = $Config; BeforeHash = $sourceHash; Header = $header
        XEditHash = Get-StreHash $Config.XEditPath; ImportHash = Get-StreHash $Config.ImportScript
        AutoModHash = Get-StreHash $Config.AutoModDll
    }
    $statePath = Join-Path $root 'session.json'
    Write-StreJson $statePath $state
    Write-Host "Session / backups: $root"
    try {
        $arguments = Get-StreMutationArguments $Request $candidate
        $preview = Invoke-StreAutoMod $Config ($arguments + '--dry-run')
        Write-StreJson (Join-Path $root 'dry-run.json') $preview
        if ($preview.result.dryRun -isnot [bool] -or -not $preview.result.dryRun -or
            $preview.result.editorId -cne $Request.EditorId -or (Get-StreHash $candidate) -ne $sourceHash) {
            throw 'Dry-run response/identity/hash failed; no mutation or import may follow.'
        }
        if ($DryRun) {
            $state.State = 'DryRunOnly'
            Write-StreJson $statePath $state
            return @{ Session = $root; State = $state.State; Sha256 = $sourceHash }
        }
        Assert-GameProcessesStopped
        $null = Assert-StreMutationGit $Config $Plugin
        $write = Invoke-StreAutoMod $Config $arguments
        Write-StreJson (Join-Path $root 'mutation.json') $write
        if ($write.result.dryRun -isnot [bool] -or $write.result.dryRun -or $write.result.editorId -cne $Request.EditorId) { throw 'Invalid mutation response.' }
        $record = Get-StreExpectedRecord $Config $Request $candidate
        Write-StreJson (Join-Path $root 'record-after.json') $record
        Assert-StreFormat (Get-StreHeader $candidate) $header
        # Private Data for the human xEdit session: no links to live masters or plugins.
        foreach ($master in $script:Masters) { Copy-StreVerified (Join-Path $Config.DataRoot $master) (Join-Path $data $master) }
        $state.State = 'AwaitingHumanXEdit'
        Write-StreJson $statePath $state
        Write-StreJson (Join-Path $root 'validation.json') @{
            Schema = 1; PluginSha256 = ''; XEditSha256 = $state.XEditHash; Reviewer = ''
            SortMasters = $false; CheckForErrors = $false; SavedAndClosed = $false; LogSha256 = ''
        }
        throw "HUMAN_XEDIT_REQUIRED: Sort Masters, save/close, reopen and Check for Errors on $candidate. Complete validation.json with the saved file/log hashes. See MODDING_TOOLCHAIN.md. Import is blocked."
    }
    catch {
        if ($state.State -ne 'AwaitingHumanXEdit') { $state.State = 'Failed'; Write-StreJson $statePath $state }
        throw
    }
}

function Import-StreValidated($Config, [string]$Session) {
    $root = Resolve-StrePath $Session
    $sessionParent = Resolve-StrePath (Join-Path $Config.RepoRoot '_audit/modding')
    if ((Split-Path -Parent $root) -ine $sessionParent -or (Split-Path -Leaf $root) -notmatch '^[a-f0-9]{32}$') { throw 'Session must be an isolated _audit/modding/<id> directory in this repository.' }
    $state = Get-Content -LiteralPath (Join-Path $root 'session.json') -Raw | ConvertFrom-Json -AsHashtable
    if ($state.Schema -ne 1 -or $state.State -ne 'AwaitingHumanXEdit') { throw 'Session is not awaiting xEdit; failed/dry-run/imported sessions cannot be promoted.' }
    $git = Assert-StreMutationGit $Config $state.Plugin
    foreach ($key in @('Branch', 'Head')) { if ($git[$key] -cne $state.Git[$key]) { throw "Git $key changed since preparation." } }
    foreach ($key in @('RepoRoot', 'DataRoot', 'XEditPath', 'AutoModRoot', 'ImportScript')) {
        if ($Config[$key] -ine $state.Config[$key]) { throw "Configuration changed since preparation: $key" }
    }
    foreach ($pair in @(@('XEditPath', 'XEditHash'), @('ImportScript', 'ImportHash'), @('AutoModDll', 'AutoModHash'))) {
        if ((Get-StreHash $Config[$pair[0]]) -ne $state[$pair[1]]) { throw "Tool changed: $($pair[0])" }
    }
    $candidate = Resolve-StrePath (Join-Path $root "Data/$($state.Plugin)")
    $receipt = Get-Content -LiteralPath (Join-Path $root 'validation.json') -Raw | ConvertFrom-Json -AsHashtable
    foreach ($field in @('SortMasters', 'CheckForErrors', 'SavedAndClosed')) {
        if ($receipt[$field] -isnot [bool] -or -not $receipt[$field]) { throw "HUMAN_XEDIT_REQUIRED: $field has not been attested." }
    }
    if ($receipt.Schema -ne 1 -or [string]::IsNullOrWhiteSpace($receipt.Reviewer) -or
        $receipt.XEditSha256 -ne $state.XEditHash -or $receipt.PluginSha256 -ne (Get-StreHash $candidate)) { throw 'Human receipt does not identify the exact saved plugin/xEdit.' }
    $log = Resolve-StrePath (Join-Path $root 'xedit-validation.log')
    if ((Get-Item -LiteralPath $log).Length -eq 0 -or $receipt.LogSha256 -ne (Get-StreHash $log)) { throw 'Human xEdit log evidence missing or changed.' }
    Assert-StreFormat (Get-StreHeader $candidate) $state.Header -Normalized
    $masters = Invoke-StreAutoMod $Config @('esp', 'list-masters', $candidate)
    if (($masters.result.masters -join '|') -cne ($script:Masters -join '|')) { throw 'AutoMod master reread disagrees with the normalized header.' }
    $record = Get-StreExpectedRecord $Config $state.Request $candidate
    $originalRecord = Get-Content -LiteralPath (Join-Path $root 'record-after.json') -Raw | ConvertFrom-Json -AsHashtable
    if ($record.result.FormKey -cne $originalRecord.result.FormKey) { throw 'Created record FormKey changed during xEdit validation.' }
    $source = Resolve-StrePath (Join-Path $Config.DataRoot $state.Plugin)
    $repoPlugin = Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$($state.Plugin)")
    foreach ($path in @($source, $repoPlugin, (Join-Path $root 'before.esp'), (Join-Path $root 'repository-before.esp'))) {
        if ((Get-StreHash $path) -ne $state.BeforeHash) { throw 'Data/repository/backup changed since preparation; refusing promotion.' }
    }
    # Freeze the human-validated bytes before either Data or Git is touched.
    $snapshot = Join-Path $root 'validated'
    $null = New-Item -ItemType Directory -Path $snapshot
    $frozen = Join-Path $snapshot $state.Plugin
    Copy-StreVerified $candidate $frozen
    if ((Get-StreHash $frozen) -ne $receipt.PluginSha256) { throw 'Candidate changed during validation.' }
    Assert-GameProcessesStopped
    $null = Assert-StreMutationGit $Config $state.Plugin
    $state.State = 'ImportStarted'
    Write-StreJson (Join-Path $root 'session.json') $state
    # Keep live CK Data aligned: the existing build script will snapshot/import it again.
    $destination = [IO.File]::Open($source, 'Open', 'ReadWrite', 'None')
    try {
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $lockedHash = [Convert]::ToHexString($sha.ComputeHash($destination)) } finally { $sha.Dispose() }
        if ($lockedHash -ne $state.BeforeHash) { throw 'Data changed before promotion.' }
        $destination.Position = 0
        $inputFile = [IO.File]::OpenRead($frozen)
        try { $inputFile.CopyTo($destination); $destination.SetLength($inputFile.Length) } finally { $inputFile.Dispose() }
    }
    finally { $destination.Dispose() }
    if ((Get-StreHash $source) -ne $receipt.PluginSha256) { throw 'Data promotion hash mismatch; stop and use retained backups.' }
    Assert-GameProcessesStopped
    $import = Invoke-StreProcess (Join-Path $PSHOME 'pwsh.exe') @(
        '-NoProfile', '-File', $Config.ImportScript, '-RelativePath', $state.Plugin, '-SourceDataRoot', $snapshot
    ) $Config.RepoRoot
    Write-StreJson (Join-Path $root 'import-result.json') $import
    if ($import.ExitCode -ne 0 -or (Get-StreHash $repoPlugin) -ne $receipt.PluginSha256) { throw 'Existing CK import failed; stop and use retained backups.' }
    $state.State = 'Imported'
    Write-StreJson (Join-Path $root 'session.json') $state
    @{ State = $state.State; Session = $root; Sha256 = $receipt.PluginSha256
        GitStatus = Invoke-StreGit $Config @('status', '--short')
        Next = 'Review git diff/status and CK audits, then run the existing build-and-deploy-dev.ps1 and human in-game validation.' }
}

function Invoke-StreModdingCore {
    param($Config, [string]$Action, [string]$Plugin = 'STRE_AlternateStart.esp', $Request = @{}, [string]$Session, [string]$RecordType = 'quest')
    $preflight = Assert-StrePreflight $Config
    Assert-StrePlugin $Plugin
    switch ($Action) {
        'Preflight' { return $preflight }
        'PapyrusStatus' { return Invoke-StreAutoMod $Config @('papyrus', 'status') }
        'Inspect' {
            $path = Resolve-StrePath (Join-Path $Config.DataRoot $Plugin)
            $before = Get-StreHash $path
            $info = Invoke-StreAutoMod $Config @('esp', 'info', $path)
            $masters = Invoke-StreAutoMod $Config @('esp', 'list-masters', $path)
            $records = Invoke-StreAutoMod $Config @('esp', 'list-records', $path, '--type', $RecordType, '--limit', '2147483647')
            if ((Get-StreHash $path) -ne $before) { throw 'Read-only inspection changed the plugin.' }
            return @{ Info = $info; Masters = $masters; Records = $records; Sha256 = $before }
        }
        'DryRun' { return New-StreSession $Config $Plugin $Request -DryRun }
        'Prepare' { return New-StreSession $Config $Plugin $Request }
        'ImportValidated' { return Import-StreValidated $Config $Session }
        default { throw 'Unknown action.' }
    }
}

function Invoke-StreModding {
    param($Config, [string]$Action, [string]$Plugin = 'STRE_AlternateStart.esp', $Request = @{}, [string]$Session, [string]$RecordType = 'quest')
    $lock = $null
    try {
        if ($Action -in @('DryRun', 'Prepare', 'ImportValidated')) {
            $audit = Resolve-StrePath (Join-Path $Config.RepoRoot '_audit')
            $null = New-Item -ItemType Directory -Path $audit -Force
            $lock = [IO.File]::Open((Join-Path $audit 'modding-operation.lock'), 'OpenOrCreate', 'ReadWrite', 'None')
        }
        Invoke-StreModdingCore @PSBoundParameters
    }
    finally { if ($null -ne $lock) { $lock.Dispose() } }
}

Export-ModuleMember -Function Resolve-StreConfiguration, Invoke-StreModding
