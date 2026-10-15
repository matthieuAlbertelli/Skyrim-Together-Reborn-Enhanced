# Typed #94 adapter. Dot-sourced inside Stre.Modding; all common guards still apply.
function Assert-StrePapyrusCompilation($Result) {
    $diagnostics = $Result.Out -replace '(?im)^0 error\(s\), 0 warning\(s\)\s*$', ''
    if ($Result.ExitCode -ne 0 -or $Result.Err.Trim() -or
        $Result.Out -notmatch '1 succeeded, 0 failed' -or
        $Result.Out -notmatch '0 error\(s\), 0 warning\(s\)' -or
        $diagnostics -match '(?im)\bwarning\b|\berror\b') {
        throw "Papyrus compilation failed: $($Result.Out) $($Result.Err)"
    }
}

function Invoke-StreValenAudit($Config, [string]$Root, [string]$Candidate, [switch]$Generate) {
    $python = @(Get-Command python -CommandType Application -ErrorAction Stop)[0].Source
    $arguments = @((Join-Path $Config.RepoRoot 'Tools/Scripts/valen_helgen.py'),
        '--before', (Join-Path $Root 'before.esp'), '--candidate', $Candidate)
    if ($Generate) { $arguments += @('--generate', '--voice', 'MaleWarlock', '--manifest', (Join-Path $Root 'candidate-manifest.json')) }
    $result = Invoke-StreProcess $python $arguments $Config.RepoRoot
    if ($result.ExitCode -ne 0 -or $result.Err.Trim()) { throw "Valen candidate audit failed: $($result.Err) $($result.Out)" }
    ConvertFrom-StreAutoModJson $result.Out $result.ExitCode
}

function Invoke-StreValenAuthor($Config, [string]$Root, [ValidateSet('DryRun','Prepare')][string]$Mode) {
    $data = Join-Path $Root 'Data'
    @(($data + '\'), $Mode) | Set-Content -LiteralPath (Join-Path $Root 'authoring-mode.txt') -Encoding ascii
    $resultPath = Join-Path $Root 'authoring-result.txt'
    if (Test-Path -LiteralPath $resultPath) { throw 'Stale authoring result; use a new phase directory.' }
    $start = [Diagnostics.ProcessStartInfo]::new($Config.XEditPath)
    $start.UseShellExecute = $false
    $start.WorkingDirectory = $Root
    # xEdit script mode ignores autoload/autoexit and needs a visible FormShow.
    # The caller explicitly opts into this UI. Only four private modules are listed.
    foreach ($arg in @('-SSE', "-D:$data", "-M:$Root", "-I:$Root\Skyrim.ini",
        "-P:$Root\plugins.txt", '-cp:1252', "-script:$Root\author.ssepas")) { $start.ArgumentList.Add($arg) }
    Write-Host "xEdit ${Mode}: accept the private module selection, wait for script completion, then close xEdit. This is authoring, NOT human validation."
    $process = [Diagnostics.Process]::Start($start)
    try {
        $deadline = [DateTime]::UtcNow.AddMinutes(15)
        while (-not $process.WaitForExit(1000)) {
            if ([DateTime]::UtcNow -gt $deadline) {
                # Only this private tool instance; no automatic saving after failure.
                $process.Kill($true)
                throw 'Private authoring timed out; session is rejected.'
            }
        }
        if ($process.ExitCode -ne 0) { throw "xEdit authoring exit: $($process.ExitCode)" }
    } finally { $process.Dispose() }
    if (-not (Test-Path -LiteralPath $resultPath) -or
        (Get-Content -LiteralPath $resultPath -Raw).Trim() -cne "SUCCESS $Mode") { throw "xEdit $Mode did not report success; inspect private authoring output." }
}

function Initialize-StreValenPhase($Config, [string]$Root) {
    @('Skyrim.esm','Update.esm','Dragonborn.esm','*STRE_AlternateStart.esp') |
        Set-Content -LiteralPath (Join-Path $Root 'plugins.txt') -Encoding ascii
    @('[General]','sLanguage=French','[Archive]','bInvalidateOlderFiles=1','sResourceDataDirsFinal=') |
        Set-Content -LiteralPath (Join-Path $Root 'Skyrim.ini') -Encoding ascii
    Get-Content -LiteralPath (Join-Path $Config.RepoRoot 'Tools/Scripts/xedit/STRE_CreateValenHelgenDialogue.pas') -Raw |
        Set-Content -LiteralPath (Join-Path $Root 'author.ssepas') -Encoding utf8BOM
}

function Assert-StreValenCompanions($Config, $State, [string]$Root, $Record) {
    $expected = @("Source/Scripts/$($Record.result.QF).psc", "Scripts/$($Record.result.QF).pex",
        "Source/Scripts/$($Record.result.TIF).psc", "Scripts/$($Record.result.TIF).pex", 'SEQ/STRE_AlternateStart.seq')
    foreach ($pair in @(@($Record.result.NewsInfo,1),@($Record.result.NewsInfo,2),@($Record.result.AcceptInfo,1))) {
        $expected += ('Sound/Voice/STRE_AlternateStart.esp/MaleWarlock/stre_quest_stre_dial_valen_{0:x8}_{1}.wav' -f ($pair[0] -band 0xFFFFFF),$pair[1])
    }
    if ((($expected | Sort-Object) -join '|') -cne (($State.Companions.Keys | Sort-Object) -join '|')) { throw 'Unexpected companion paths.' }
    foreach ($relative in $expected) {
        $file = Resolve-StrePath (Join-Path $Root "Data/$relative")
        if ((Get-StreHash $file) -ne $State.Companions[$relative]) { throw "Companion changed: $relative" }
        foreach ($base in @($Config.DataRoot,(Join-Path $Config.RepoRoot 'GameFiles/Skyrim'))) {
            if (Test-Path -LiteralPath (Join-Path $base $relative)) { throw "Companion destination already exists: $relative" }
        }
    }
    if ((Get-StreHash (Join-Path $Root 'candidate-manifest.json')) -ne $State.ManifestHash) { throw 'Candidate manifest changed.' }
    foreach ($path in $State.Protected.Keys) { if ((Get-StreHash $path) -ne $State.Protected[$path]) { throw "Protected original changed: $path" } }
}

function Copy-StreValenCompanions($Config, $State, [string]$Root, [string]$Snapshot) {
    foreach ($relative in $State.Companions.Keys) {
        $destination = Join-Path $Snapshot $relative
        $null = New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force
        Copy-StreVerified (Join-Path $Root "Data/$relative") $destination
        if ((Get-StreHash $destination) -ne $State.Companions[$relative]) { throw 'Companion changed while freezing.' }
    }
    Copy-StreVerified (Join-Path $Root 'candidate-manifest.json') (Join-Path $Snapshot 'candidate-manifest.json')
    if ((Get-StreHash (Join-Path $Snapshot 'candidate-manifest.json')) -ne $State.ManifestHash) { throw 'Manifest changed while freezing.' }
}

function Import-StreValenCompanions($Config, $State, [string]$Root, [string]$Snapshot) {
    foreach ($relative in $State.Companions.Keys) {
        $frozen = Join-Path $Snapshot $relative
        if ((Get-StreHash $frozen) -ne $State.Companions[$relative]) { throw 'Frozen companion changed.' }
        Assert-GameProcessesStopped
        $live = Resolve-StrePath (Join-Path $Config.DataRoot $relative)
        $null = New-Item -ItemType Directory -Path (Split-Path -Parent $live) -Force
        Copy-StreVerified $frozen $live
        $import = Invoke-StreProcess (Join-Path $PSHOME 'pwsh.exe') @('-NoProfile','-File',$Config.ImportScript,
            '-RelativePath',$relative,'-SourceDataRoot',$Snapshot) $Config.RepoRoot
        if ($import.ExitCode -ne 0 -or (Get-StreHash (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$relative")) -ne $State.Companions[$relative]) {
            throw "Companion import failed: $relative. Stop and retain backups."
        }
    }
    $manifest = Join-Path $Snapshot 'candidate-manifest.json'
    if ((Get-StreHash $manifest) -ne $State.ManifestHash) { throw 'Frozen manifest changed.' }
    Copy-Item -LiteralPath $manifest -Destination (Join-Path $Config.RepoRoot 'docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json')
}

function New-StreValenSession($Config, [string]$Plugin, $Request, [switch]$DryRun) {
    if (-not $Request.ShowXEdit) { throw 'AddValenHelgen requires explicit -ShowXEdit for private authoring windows.' }
    if ($Request.EditorId -and $Request.EditorId -cne 'STRE_QUEST_ValenHelgen') { throw 'AddValenHelgen has a fixed EditorID and contract.' }
    $Request.EditorId = 'STRE_QUEST_ValenHelgen'
    $git = Assert-StreMutationGit $Config $Plugin
    $source = Resolve-StrePath (Join-Path $Config.DataRoot $Plugin)
    $repoPlugin = Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$Plugin")
    $hash = Get-StreHash $source
    if ($hash -ne (Get-StreHash $repoPlugin)) { throw 'Data and repository ESP differ.' }
    $header = Get-StreHeader $source
    Assert-StreFormat $header $header -Normalized
    $papyrus = Invoke-StreAutoMod $Config @('papyrus','status')
    if ($papyrus.result.compilerAvailable -isnot [bool] -or -not $papyrus.result.compilerAvailable -or
        $papyrus.result.decompilerAvailable -isnot [bool] -or -not $papyrus.result.decompilerAvailable) { throw 'Required Papyrus tools unavailable.' }
    $root = Resolve-StrePath (Join-Path $Config.RepoRoot ('_audit/modding/' + [guid]::NewGuid().ToString('N')))
    $data = Join-Path $root 'Data'
    $null = New-Item -ItemType Directory -Path $data
    Copy-StreVerified $source (Join-Path $root 'before.esp')
    Copy-StreVerified $repoPlugin (Join-Path $root 'repository-before.esp')
    $candidate = Join-Path $data $Plugin
    Copy-StreVerified $source $candidate
    $protected = @{}
    foreach ($path in @($source,$repoPlugin,(Join-Path $Config.DataRoot 'SkyrimTogether.esp'),
        (Join-Path $Config.RepoRoot 'GameFiles/Skyrim/SkyrimTogether.esp'))) { $protected[$path] = Get-StreHash $path }
    $masterHashes = @{}
    foreach ($master in $script:Masters) {
        Copy-StreVerified (Join-Path $Config.DataRoot $master) (Join-Path $data $master)
        $masterHashes[$master] = Get-StreHash (Join-Path $data $master)
    }
    $state = @{ Schema=1; State='Started'; Plugin=$Plugin; Request=$Request; Git=$git; Config=$Config
        BeforeHash=$hash; Header=$header; XEditHash=(Get-StreHash $Config.XEditPath)
        ImportHash=(Get-StreHash $Config.ImportScript); AutoModHash=(Get-StreHash $Config.AutoModDll); Protected=$protected; MasterHashes=$masterHashes }
    $statePath = Join-Path $root 'session.json'
    Write-StreJson $statePath $state
    Write-StreJson (Join-Path $root 'papyrus-status.json') $papyrus
    Write-Host "Session / backups: $root"
    try {
        Initialize-StreValenPhase $Config $root
        Invoke-StreValenAuthor $Config $root DryRun
        if ((Get-StreHash $candidate) -ne $hash -or (Test-Path (Join-Path $root 'authored.esp'))) { throw 'Dry-run modified the plugin.' }
        Move-Item -LiteralPath (Join-Path $root 'authoring-result.txt') -Destination (Join-Path $root 'dry-run-result.txt')
        if ($DryRun) { $state.State='DryRunOnly'; return @{Session=$root; State=$state.State; Sha256=$hash} }
        $null = Assert-StreMutationGit $Config $Plugin
        Assert-GameProcessesStopped
        Invoke-StreValenAuthor $Config $root Prepare
        if ((Get-StreHash $candidate) -ne $hash) { throw 'Authoring changed its immutable input.' }
        $authored = Join-Path $root 'authored.esp'
        $null = Invoke-StreValenAudit $Config $root $authored
        Copy-Item -LiteralPath $authored -Destination $candidate
        $record = Invoke-StreValenAudit $Config $root $candidate -Generate
        Write-StreJson (Join-Path $root 'record-after.json') $record
        Assert-StreFormat (Get-StreHeader $candidate) $header -Normalized
        $read = Invoke-StreAutoMod $Config @('esp','view-record',$candidate,'--editor-id',$Request.EditorId,'--type','quest','--include-raw')
        if ($read.result.FormKey -cne $record.result.FormKey) { throw 'AutoMod independent FormKey mismatch.' }
        Write-StreJson (Join-Path $root 'automod-after.json') $read
        $compiler = Resolve-StrePath $papyrus.result.compilerPath
        $imports = @((Join-Path $data 'Source/Scripts'), (Join-Path $Config.RepoRoot 'GameFiles/Skyrim/Source/Scripts'),
            (Join-Path $Config.RepoRoot 'GameFiles/Skyrim/scripts/source'), (Join-Path $Config.DataRoot 'scripts/source'),
            (Join-Path $Config.DataRoot 'Source/Scripts'))
        foreach ($dir in $imports) { if (-not (Test-Path -LiteralPath $dir -PathType Container)) { throw "Missing Papyrus import: $dir" } }
        $flags = Join-Path (Split-Path -Parent $compiler) 'TESV_Papyrus_Flags.flg'
        foreach ($name in @($record.result.QF,$record.result.TIF)) {
            $compiled = Invoke-StreProcess $compiler @((Join-Path $data "Source/Scripts/$name.psc"),
                "-i=$($imports -join ';')", "-o=$data\Scripts", "-f=$flags") $root
            Write-StreJson (Join-Path $root "$name.compile.json") $compiled
            Assert-StrePapyrusCompilation $compiled
            if (-not (Test-Path -LiteralPath (Join-Path $data "Scripts/$name.pex"))) { throw 'Missing compiled fragment.' }
        }
        $companions = @{}
        foreach ($folder in @('Source/Scripts','Scripts','SEQ','Sound/Voice')) {
            foreach ($file in Get-ChildItem -LiteralPath (Join-Path $data $folder) -File -Recurse) {
                $relative = [IO.Path]::GetRelativePath($data,$file.FullName).Replace('\','/')
                foreach ($base in @($Config.DataRoot,(Join-Path $Config.RepoRoot 'GameFiles/Skyrim'))) {
                    if (Test-Path -LiteralPath (Join-Path $base $relative)) { throw "Companion would overwrite existing file: $relative" }
                }
                $companions[$relative] = Get-StreHash $file.FullName
            }
        }
        $state.Companions = $companions
        $state.ManifestHash = Get-StreHash (Join-Path $root 'candidate-manifest.json')
        Assert-StreValenCompanions $Config $state $root $record
        $finalGit = Assert-StreMutationGit $Config $Plugin
        if ($finalGit.Head -cne $git.Head -or $finalGit.Branch -cne $git.Branch) { throw 'Git changed during preparation.' }
        $state.State = 'AwaitingHumanXEdit'
        return @{Session=$root; State=$state.State; Record=$record; Next='Human xEdit gate. No attestation has been created; no import/build/deploy.'}
    } catch { $state.State='Failed'; throw }
    finally {
        try {
            foreach ($path in $protected.Keys) {
                if ((Get-StreHash $path) -ne $protected[$path]) { throw "Protected original changed: $path" }
            }
            foreach ($master in $masterHashes.Keys) {
                if ((Get-StreHash (Join-Path $data $master)) -ne $masterHashes[$master]) { throw "Private master changed: $master" }
            }
        } catch { $state.State='Failed'; throw }
        finally { Write-StreJson $statePath $state }
    }
}
