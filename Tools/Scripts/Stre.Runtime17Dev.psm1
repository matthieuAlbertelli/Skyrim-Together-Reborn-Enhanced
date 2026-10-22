#requires -Version 7.2
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Native-only successor to the lot-4 local wrapper. No CK import, app launch,
# remote command, credentials, server configuration write or Vortex API.
$script:RootFiles = @(
    'SkyrimTogether.exe','EarlyLoad.dll','TPProcess.exe','SkyrimTogetherServer.exe',
    'STServer.dll','libcef.dll','discord_game_sdk.dll','chrome_100_percent.pak',
    'chrome_200_percent.pak','chrome_elf.dll','d3dcompiler_47.dll','dxcompiler.dll',
    'dxil.dll','icudtl.dat','libEGL.dll','libGLESv2.dll','resources.pak',
    'v8_context_snapshot.bin','vk_swiftshader.dll','vulkan-1.dll'
)
$script:Required = $script:RootFiles + @('UI/index.html')

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
    if ($name -match '\.(esp|esm|esl|psc|pex|fuz|xwm|ess|skse|ini|log)$' -or
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
        if (!(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -ne $row.bytes -or (Get-R17Hash $path) -ne $row.sha256) { throw "Hash/size mismatch: $path" }
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
    if ($branch -cne 'feat/skyrim-1.7-runtime') { throw 'Wrong source branch.' }
    $head = (Invoke-R17Git $Repo @('rev-parse','HEAD')) -join ''
    if ($ExpectedCommit -and ($ExpectedCommit -notmatch '^[a-f0-9]{40}$' -or $head -cne $ExpectedCommit)) { throw 'Source commit mismatch.' }
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
    Assert-R17Content (Join-Path $Config.repository 'GameFiles/Skyrim') $ref.contentFiles -Repository
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
        Assert-R17Content $stage $Reference.contentFiles
        Assert-R17Content (Join-Path $game 'Data') $Reference.contentFiles
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
function Get-R17Preflight($Config, $Reference) {
    foreach ($target in $Config.targets) {
        if (!$target.gameRoot) { throw "Configure the actual gameRoot for $($target.id); staging is not an installation." }
    }
    $paths = @($Config.repository,$Config.receipts)
    foreach ($target in $Config.targets) { $paths += $target.stagingRoot; $paths += $target.gameRoot }
    # Receipt storage must be outside the selected source checkout and both PCs.
    Assert-R17Separate $paths
    $protected = @(Get-R17ProtectedState $Config $Reference)
    $targets = @(foreach ($target in $Config.targets) {
        $stage = Join-Path $target.stagingRoot 'SkyrimTogetherReborn'
        $installed = Join-Path $target.gameRoot 'Data/SkyrimTogetherReborn'
        Assert-R17Separate @($stage,$installed)
        $old = @(Get-R17Rows $stage)
        # A pending or conflicting Vortex deployment must be resolved by the
        # maintainer before this transaction. This also makes rollback exact.
        Assert-R17Exact $installed $old
        @{id=$target.id; staging=$stage; installed=$installed;
          stagingId=Get-R17DirectoryId $stage; installedId=Get-R17DirectoryId $installed;
          stagingBefore=$old; installedBefore=@(Get-R17Rows $installed)}
    })
    @{targets=$targets; protected=$protected}
}
function Assert-R17Stopped {
    . (Join-Path $PSScriptRoot 'Stre.ProcessGuard.ps1')
    Assert-GameProcessesStopped
    if (Get-Process -Name steam -ErrorAction SilentlyContinue) { throw 'Close Steam before running this workflow.' }
}
function Invoke-R17Build([string]$Repo, [string]$Run, [int]$Jobs) {
    # Reuse lot-4 configure/build/install, always in the explicitly selected repo.
    $oldPin=$env:COREPACK_ENABLE_AUTO_PIN; $oldLatest=$env:COREPACK_DEFAULT_TO_LATEST
    Push-Location -LiteralPath $Repo
    try {
        $env:COREPACK_ENABLE_AUTO_PIN='0'; $env:COREPACK_DEFAULT_TO_LATEST='0'
        foreach ($argsList in @(
            @('config','-P','.','-m','debug','-y','-o',(Join-Path $Run 'build')),
            @('build','-P','.','-y','-j',"$Jobs"),
            @('install','-P','.','-o',(Join-Path $Run 'distrib'))
        )) {
            & xmake @argsList 2>&1 | Tee-Object -FilePath (Join-Path $Run 'build.log') -Append | Out-Host
            if ($LASTEXITCODE) { throw "xmake $($argsList[0]) failed: $LASTEXITCODE" }
        }
    } finally { Pop-Location; $env:COREPACK_ENABLE_AUTO_PIN=$oldPin; $env:COREPACK_DEFAULT_TO_LATEST=$oldLatest }
}
function New-R17Candidate($Source, [string]$Run) {
    $bin = Join-Path $Run 'distrib/bin'
    $header = Get-Content -LiteralPath (Join-Path $Run 'build/BuildInfo.h') -Raw
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
        foreach ($row in $Target["${kind}Before"]) {
            $dest = Get-R17Child $backup $row.path
            New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
            Copy-Item -LiteralPath (Get-R17Child $Target[$kind] $row.path) -Destination $dest
        }
        Assert-R17Files $backup $Target["${kind}Before"]
    }
}
function Set-R17Payload([string]$Destination, [string]$Source, $Before, $After) {
    Assert-R17Rows $After -Native
    Assert-R17Exact $Source $After -Distribution
    if (@($Before).Count) { Assert-R17Exact $Destination $Before }
    elseif (@(Get-R17Rows $Destination).Count) { throw 'Unexpected files at an empty destination.' }
    # Each individual unlink breaks Vortex hardlinks; no in-place overwrite.
    $oldPaths = @($Before | ForEach-Object { $_.path })
    foreach ($rel in @($oldPaths+@($After.path) | Sort-Object -Unique)) {
        $path = Get-R17Child $Destination $rel
        Assert-R17Ordinary $path -AllowMissing
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
        $row = @($After | Where-Object path -eq $rel)
        if ($row.Count) {
            New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
            Copy-Item -LiteralPath (Get-R17Child $Source $rel) -Destination $path
            if ((Get-R17Hash $path) -ne $row[0].sha256) { throw "Copied hash mismatch: $rel" }
        }
    }
    Assert-R17Exact $Destination $After
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
    $candidate = $Receipt.candidate.files
    # Validate every backup and destination before restoring either target.
    $plans = @(foreach ($target in $Receipt.preflight.targets) {
        $backup = Join-Path $Run "backup/$($target.id)/staging"
        Assert-R17Exact $backup $target.stagingBefore -Distribution
        Assert-R17Ordinary $target.staging
        if ((Get-R17DirectoryId $target.staging) -ne $target.stagingId) { throw 'Restore destination identity changed.' }
        $current = @(Get-R17Rows $target.staging)
        foreach ($row in $current) {
            $allowed = @(@($target.stagingBefore)+@($candidate) | Where-Object { $_.path -eq $row.path -and $_.sha256 -eq $row.sha256 })
            if (!$allowed.Count) { throw "Restore would overwrite unrelated changes: $($row.path)" }
        }
        @{target=$target; backup=$backup; current=$current}
    })
    foreach ($plan in $plans) {
        Set-R17Payload $plan.target.staging $plan.backup $plan.current $plan.target.stagingBefore
        Write-R17Json @{id=$plan.target.id; status='STAGING_RESTORED'; actualGameVerified=$false} (Join-Path $Run "restore-$($plan.target.id).json")
    }
}
function Assert-R17ReceiptTargets($Config, $Receipt) {
    if (@($Receipt.preflight.targets).Count -ne 2) { throw 'Invalid receipt targets.' }
    $paths = @()
    foreach ($target in $Config.targets) {
        $saved = @($Receipt.preflight.targets | Where-Object id -eq $target.id)
        if ($saved.Count -ne 1 -or $saved[0].staging -ne (Join-Path $target.stagingRoot 'SkyrimTogetherReborn') -or $saved[0].installed -ne (Join-Path $target.gameRoot 'Data/SkyrimTogetherReborn')) { throw 'Receipt destination mismatch.' }
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
            targets=$config.targets; missingGameRoots=@($config.targets | Where-Object {!$_.gameRoot} | ForEach-Object { $_.id });
            steps='preflight both; build fresh; backup both; stage both; human Vortex deploy; verify actual Data'}
        return
    }
    foreach ($target in $config.targets) { if (!$target.gameRoot) { throw "Configure actual gameRoot for $($target.id) before execution." } }
    Assert-R17Stopped
    # Rollback uses saved native bytes even if current source is dirty or has
    # evolved. Verify still records the current documentation/source HEAD.
    $source = if ($Mode -notin @('RestoreStaging','VerifyRestored')) { Get-R17Source $config.repository $ExpectedCommit } else { $null }
    $reference = if ($Mode -ne 'RestoreStaging') { Get-R17Reference $config } else { $null }
    if ($Mode -ne 'BuildDeploy') {
        Assert-R17Local $ReceiptPath
        Assert-R17Ordinary $ReceiptPath
        $receipt = Read-R17Json (Join-Path $ReceiptPath 'receipt.json')
        if ($receipt.configSha256 -ne (Get-R17Hash $ConfigPath) -or $receipt.schema -ne 1) { throw 'Receipt/configuration mismatch.' }
        Assert-R17Rows $receipt.candidate.files -Native
        Assert-R17ReceiptTargets $config $receipt
        if ($Mode -eq 'RestoreStaging') {
            $receipt.status='RESTORE_REQUESTED'; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
            Restore-R17Staging $receipt $ReceiptPath
            $receipt.status='STAGING_RESTORED'; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
            Write-Host 'Staging restored. Human Vortex deployment is required; installed files are not declared restored.'
            return
        }
        if ($Mode -eq 'Verify') {
            $builtSource=$receipt.candidate.source
            if ($builtSource.commit -notmatch '^[a-f0-9]{40}$' -or $builtSource.identity -cne "runtime17-$($builtSource.commit)" -or $builtSource.branch -cne 'feat/skyrim-1.7-runtime') { throw 'Invalid retained candidate provenance.' }
            $retained=New-R17Candidate $builtSource $ReceiptPath
            Assert-R17Exact (Join-Path $ReceiptPath 'distrib/bin') $receipt.candidate.files -Distribution
            if ($retained.files.Count -ne $receipt.candidate.files.Count) { throw 'Retained manifest changed.' }
        }
        Assert-R17Protected $config $reference $receipt.preflight.protected
        if ($Mode -eq 'Verify' -and $receipt.status -in @('RESTORE_REQUESTED','STAGING_RESTORED','RESTORED_VERIFIED')) { throw 'Use VerifyRestored after restoration.' }
        $status = if ($Mode -eq 'VerifyRestored') { 'RESTORED_VERIFIED' } else { 'INSTALLED_VERIFIED' }
        foreach ($target in $receipt.preflight.targets) {
            $expected = if ($Mode -eq 'VerifyRestored') { $target.installedBefore } else { $receipt.candidate.files }
            Assert-R17Exact $target.staging $expected
            Assert-R17Exact $target.installed $expected
            Write-R17Json @{id=$target.id; status=$status; utc=[datetime]::UtcNow; candidateCommit=$receipt.candidate.source.commit; verificationHead=$(if ($source) {$source.commit} else {$null}); files=$expected} (Join-Path $ReceiptPath "verified-$($target.id).json")
        }
        $receipt.status=$status; Write-R17Json $receipt (Join-Path $ReceiptPath 'receipt.json')
        Write-Host "$status on both actual installations. No runtime or multiplayer validation is implied."
        return
    }
    $preflight = Get-R17Preflight $config $reference
    $answer = Read-Host 'Both PCs: close game, Vortex, Steam, server, CK and xEdit; keep them closed through staging. Type READY'
    if ($answer -cne 'READY') { throw 'Maintainer readiness not confirmed.' }
    $run = Join-Path $config.receipts ((Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')+'-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $run | Out-Null
    $receipt = @{schema=1; status='BUILDING'; configSha256=Get-R17Hash $ConfigPath; source=$source; preflight=$preflight; candidate=$null}
    Write-R17Json $receipt (Join-Path $run 'receipt.json')
    Write-Host "Receipt: $run"
    try {
        Invoke-R17Build $config.repository $run $Jobs
        $null = Get-R17Source $config.repository $source.commit
        $receipt.candidate = New-R17Candidate $source $run
        Write-R17Json $receipt.candidate (Join-Path $run 'manifest.json')
        Assert-R17Exact (Join-Path $run 'distrib/bin') $receipt.candidate.files -Distribution
        Assert-R17Stopped
        # Repeat both preflights after the long build and before any target write.
        $receipt.preflight = Get-R17Preflight $config $reference
        foreach ($target in $receipt.preflight.targets) { Backup-R17Target $target $run }
        $receipt.status='BACKED_UP'; Write-R17Json $receipt (Join-Path $run 'receipt.json')
        foreach ($target in $receipt.preflight.targets) {
            $receipt.status="STAGING_$($target.id)"; Write-R17Json $receipt (Join-Path $run 'receipt.json')
            Set-R17Payload $target.staging (Join-Path $run 'distrib/bin') $target.stagingBefore $receipt.candidate.files
            Write-R17Json @{id=$target.id; status='STAGED'; actualGameVerified=$false; files=$receipt.candidate.files} (Join-Path $run "staged-$($target.id).json")
        }
        $receipt.status='AWAITING_HUMAN_VORTEX'; Write-R17Json $receipt (Join-Path $run 'receipt.json')
        $answer = Read-Host 'Deploy mods manually with Vortex on BOTH PCs, then close Vortex. Do not launch any game/server. Type DEPLOYED'
        if ($answer -cne 'DEPLOYED') { throw 'Staged only; human Vortex deployment not confirmed. Resume with Verify.' }
        Assert-R17Stopped
        Invoke-R17Workflow -ConfigPath $ConfigPath -Mode Verify -ReceiptPath $run
    } catch {
        $receipt.failedPhase=$receipt.status; $receipt.status='STOPPED'; $receipt.error=$_.Exception.Message
        Write-R17Json $receipt (Join-Path $run 'receipt.json')
        throw "STOP: $($_.Exception.Message) Receipt: $run. Use Verify after correction, or RestoreStaging then human Vortex deployment; do not launch mismatched clients."
    }
}
Export-ModuleMember -Function Invoke-R17Workflow
