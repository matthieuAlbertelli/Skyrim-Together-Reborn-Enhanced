#requires -Version 7.2
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Native build + installed-A content successor to the lot-4 local wrapper. No CK import, app launch,
# remote command, credentials, server configuration write or Vortex API.
$script:RootFiles = @(
    'SkyrimTogether.exe','EarlyLoad.dll','TPProcess.exe','SkyrimTogetherServer.exe',
    'STServer.dll','libcef.dll','discord_game_sdk.dll','chrome_100_percent.pak',
    'chrome_200_percent.pak','chrome_elf.dll','d3dcompiler_47.dll','dxcompiler.dll',
    'dxil.dll','icudtl.dat','libEGL.dll','libGLESv2.dll','resources.pak',
    'v8_context_snapshot.bin','vk_swiftshader.dll','vulkan-1.dll'
)
$script:Required = $script:RootFiles + @('UI/index.html')
function Test-R17SourceBranch([string]$Branch) {
    # Repository work-branch conventions, including Codex task branches.
    $Branch -ceq 'main' -or $Branch -cmatch '^(feat|fix|refactor|docs|test|chore|art|audio|codex)/[A-Za-z0-9][A-Za-z0-9._/-]*$'
}

function Get-R17Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Read-R17Json([string]$Path) { Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json -AsHashtable }
function Write-R17Json($Value, [string]$Path) {
    # Receipts are local, new files or replaced through a fresh temporary name.
    $temp = "$Path.$([guid]::NewGuid().ToString('N')).tmp"
    $Value | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $temp -Encoding utf8
    Move-Item -LiteralPath $temp -Destination $Path -Force
}
function Assert-R17Local([string]$Path) {
    if (!$Path -or ![IO.Path]::IsPathFullyQualified($Path) -or $Path.StartsWith('\\') -or $Path.StartsWith('//')) {
        throw "A fully qualified local path is required: $Path"
    }
    $drive = Get-PSDrive -Name ([IO.Path]::GetPathRoot($Path).TrimEnd(':','\')) -ErrorAction Stop
    if ($drive.DisplayRoot -like '\\*') { throw 'Mapped network drives are not local receipt/source paths.' }
}
function Get-R17Child([string]$Root, [string]$Relative) {
    if (!$Relative -or [IO.Path]::IsPathRooted($Relative) -or $Relative -match '[:*?"<>|]' ) { throw "Unsafe relative path: $Relative" }
    foreach ($part in ($Relative -split '[/\\]')) {
        if (!$part -or $part -in @('.','..') -or $part -match '[. ]$' -or $part -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)') { throw "Unsafe relative path: $Relative" }
    }
    [IO.Path]::GetFullPath((Join-Path $Root $Relative))
}
function Assert-R17Ordinary([string]$Path, [switch]$AllowMissing) {
    $cursor = [IO.Path]::GetFullPath($Path)
    $first = $true
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse path refused: $cursor" }
        } elseif ($first -and !$AllowMissing) { throw "Missing or inaccessible path: $Path" }
        $first = $false
        $parent = Split-Path -Parent $cursor
        if ($parent -eq $cursor) { break }
        $cursor = $parent
    }
}
function Get-R17Tree([string]$Root) {
    Assert-R17Ordinary $Root
    # Walk each level only after checking it; never recurse through a junction.
    foreach ($item in Get-ChildItem -LiteralPath $Root -Force) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse entry refused: $($item.FullName)" }
        if ($item.PSIsContainer) { Get-R17Tree $item.FullName } else { $item }
    }
}
function Assert-R17NativePath([string]$Relative) {
    $null = Get-R17Child 'C:\native' $Relative
    $name = $Relative.Replace('\','/')
    if ($name -match '\.(esp|esm|esl|psc|pex|fuz|xwm|ess|skse|ini|log|db|sqlite|sqlite3|dmp)$' -or
        $name -match '(?i)(^|/)(config|configs|saves|campaigns|identities|logs|cache|credentials|tokens)(/|\.)' -or
        ($name -match '\.wav$' -and $name -notmatch '^UI/assets/sounds/') -or
        $name -match '(?i)(BetterGrabbing|skse64|versionlib-|Tests\.exe)') { throw "Protected/non-payload file: $name" }
    if ($name -notin $script:RootFiles -and $name -notmatch '^(UI|assets|locales)/') { throw "Unreviewed native package path: $name" }
}
function Assert-R17Rows($Rows, [switch]$Native) {
    $seen = @{}
    if (@($Rows).Count -eq 0) { throw 'Empty manifest.' }
    foreach ($row in $Rows) {
        $null = Get-R17Child 'C:\manifest' $row.path
        $key = $row.path.Replace('\','/')
        if ($seen.ContainsKey($key)) { throw "Duplicate manifest path: $key" }
        $seen[$key] = $true
        if ($row.sha256 -notmatch '^[a-fA-F0-9]{64}$' -or $row.bytes -isnot [ValueType] -or $row.bytes -lt 0) { throw "Invalid manifest row: $key" }
        if ($Native) { Assert-R17NativePath $key }
    }
}
function Assert-R17Files([string]$Root, $Rows) {
    Assert-R17Rows $Rows
    foreach ($row in $Rows) {
        $path = Get-R17Child $Root $row.path
        Assert-R17Ordinary $path
        $actual=Get-R17FileState $Root $row.path 'required reference file'
        if (!(Test-R17SameFile $row $actual)) { throw "Hash/size mismatch: $path; reference bytes=$($row.bytes) SHA256=$($row.sha256); observed=$($actual.presence) bytes=$($actual.bytes) SHA256=$($actual.sha256)" }
    }
}
function Get-R17Rows([string]$Root, [switch]$Distribution) {
    $files = if ($Distribution) { @(Get-R17Tree $Root) } else {
        Assert-R17Ordinary $Root
        @(foreach ($item in Get-ChildItem -LiteralPath $Root -Force) {
            if ($item.Name -in @('UI','assets','locales')) { Get-R17Tree $item.FullName }
            elseif ($item.Name -in $script:RootFiles -or $item.Extension -in @('.exe','.dll','.pak','.bin','.dat')) {
                if ($item.PSIsContainer) { throw "Unexpected payload directory: $($item.Name)" }; $item
            }
        })
    }
    @(foreach ($file in $files | Sort-Object FullName) {
        if ($Distribution -and ($file.Extension -in @('.pdb','.ilk','.exp','.lib') -or $file.Name -like '*Tests.exe')) { continue }
        Assert-R17Ordinary $file.FullName
        $rel = [IO.Path]::GetRelativePath($Root,$file.FullName).Replace('\','/')
        Assert-R17NativePath $rel
        @{path=$rel; bytes=$file.Length; sha256=Get-R17Hash $file.FullName}
    })
}
function Assert-R17Exact([string]$Root, $Rows, [switch]$Distribution) {
    Assert-R17Rows $Rows -Native
    Assert-R17Files $Root $Rows
    $actual = @(Get-R17Rows $Root -Distribution:$Distribution)
    if (@(Compare-Object @($Rows.path | Sort-Object) @($actual.path | Sort-Object)).Count) { throw "Payload file set mismatch: $Root" }
}
function Invoke-R17Git([string]$Repo, [string[]]$Arguments) {
    $value = @(& git -C $Repo @Arguments)
    if ($LASTEXITCODE) { throw "Git failed: $($Arguments -join ' ')" }
    $value
}
function Get-R17Source([string]$Repo, [string]$ExpectedCommit) {
    Assert-R17Local $Repo
    Assert-R17Ordinary $Repo
    $top = (Invoke-R17Git $Repo @('rev-parse','--show-toplevel')) -join ''
    if ([IO.Path]::GetFullPath($top).TrimEnd('\') -ne [IO.Path]::GetFullPath($Repo).TrimEnd('\')) { throw 'Selected path is not the repository root.' }
    $origin = (Invoke-R17Git $Repo @('remote','get-url','origin')) -join ''
    if ($origin -notmatch '^(https://github.com/|git@github.com:)matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced(\.git)?$') { throw 'Unexpected source repository origin.' }
    $branch = (Invoke-R17Git $Repo @('branch','--show-current')) -join ''
    if (!(Test-R17SourceBranch $branch)) { throw 'Wrong source branch: select main or a governed work branch (no detached HEAD).' }
    $head = (Invoke-R17Git $Repo @('rev-parse','HEAD')) -join ''
    if ($ExpectedCommit -and ($ExpectedCommit -notmatch '^[a-f0-9]{40}$' -or $head -cne $ExpectedCommit)) { throw 'Source commit mismatch.' }
    if ($branch -ceq 'main') {
        # Local reference only: the operator must explicitly fetch before use.
        try { $main = (Invoke-R17Git $Repo @('rev-parse','--verify','refs/remotes/origin/main^{commit}')) -join '' }
        catch { throw 'Source main requires a valid origin/main commit; fetch origin explicitly before use.' }
        if ($head -cne $main) { throw 'Source main HEAD must equal origin/main exactly; ahead/behind/divergent checkouts are refused.' }
    }
    if (@(Invoke-R17Git $Repo @('status','--porcelain=v1','--untracked-files=all')).Count) { throw 'Commit or isolate all source changes before building.' }
    $compat = Get-Content -LiteralPath (Join-Path $Repo 'Code/common/Runtime/Compatibility.h') -Raw
    $template = Get-Content -LiteralPath (Join-Path $Repo 'Code/common/BuildInfo.h.in') -Raw
    if ($compat -notmatch 'TargetVersion\s*\{\s*1,\s*7,\s*104,\s*0\s*\}' -or $compat -notmatch 'skse64_1_7_104\.dll' -or $compat -notmatch 'RequiredSkseFileVersion\s*\{\s*0,\s*2,\s*3,\s*1\s*\}' -or $template -notmatch 'runtime17-\$\{GIT_COMMIT_LONG\}') { throw 'Wrong runtime/build identity contract.' }
    $modules = @(foreach ($line in Invoke-R17Git $Repo @('ls-files','--stage')) {
        if ($line -match '^160000 ([a-f0-9]{40}) 0\t(.+)$') {
            $sha = $Matches[1]; $relative = $Matches[2]
            $path = Get-R17Child $Repo $relative
            Assert-R17Ordinary $path
            $actual = (Invoke-R17Git $path @('rev-parse','HEAD')) -join ''
            if ($actual -ne $sha -or @(Invoke-R17Git $path @('status','--porcelain=v1','--untracked-files=all')).Count) { throw "Submodule commit/cleanliness mismatch: $relative" }
            $expectedUrl = (Invoke-R17Git $Repo @('config','-f','.gitmodules',"submodule.$relative.url")) -join ''
            $url = (Invoke-R17Git $path @('remote','get-url','origin')) -join ''
            $normalize = { param($s) ($s -replace '^git@github.com:', 'https://github.com/' -replace '\.git$','').TrimEnd('/') }
            if (!$expectedUrl -or (& $normalize $url) -ne (& $normalize $expectedUrl)) { throw "Submodule origin mismatch: $relative" }
            @{path=$relative; commit=$sha; origin=$url}
        }
    })
    if (!$modules.Count) { throw 'Missing submodule provenance.' }
    @{repository=$Repo; origin=$origin; branch=$branch; commit=$head; identity="runtime17-$head"; submodules=$modules; runtime='1.7.104.0'}
}
function Get-R17DirectoryId([string]$Path) {
    if (!('StreRuntime17FileId' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
public static class StreRuntime17FileId {
 [StructLayout(LayoutKind.Sequential)] struct Info {
  public uint Attr; public System.Runtime.InteropServices.ComTypes.FILETIME C,A,W;
  public uint Volume,SizeHigh,SizeLow,Links,IndexHigh,IndexLow;
 }
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern SafeFileHandle CreateFileW(string p,uint a,uint s,IntPtr sa,uint d,uint f,IntPtr t);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool GetFileInformationByHandle(SafeFileHandle h,out Info i);
 public static string Read(string p) {
  using(var h=CreateFileW(p,0,7,IntPtr.Zero,3,0x02000000,IntPtr.Zero)) {
   Info i; if(h.IsInvalid || !GetFileInformationByHandle(h,out i)) throw new Win32Exception(Marshal.GetLastWin32Error());
   return i.Volume.ToString("X8")+":"+i.IndexHigh.ToString("X8")+i.IndexLow.ToString("X8");
  }
 }
}
'@
    }
    [StreRuntime17FileId]::Read($Path)
}
function Assert-R17Separate($Paths) {
    $full = @($Paths | ForEach-Object { [IO.Path]::GetFullPath($_).TrimEnd('\','/') })
    for ($i=0; $i -lt $full.Count; $i++) {
        for ($j=0; $j -lt $i; $j++) {
            if ($full[$i] -eq $full[$j] -or $full[$i].StartsWith($full[$j]+'\',[StringComparison]::OrdinalIgnoreCase) -or $full[$j].StartsWith($full[$i]+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Overlapping/aliased destinations.' }
        }
    }
    $ids = @{}
    foreach ($path in $full) {
        Assert-R17Ordinary $path
        if (!(Test-Path -LiteralPath $path -PathType Container)) { throw "Not a directory: $path" }
        $id = Get-R17DirectoryId $path
        if ($ids.ContainsKey($id)) { throw 'Physical destination alias detected.' }
        $ids[$id] = $path
    }
}
function Read-R17Config([string]$Path) {
    Assert-R17Local $Path
    Assert-R17Ordinary $Path
    $config = Read-R17Json $Path
    if ($config.schema -ne 1 -or @($config.targets).Count -ne 2 -or ($config.targets.id -join ',') -cne 'A,B') { throw 'Configuration requires schema 1 and ordered targets A,B.' }
    foreach ($key in @('repository','receipts','referencePath')) { Assert-R17Local $config[$key] }
    # Optional for reading older receipts/configs; new builds always allocate here.
    if (!$config.ContainsKey('buildRoot')) { $config.buildRoot=Join-Path ([IO.Path]::GetPathRoot($config.repository)) 'STRE-R17' }
    Assert-R17Local $config.buildRoot
    if ($config.referenceSha256 -notmatch '^[a-fA-F0-9]{64}$') { throw 'Pin the accepted reference SHA-256 in local configuration.' }
    foreach ($target in $config.targets) {
        if (!$target.stagingRoot -or ![IO.Path]::IsPathFullyQualified($target.stagingRoot)) { throw "Missing absolute stagingRoot for $($target.id)." }
        if ($target.gameRoot -and ![IO.Path]::IsPathFullyQualified($target.gameRoot)) { throw "Invalid gameRoot for $($target.id)." }
    }
    $config
}
function Assert-R17Content([string]$Root, $Rows, [switch]$Repository) {
    $expected = @(foreach ($row in $Rows) {
        @{path=$row.path; bytes=$(if ($Repository) {$row.repositoryBytes} else {$row.bytes}); sha256=$(if ($Repository) {$row.repositorySha256} else {$row.sha256})}
    })
    Assert-R17Files $Root $expected
}
function Get-R17Reference($Config) {
    Assert-R17Ordinary $Config.referencePath
    if ((Get-R17Hash $Config.referencePath) -ne $Config.referenceSha256) { throw 'Accepted reference manifest changed.' }
    $ref = Read-R17Json $Config.referencePath
    if ($ref.schema -ne 1 -or $ref.runtime -ne '1.7.104.0' -or $ref.contentFiles.Count -eq 0) { throw 'Invalid accepted reference.' }
    Assert-R17Rows $ref.nativeFiles -Native
    Assert-R17Rows $ref.dependencies
    Assert-R17Rows $ref.contentFiles
    foreach ($row in $ref.contentFiles) { Assert-R17ContentPath $row.path }
    $tracked = @(Invoke-R17Git $Config.repository @('ls-files','--','GameFiles/Skyrim') | ForEach-Object { $_.Substring('GameFiles/Skyrim/'.Length) })
    if (@(Compare-Object @($tracked | Sort-Object) @($ref.contentFiles.path | Sort-Object)).Count) { throw 'Content source inventory changed; requalification required.' }
    $ref
}
function Get-R17IniAttributes([string]$Path) { [IO.File]::GetAttributes($Path) }
function Read-R17IniBytes([string]$Path) { ,([IO.File]::ReadAllBytes($Path)) }
function Get-R17IniState([string]$Path, [switch]$Required) {
    # File.Exists/Test-Path can hide access failures as false. Only explicit
    # file/directory-not-found exceptions establish absence; all other IO fails.
    $Path = [IO.Path]::GetFullPath($Path)
    $cursor = $Path
    $root = [IO.Path]::GetPathRoot($Path).TrimEnd('\','/')
    $missing = $false
    while ($cursor) {
        try {
            $attributes = Get-R17IniAttributes $cursor
            if ($attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse configuration path refused: $cursor" }
            if ($cursor -eq $Path -and ($attributes -band [IO.FileAttributes]::Directory)) { throw "Server configuration is not a file: $Path" }
            if ($cursor -ne $Path -and !($attributes -band [IO.FileAttributes]::Directory)) { throw "Server configuration ancestor is not a directory: $cursor" }
        } catch [IO.FileNotFoundException] { $missing = $true
        } catch [IO.DirectoryNotFoundException] { $missing = $true
        } catch [UnauthorizedAccessException] { throw "Access denied inspecting server configuration: $cursor"
        } catch [IO.IOException] { throw "IO failure inspecting server configuration: $cursor" }
        if ($cursor.TrimEnd('\','/') -eq $root) { break }
        $parent = Split-Path -Parent $cursor
        if ($parent -eq $cursor) { break }
        $cursor = $parent
    }
    if ($missing) {
        if ($Required) { throw "Required installed server configuration is absent: $Path" }
        return @{path=$Path; presence='absent'; sha256=$null}
    }
    try { $bytes = Read-R17IniBytes $Path
    } catch [UnauthorizedAccessException] { throw "Access denied reading server configuration: $Path"
    } catch [IO.IOException] { throw "IO failure reading server configuration (unreadable or changed during inspection): $Path" }
    # Inspect policy and hash the same bytes, without exposing or copying the INI.
    $reader = [IO.StreamReader]::new([IO.MemoryStream]::new($bytes), [Text.Encoding]::UTF8, $true)
    try { $iniText = $reader.ReadToEnd() } finally { $reader.Dispose() }
    if ([regex]::Matches($iniText, '(?m)^\s*sRequiredNativePlugins\s*=\s*BetterGrabbing\.dll\s*$').Count -ne 1) { throw "Required native plugin policy mismatch: $Path" }
    @{path=$Path; presence='present'; sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()}
}
function Get-R17ProtectedState($Config, $Reference) {
    $state = @(foreach ($target in $Config.targets) {
        $game = $target.gameRoot; $stage = $target.stagingRoot
        Assert-R17Files $game $Reference.dependencies
        $plugins = @(Get-ChildItem -LiteralPath (Join-Path $game 'Data/SKSE/Plugins') -Filter '*.dll' -File -Force)
        if ($plugins.Count -ne 1 -or $plugins[0].Name -ine 'BetterGrabbing.dll' -or (Test-Path -LiteralPath (Join-Path $game 'winhttp.dll'))) { throw "Unqualified native plugin/proxy set on $($target.id)." }
        # DediRunner uses config/STServer.ini. This is preserved runtime state,
        # never a native package requirement. Only the installed copy is needed
        # to attest the effective policy; staging may legitimately have no copy.
        Get-R17IniState (Join-Path $stage 'SkyrimTogetherReborn/config/STServer.ini')
        Get-R17IniState (Join-Path $game 'Data/SkyrimTogetherReborn/config/STServer.ini') -Required
    })
    $state
}
function Assert-R17ContentPath([string]$Relative) {
    $null = Get-R17Child 'C:\content' $Relative
    $path = $Relative.Replace('\','/')
    # The pinned manifest is the exact scope, these categories are a second fence.
    # The two main-menu INIs are shipped presentation assets, not server settings.
    if ($path -notmatch '^(SkyrimTogether\.esp|STRE_AlternateStart\.esp|(?:scripts|Source)/.+\.(?:pex|psc)|Sound/Voice/(?:STRE_AlternateStart|SkyrimTogether)\.esp/.+\.wav|meshes/.+\.nif|textures/.+\.dds|SEQ/.+\.seq|SkyrimTogetherRebornBehaviors/.+\.txt|STRE/Licenses/[^/]+\.txt|STRE/MainMenu/(?:[^/]+\.mp4|[^/]+\.png|Branding/[^/]+\.png|localization\.ini|presentation\.ini))$') {
        throw "Protected/unreviewed managed content path: $path"
    }
}
function Get-R17FileAttributes([string]$Path) { [IO.File]::GetAttributes($Path) }
function Get-R17FileState([string]$Root, [string]$Relative, [string]$Phase='inspect') {
    $path = Get-R17Child $Root $Relative
    $cursor = $path; $missing = $false
    $volume = [IO.Path]::GetPathRoot($path).TrimEnd('\','/')
    while ($cursor) {
        try {
            $attr = Get-R17FileAttributes $cursor
            if ($attr -band [IO.FileAttributes]::ReparsePoint) { throw "[$Phase] Reparse path refused: $cursor" }
            if ($cursor -eq $path -and ($attr -band [IO.FileAttributes]::Directory)) { throw "[$Phase] Expected file, found directory: $path" }
            if ($cursor -ne $path -and !($attr -band [IO.FileAttributes]::Directory)) { throw "[$Phase] Ancestor is not a directory: $cursor" }
        } catch [IO.FileNotFoundException] { $missing=$true
        } catch [IO.DirectoryNotFoundException] { $missing=$true
        } catch [UnauthorizedAccessException] { throw "[$Phase] Access denied inspecting: $cursor"
        } catch [IO.IOException] { throw "[$Phase] IO failure inspecting: $cursor" }
        if ($cursor.TrimEnd('\','/') -eq $volume) { break }
        $cursor = Split-Path -Parent $cursor
    }
    if ($missing) { return @{path=$Relative; presence='absent'; bytes=$null; sha256=$null} }
    try {
        $item = Get-Item -LiteralPath $path -Force
        $bytes = $item.Length; $stamp = $item.LastWriteTimeUtc.Ticks
        $hash = Get-R17Hash $path
        $item.Refresh()
        if ($item.Length -ne $bytes -or $item.LastWriteTimeUtc.Ticks -ne $stamp) { throw "[$Phase] File changed during inspection: $path" }
        @{path=$Relative; presence='present'; bytes=$bytes; sha256=$hash; writeTicks=$stamp}
    } catch [UnauthorizedAccessException] { throw "[$Phase] Access denied reading: $path"
    } catch [IO.IOException] { throw "[$Phase] IO failure reading (unreadable or changed): $path" }
}
function Test-R17SameFile($Expected, $Actual) {
    $presence = if ($Expected.ContainsKey('presence')) { $Expected.presence } else { 'present' }
    $presence -eq $Actual.presence -and ($presence -eq 'absent' -or ($Expected.bytes -eq $Actual.bytes -and $Expected.sha256 -eq $Actual.sha256))
}
function Assert-R17State([string]$Root, $Rows, [string]$Phase) {
    foreach ($row in $Rows) {
        $actual = Get-R17FileState $Root $row.path $Phase
        if (!(Test-R17SameFile $row $actual)) {
            $presence = if ($row.ContainsKey('presence')) {$row.presence} else {'present'}
            throw "[$Phase] Hash/size/presence mismatch: $(Get-R17Child $Root $row.path); reference=$presence bytes=$($row.bytes) SHA256=$($row.sha256); observed=$($actual.presence) bytes=$($actual.bytes) SHA256=$($actual.sha256)"
        }
    }
}
function Copy-R17IndependentFile([string]$Source, [string]$Destination) {
    # Both handles are owned here. CreateNew cannot overwrite an inode/hardlink;
    # FileShare.Read prevents concurrent writes/deletes while copying the source.
    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    $inputStream=$null; $outputStream=$null
    try {
        $inputStream=[IO.File]::Open($Source,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
        $outputStream=[IO.File]::Open($Destination,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
        $inputStream.CopyTo($outputStream)
    } catch [UnauthorizedAccessException] { throw "Access denied copying: $Source -> $Destination"
    } catch [IO.IOException] { throw "IO failure copying: $Source -> $Destination; $($_.Exception.Message)"
    } finally {
        if ($outputStream) { $outputStream.Dispose() }
        if ($inputStream) { $inputStream.Dispose() }
    }
}
function Get-R17DeploymentRows($Candidate) {
    Assert-R17Rows $Candidate.files -Native
    Assert-R17Rows $Candidate.contentFiles
    $rows = @(foreach ($row in $Candidate.files) {
        @{path="SkyrimTogetherReborn/$($row.path)"; bytes=$row.bytes; sha256=$row.sha256; provenance=$(if ($Candidate.source.branch -ceq 'main') {'main checkout build'} elseif ($Candidate.source.branch -ceq 'feat/skyrim-1.7-runtime') {'experimental worktree build'} else {'development checkout build'})}
    }; foreach ($row in $Candidate.contentFiles) {
        Assert-R17ContentPath $row.path
        if ($row.provenance -cne 'installed main PC' -or ![IO.Path]::IsPathFullyQualified($row.sourcePath)) { throw "Invalid content provenance: $($row.path)" }
        $row
    })
    Assert-R17Rows $rows # Includes case-insensitive collisions between categories.
    $rows
}
function Assert-R17Package([string]$Package, $Rows) {
    Assert-R17Rows $Rows
    Assert-R17State $Package $Rows 'frozen candidate integrity'
    $actual = @(Get-R17Tree $Package | ForEach-Object { [IO.Path]::GetRelativePath($Package,$_.FullName).Replace('\','/') })
    if (@(Compare-Object @($Rows.path | Sort-Object) @($actual | Sort-Object)).Count) { throw 'Frozen candidate file set mismatch.' }
}
function Add-R17InstalledContent($Candidate, $Config, $Reference, [string]$Run) {
    Assert-R17Local $Config.targets[0].gameRoot
    $data = Join-Path $Config.targets[0].gameRoot 'Data'
    # Only main-PC source and local build/output are inspected in this phase.
    Assert-R17Separate @($data,$Run)
    $package = Join-Path $Run 'package'
    New-Item -ItemType Directory -Path $package | Out-Null
    $before = @(foreach ($row in $Reference.contentFiles) {
        Assert-R17ContentPath $row.path
        $state = Get-R17FileState $data $row.path 'installed main PC snapshot source'
        if ($state.presence -ne 'present') { throw "[snapshot] Required installed main PC source absent: $(Get-R17Child $data $row.path); no Git fallback." }
        $state
    })
    $contents = @(foreach ($row in $before) {
        $sourcePath = Get-R17Child $data $row.path
        Copy-R17IndependentFile $sourcePath (Get-R17Child $package $row.path)
        Assert-R17State $package @($row) 'snapshot copy from installed main PC'
        @{path=$row.path; sourcePath=$sourcePath; bytes=$row.bytes; sha256=$row.sha256; provenance='installed main PC'}
    })
    # Recheck the whole source set, not only the last captured file.
    foreach ($row in $before) {
        $after = Get-R17FileState $data $row.path 'installed main PC snapshot stability'
        if (!(Test-R17SameFile $row $after) -or $row.writeTicks -ne $after.writeTicks) { throw "[snapshot] Installed main PC source changed during capture: $(Get-R17Child $data $row.path); expected SHA256=$($row.sha256), observed=$($after.sha256)." }
    }
    foreach ($row in $Candidate.files) {
        Copy-R17IndependentFile (Get-R17Child (Join-Path $Run 'distrib/bin') $row.path) (Get-R17Child $package "SkyrimTogetherReborn/$($row.path)")
    }
    $Candidate.schema=2; $Candidate.contentFiles=$contents
    $Candidate.capturedUtc=[datetime]::UtcNow; $Candidate.contentSourceRoot=$data
    $Candidate.scopeReferenceSha256=$Config.referenceSha256
    Assert-R17Package $package @(Get-R17DeploymentRows $Candidate)
    $Candidate
}
function Get-R17Plan([string]$Root, $Rows) {
    @(foreach ($row in $Rows) {
        $state=Get-R17FileState $Root $row.path 'destination preflight'
        $state.action=if ($state.presence -eq 'absent') {'create'} elseif (Test-R17SameFile $row $state) {'identical'} else {'replace'}
        $state
    })
}
function Get-R17Preflight($Config, $Reference, $Rows) {
    foreach ($target in $Config.targets) {
        if (!$target.gameRoot) { throw "Configure the actual gameRoot for $($target.id); staging is not an installation." }
    }
    $paths = @($Config.repository,$Config.receipts,$Config.buildRoot)
    foreach ($target in $Config.targets) { $paths += $target.stagingRoot; $paths += $target.gameRoot }
    # Receipt storage must be outside the selected source checkout and both PCs.
    Assert-R17Separate $paths
    $protected = @(Get-R17ProtectedState $Config $Reference)
    $targets = @(foreach ($target in $Config.targets) {
        $stage = $target.stagingRoot
        $installed = Join-Path $target.gameRoot 'Data'
        Assert-R17Separate @($stage,$installed)
        if (!$Rows) { throw 'A frozen synchronization manifest is required for destination preflight.' }
        @{id=$target.id; staging=$stage; installed=$installed;
          stagingId=Get-R17DirectoryId $stage; installedId=Get-R17DirectoryId $installed;
          stagingBefore=@(Get-R17Plan $stage $Rows); installedBefore=@(Get-R17Plan $installed $Rows)}
    })
    @{targets=$targets; protected=$protected}
}
function Assert-R17Stopped {
    . (Join-Path $PSScriptRoot 'Stre.ProcessGuard.ps1')
    Assert-GameProcessesStopped
    if (Get-Process -Name steam -ErrorAction SilentlyContinue) { throw 'Close Steam before running this workflow.' }
}
function New-R17BuildLayout($Config, [string]$Run, $Source) {
    Assert-R17Local $Config.buildRoot
    Assert-R17Ordinary $Config.buildRoot
    $root=Join-Path $Config.buildRoot ([guid]::NewGuid().ToString('N'))
    $build=Join-Path $root 'build'
    # Conservative budget for this project's objects/PCH/dependency suffixes.
    # MSVC also has limits before normalizing relative paths with .. components.
    if ($build.Length -gt 100) { throw "Short compiler build directory required (maximum 100 characters): $build" }
    New-Item -ItemType Directory -Path $root | Out-Null
    $layout=@{schema=1; root=$root; rootId=Get-R17DirectoryId $root;
        buildDirectory=$build; receiptRoot=[IO.Path]::GetFullPath($Run);
        repository=$Source.repository; sourceCommit=$Source.commit; utc=[datetime]::UtcNow}
    Write-R17Json $layout (Join-Path $root 'receipt-link.json')
    Write-R17Json $layout (Join-Path $Run 'build-layout.json')
    $layout
}
function Get-R17BuildDirectory([string]$Run, $Source) {
    $path=Join-Path $Run 'build-layout.json'
    if (!(Test-Path -LiteralPath $path)) { return (Join-Path $Run 'build') } # Retained pre-fix candidate only.
    Assert-R17Ordinary $path
    $layout=Read-R17Json $path
    Assert-R17Local $layout.root
    Assert-R17Ordinary $layout.root
    if ($layout.schema -ne 1 -or $layout.receiptRoot -ne [IO.Path]::GetFullPath($Run) -or
        $layout.buildDirectory -ne (Join-Path $layout.root 'build') -or $layout.buildDirectory.Length -gt 100 -or
        (Get-R17DirectoryId $layout.root) -ne $layout.rootId) { throw 'Build/receipt path identity mismatch.' }
    if ($Source -and ($layout.repository -ne $Source.repository -or $layout.sourceCommit -ne $Source.commit)) { throw 'Build/receipt source commit mismatch.' }
    Assert-R17Separate @($Run,$layout.root)
    $linkPath=Join-Path $layout.root 'receipt-link.json'
    Assert-R17Ordinary $linkPath
    $link=Read-R17Json $linkPath
    foreach ($key in @('schema','root','rootId','buildDirectory','receiptRoot','repository','sourceCommit')) {
        if ($link[$key] -ne $layout[$key]) { throw 'Build/receipt reverse link mismatch.' }
    }
    $layout.buildDirectory
}
function Invoke-R17Build([string]$Repo, [string]$Run, [int]$Jobs) {
    if (!(Test-Path -LiteralPath (Join-Path $Run 'build-layout.json'))) { throw 'Allocate a short build directory before compilation.' }
    $head=(Invoke-R17Git $Repo @('rev-parse','HEAD')) -join ''
    $build=Get-R17BuildDirectory $Run @{repository=$Repo;commit=$head}
    $commands=@(
        @('config','-P','.','-m','debug','-y','-o',$build),
        @('build','-P','.','-y','-j',"$Jobs"),
        @('install','-P','.','-o',(Join-Path $Run 'distrib'))
    )
    Write-R17Json @{repository=$Repo;commit=$head;buildDirectory=$build;commands=$commands} (Join-Path $Run 'build-commands.json')
    # Reuse lot-4 configure/build/install, always in the explicitly selected repo.
    $oldPin=$env:COREPACK_ENABLE_AUTO_PIN; $oldLatest=$env:COREPACK_DEFAULT_TO_LATEST
    Push-Location -LiteralPath $Repo
    try {
        $env:COREPACK_ENABLE_AUTO_PIN='0'; $env:COREPACK_DEFAULT_TO_LATEST='0'
        foreach ($argsList in $commands) {
            & xmake @argsList 2>&1 | Tee-Object -FilePath (Join-Path $Run 'build.log') -Append | Out-Host
            if ($LASTEXITCODE) { throw "xmake $($argsList[0]) failed: $LASTEXITCODE" }
        }
    } finally { Pop-Location; $env:COREPACK_ENABLE_AUTO_PIN=$oldPin; $env:COREPACK_DEFAULT_TO_LATEST=$oldLatest }
}
function New-R17Candidate($Source, [string]$Run) {
    $bin = Join-Path $Run 'distrib/bin'
    $header = Get-Content -LiteralPath (Join-Path (Get-R17BuildDirectory $Run $Source) 'BuildInfo.h') -Raw
    if (!$header.Contains($Source.identity)) { throw 'Stale generated build identity.' }
    foreach ($name in $script:Required) {
        if (!(Test-Path -LiteralPath (Join-Path $bin $name) -PathType Leaf)) { throw "Missing required artifact: $name" }
    }
    foreach ($name in @('SkyrimTogether.exe','STServer.dll')) {
        if (![Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Join-Path $bin $name))).Contains($Source.identity)) { throw "Stale binary identity: $name" }
    }
    @{schema=1; source=$Source; files=@(Get-R17Rows $bin -Distribution)}
}
function Backup-R17Target($Target, [string]$Run) {
    foreach ($kind in @('staging','installed')) {
        $backup = Join-Path $Run "backup/$($Target.id)/$kind"
        New-Item -ItemType Directory -Path $backup | Out-Null
        Assert-R17State $Target[$kind] $Target["${kind}Before"] "backup $($Target.id) $kind source"
        foreach ($row in $Target["${kind}Before"]) {
            if ($row.presence -eq 'absent') { continue }
            $dest = Get-R17Child $backup $row.path
            Copy-R17IndependentFile (Get-R17Child $Target[$kind] $row.path) $dest
        }
        Assert-R17State $backup $Target["${kind}Before"] "backup $($Target.id) $kind integrity"
    }
}
function Set-R17Payload([string]$Destination, [string]$Source, $Before, $After, [string]$JournalPath) {
    if (@(Compare-Object @($Before.path | Sort-Object) @($After.path | Sort-Object)).Count) { throw 'Copy scope differs from preflight scope.' }
    foreach ($row in $After) {
        if ($row.path -match '^SkyrimTogetherReborn/(.+)$') { Assert-R17NativePath $Matches[1] } else { Assert-R17ContentPath $row.path }
    }
    Assert-R17State $Source $After 'copy source manifest'
    Assert-R17State $Destination $Before 'destination unchanged since preflight'
    $journal=@{destination=$Destination; reference=$Source; files=@(); status='COPYING'}
    foreach ($row in $After) {
        $path=Get-R17Child $Destination $row.path
        $old=@($Before | Where-Object path -eq $row.path)[0]
        $entry=@{path=$row.path; status='PENDING'; mutationStarted=$false; observed=$null}
        $journal.files += $entry
        if ($JournalPath) { Write-R17Json $journal $JournalPath }
        try {
            Assert-R17State $Destination @($old) 'before individual replacement'
            if (Test-R17SameFile $row $old) { $entry.status='IDENTICAL' } else {
                # Only this manifest path is unlinked; no union/purge of old UI.
                if ($old.presence -eq 'present') { Remove-Item -LiteralPath $path -Force }
                $entry.mutationStarted=$true
                $entry.status='UNLINKED'
                if ($JournalPath) { Write-R17Json $journal $JournalPath }
                if (!$row.ContainsKey('presence') -or $row.presence -eq 'present') {
                    Copy-R17IndependentFile (Get-R17Child $Source $row.path) $path
                }
                Assert-R17State $Destination @($row) 'after copy against frozen manifest'
                $entry.status='VERIFIED'
            }
        } catch {
            $entry.status='FAILED'; $entry.error=$_.Exception.Message
            # Record partial bytes for guarded restoration; never accept an
            # arbitrary later edit just because a copy previously failed.
            if ($entry.mutationStarted) {
                try { $entry.observed=Get-R17FileState $Destination $row.path 'partial copy evidence' } catch { $entry.observationError=$_.Exception.Message }
            }
            $journal.status='STOPPED'
            if ($JournalPath) { Write-R17Json $journal $JournalPath }
            throw
        }
        if ($JournalPath) { Write-R17Json $journal $JournalPath }
    }
    Assert-R17State $Destination $After 'complete staging against frozen manifest'
    $journal.status='COMPLETE'
    if ($JournalPath) { Write-R17Json $journal $JournalPath }
}
function Assert-R17Protected($Config, $Reference, $Original) {
    $current = @(Get-R17ProtectedState $Config $Reference)
    if ($current.Count -ne @($Original).Count) { throw 'Protected configuration inventory changed; run a new preflight.' }
    foreach ($row in $current) {
        $old = @($Original | Where-Object { $_.path -eq $row.path -and $_.presence -eq $row.presence -and $_.sha256 -eq $row.sha256 })
        if ($old.Count -ne 1) { throw 'Protected server configuration presence/hash changed; stop and investigate.' }
    }
}
function Restore-R17Staging($Receipt, [string]$Run) {
    $candidate = @(Get-R17DeploymentRows $Receipt.candidate)
    # Validate every backup and destination before restoring either target.
    $plans = @(foreach ($target in $Receipt.preflight.targets) {
        foreach ($kind in @('staging','installed')) {
            Assert-R17State (Join-Path $Run "backup/$($target.id)/$kind") $target["${kind}Before"] "restore backup $($target.id) $kind"
        }
        # Initially divergent staging is not a safe rollback image for Data.
        # Feed the original installed bytes to Vortex; retain both backups.
        $backup = Join-Path $Run "backup/$($target.id)/installed"
        Assert-R17Ordinary $target.staging
        if ((Get-R17DirectoryId $target.staging) -ne $target.stagingId) { throw 'Restore destination identity changed.' }
        $current = @(Get-R17Plan $target.staging $candidate)
        $partial=@()
        foreach ($name in @("copy-$($target.id).json","restore-copy-$($target.id).json")) {
            $journalPath=Join-Path $Run $name
            if (Test-Path -LiteralPath $journalPath) {
                $partial += @((Read-R17Json $journalPath).files | Where-Object { $_.ContainsKey('observed') -and $null -ne $_.observed } | ForEach-Object { $_.observed })
            }
        }
        foreach ($row in $current) {
            if ($row.presence -eq 'absent') { continue }
            $allowed = @(@($target.stagingBefore)+@($target.installedBefore)+@($candidate)+$partial | Where-Object { $_.path -eq $row.path -and (Test-R17SameFile $_ $row) })
            if (!$allowed.Count) { throw "Restore would overwrite unrelated changes: $($row.path)" }
        }
        @{target=$target; backup=$backup; current=$current}
    })
    foreach ($plan in $plans) {
        $Receipt.status="RESTORING_$($plan.target.id)"
        Write-R17Json $Receipt (Join-Path $Run 'receipt.json')
        Set-R17Payload $plan.target.staging $plan.backup $plan.current $plan.target.installedBefore (Join-Path $Run "restore-copy-$($plan.target.id).json")
        Write-R17Json @{id=$plan.target.id; status='STAGING_RESTORED'; actualGameVerified=$false} (Join-Path $Run "restore-$($plan.target.id).json")
    }
}
function Assert-R17ReceiptTargets($Config, $Receipt) {
    if (@($Receipt.preflight.targets).Count -ne 2) { throw 'Invalid receipt targets.' }
    $paths = @()
    foreach ($target in $Config.targets) {
        $saved = @($Receipt.preflight.targets | Where-Object id -eq $target.id)
        if ($saved.Count -ne 1 -or $saved[0].staging -ne $target.stagingRoot -or $saved[0].installed -ne (Join-Path $target.gameRoot 'Data')) { throw 'Receipt destination mismatch.' }
        foreach ($kind in @('staging','installed')) {
            $path = $saved[0][$kind]; $paths += $path
            Assert-R17Ordinary $path
            if ((Get-R17DirectoryId $path) -ne $saved[0]["${kind}Id"]) { throw 'Destination directory identity changed.' }
        }
    }
    Assert-R17Separate $paths
}
function Invoke-R17Workflow {
    param([string]$ConfigPath, [string]$Mode='BuildDeploy', [string]$ReceiptPath,
        [string]$ExpectedCommit, [int]$Jobs=6, [switch]$PlanOnly)
    $config = Read-R17Config $ConfigPath
    if ($PlanOnly) {
        [pscustomobject]@{mode=$Mode; destinationIO=$false; repository=$config.repository;
            buildRoot=$config.buildRoot; targets=$config.targets; missingGameRoots=@($config.targets | Where-Object {!$_.gameRoot} | ForEach-Object { $_.id });
            steps='build fresh; snapshot installed A content; preflight both; backup both; stage both; human Vortex deploy; verify actual Data'}
        return
    }
    foreach ($target in $config.targets) { if (!$target.gameRoot) { throw "Configure actual gameRoot for $($target.id) before execution." } }
    Assert-R17Stopped
    # Rollback uses saved installed bytes even if current source is dirty or has
    # evolved. Verify still records the current documentation/source HEAD.
    $source = if ($Mode -notin @('RestoreStaging','VerifyRestored')) { Get-R17Source $config.repository $ExpectedCommit } else { $null }
    if ($source) { Write-Host "Selected source: $($source.repository); branch=$($source.branch); commit=$($source.commit)" }
    $reference = if ($Mode -ne 'RestoreStaging') { Get-R17Reference $config } else { $null }
    if ($Mode -ne 'BuildDeploy') {
        Assert-R17Local $ReceiptPath
        Assert-R17Ordinary $ReceiptPath
        $receipt = Read-R17Json (Join-Path $ReceiptPath 'receipt.json')
        if ($receipt.configSha256 -ne (Get-R17Hash $ConfigPath) -or $receipt.schema -ne 2) { throw 'Receipt/configuration mismatch (schema 2 required; do not reinterpret an older native-only receipt).' }
        $rows=@(Get-R17DeploymentRows $receipt.candidate)
        $receipt.attemptId=[guid]::NewGuid().ToString('N')
        $receipt.status="${Mode}_PREFLIGHT"
        Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
        try {
        Assert-R17ReceiptTargets $config $receipt
        if ($Mode -eq 'RestoreStaging') {
            $receipt.status='RESTORE_REQUESTED'; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
            Restore-R17Staging $receipt $ReceiptPath
            $receipt.status='STAGING_RESTORED'; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
            Write-Host 'Original installed bytes prepared in staging. Human Vortex deployment is required; installed files are not declared restored.'
            return
        }
        if ($Mode -eq 'Verify') {
            $builtSource=$receipt.candidate.source
            if ($builtSource.commit -notmatch '^[a-f0-9]{40}$' -or $builtSource.identity -cne "runtime17-$($builtSource.commit)" -or !(Test-R17SourceBranch $builtSource.branch)) { throw 'Invalid retained candidate provenance.' }
            $retained=New-R17Candidate $builtSource $ReceiptPath
            Assert-R17Exact (Join-Path $ReceiptPath 'distrib/bin') $receipt.candidate.files -Distribution
            if ($retained.files.Count -ne $receipt.candidate.files.Count) { throw 'Retained manifest changed.' }
            if ($receipt.candidate.scopeReferenceSha256 -ne $config.referenceSha256 -or $receipt.candidate.contentSourceRoot -ne (Join-Path $config.targets[0].gameRoot 'Data')) { throw 'Retained content source/scope changed.' }
            if (@(Compare-Object @($reference.contentFiles.path | Sort-Object) @($receipt.candidate.contentFiles.path | Sort-Object)).Count) { throw 'Retained content scope changed.' }
            foreach ($row in $receipt.candidate.contentFiles) {
                if ($row.sourcePath -ne (Get-R17Child $receipt.candidate.contentSourceRoot $row.path)) { throw 'Retained content source path changed.' }
            }
            Assert-R17Package (Join-Path $ReceiptPath 'package') $rows
        }
        Assert-R17Protected $config $reference $receipt.preflight.protected
        if ($Mode -eq 'Verify' -and (Test-Path -LiteralPath (Join-Path $ReceiptPath 'restore-A.json'))) { throw 'Use VerifyRestored after restoration.' }
        $status = if ($Mode -eq 'VerifyRestored') { 'RESTORED_VERIFIED' } else { 'INSTALLED_VERIFIED' }
        foreach ($target in $receipt.preflight.targets) {
            $receipt.status="${Mode}_$($target.id)"; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
            $expected = if ($Mode -eq 'VerifyRestored') { $target.installedBefore } else { $rows }
            Assert-R17State $target.staging $expected "$Mode $($target.id) staging"
            Assert-R17State $target.installed $expected "$Mode $($target.id) installed Data"
            Write-R17Json @{id=$target.id; attemptId=$receipt.attemptId; status=$status; utc=[datetime]::UtcNow; candidateCommit=$receipt.candidate.source.commit; verificationHead=$(if ($source) {$source.commit} else {$null}); files=$expected} (Join-Path $ReceiptPath "verified-$($target.id).json")
        }
        $receipt.status=$status; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
        Write-Host "$status on both actual installations. No runtime or multiplayer validation is implied."
        return
        } catch {
            $receipt.failedPhase=$receipt.status; $receipt.status='STOPPED'; $receipt.error=$_.Exception.Message
            Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
            throw
        }
    }
    # No remote destination access before the fresh build and frozen candidate.
    # Check local output isolation before xmake can create any build files.
    Assert-R17Local $config.targets[0].gameRoot
    Assert-R17Separate @($config.repository,$config.receipts,$config.buildRoot,$config.targets[0].gameRoot,$config.targets[0].stagingRoot)
    $answer = Read-Host 'Both PCs: close game, Vortex, Steam, server, CK and xEdit; keep them closed through staging. Type READY'
    if ($answer -cne 'READY') { throw 'Maintainer readiness not confirmed.' }
    $run = Join-Path $config.receipts ((Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')+'-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $run | Out-Null
    $receipt = @{schema=2; status='BUILDING'; configSha256=Get-R17Hash $ConfigPath; source=$source; preflight=$null; candidate=$null}
    Write-R17Json $receipt (Join-Path $run 'receipt.json')
    Write-Host "Receipt: $run"
    try {
        $receipt.build=New-R17BuildLayout $config $run $source
        Write-R17Json $receipt (Join-Path $run 'receipt.json')
        Invoke-R17Build $config.repository $run $Jobs
        $null = Get-R17Source $config.repository $source.commit
        $receipt.candidate = New-R17Candidate $source $run
        Assert-R17Exact (Join-Path $run 'distrib/bin') $receipt.candidate.files -Distribution
        $receipt.status='SNAPSHOT_INSTALLED_A'; Write-R17Json $receipt (Join-Path $run 'receipt.json')
        $receipt.candidate = Add-R17InstalledContent $receipt.candidate $config $reference $run
        $rows=@(Get-R17DeploymentRows $receipt.candidate)
        Write-R17Json $receipt.candidate (Join-Path $run 'manifest.json')
        Assert-R17Stopped
        $receipt.status='DESTINATION_PREFLIGHT'; Write-R17Json $receipt (Join-Path $run 'receipt.json')
        $receipt.preflight = Get-R17Preflight $config $reference $rows
        foreach ($target in $receipt.preflight.targets) {
            $receipt.status="BACKUP_$($target.id)"; Write-R17Json $receipt (Join-Path $run 'receipt.json')
            Backup-R17Target $target $run
            Write-R17Json @{id=$target.id; status='BACKED_UP'; staging=$target.stagingBefore; installed=$target.installedBefore} (Join-Path $run "backup-$($target.id).json")
        }
        $receipt.status='BACKED_UP'; Write-R17Json $receipt (Join-Path $run 'receipt.json')
        foreach ($target in $receipt.preflight.targets) {
            $receipt.status="STAGING_$($target.id)"; Write-R17Json $receipt (Join-Path $run 'receipt.json')
            Assert-R17Package (Join-Path $run 'package') $rows
            Set-R17Payload $target.staging (Join-Path $run 'package') $target.stagingBefore $rows (Join-Path $run "copy-$($target.id).json")
            Write-R17Json @{id=$target.id; status='STAGED'; actualGameVerified=$false; files=$rows} (Join-Path $run "staged-$($target.id).json")
        }
        $receipt.status='AWAITING_HUMAN_VORTEX'; Write-R17Json $receipt (Join-Path $run 'receipt.json')
        $answer = Read-Host 'Deploy mods manually with Vortex on BOTH PCs, then close Vortex. Do not launch any game/server. Type DEPLOYED'
        if ($answer -cne 'DEPLOYED') { throw 'Staged only; human Vortex deployment not confirmed. Resume with Verify.' }
        Assert-R17Stopped
        Invoke-R17Workflow -ConfigPath $ConfigPath -Mode Verify -ReceiptPath $run
    } catch {
        $savedReceipt=Read-R17Json (Join-Path $run 'receipt.json')
        if ($savedReceipt.status -eq 'STOPPED') { $receipt=$savedReceipt } else {
            $receipt.failedPhase=$receipt.status; $receipt.status='STOPPED'; $receipt.error=$_.Exception.Message
        }
        Write-R17Json $receipt (Join-Path $run 'receipt.json')
        $recovery=Get-R17RecoveryMessage $receipt $run
        throw "STOP: $($_.Exception.Message) Receipt: $run. $recovery"
    }
}
function Get-R17RecoveryMessage($Receipt, [string]$Run) {
    if ($Receipt.failedPhase -match '^(STAGING_[AB]|AWAITING_HUMAN_VORTEX|Verify_)') {
        if ($Receipt.failedPhase -eq 'AWAITING_HUMAN_VORTEX' -or $Receipt.failedPhase -like 'Verify_*') {
            return 'Staging completed: finish the human Vortex deployment and use Verify, or use RestoreStaging then human Vortex and VerifyRestored.'
        }
        return 'Staging may be partial: inspect the per-PC copy journals; use RestoreStaging then human Vortex and VerifyRestored before starting a new run.'
    }
    'Stopped before staging writes. Preserve this receipt, correct the build/preparation failure and start a new execution. No destination restoration is indicated for this run.'
}
Export-ModuleMember -Function Invoke-R17Workflow
