# Typed #96 private-only preparation. The common process/Git/format guards apply.
function Invoke-StreMatrixPython($Config, [string]$Script, [string[]]$Arguments) {
    if ($Script -notin @('valen_matrix_writer.py','valen_matrix_audit.py','valen_matrix_assets.py','valen_presentation.py')) { throw 'Unsupported matrix helper.' }
    $python = @(Get-Command python -CommandType Application -ErrorAction Stop)[0].Source
    $result = Invoke-StreProcess $python (@((Join-Path $Config.RepoRoot "Tools/Scripts/$Script")) + $Arguments) $Config.RepoRoot
    if ($result.ExitCode -ne 0 -or $result.Err.Trim()) { throw "Matrix helper failed: $($result.Out) $($result.Err)" }
    ConvertFrom-StreAutoModJson $result.Out $result.ExitCode
}

function Initialize-StreMatrixPhase($Config, [string]$Root) {
    Initialize-StreValenPhase $Config $Root
    $null = Invoke-StreMatrixPython $Config 'valen_matrix_writer.py' @('--root',$Root)
}

function Invoke-StreMatrixAudit($Config, [string]$Root, [string]$Candidate) {
    Invoke-StreMatrixPython $Config 'valen_matrix_audit.py' @('--before',(Join-Path $Root 'before.esp'),
        '--candidate',$Candidate,'--output',(Join-Path $Root 'candidate-audit.json'))
}

function Assert-StreMatrixCI($Config, [string]$Head) {
    $result = Invoke-StreProcess 'gh' @('run','list','--commit',$Head,'--limit','30',
        '--json','headSha,name,status,conclusion,databaseId,url,createdAt') $Config.RepoRoot
    if ($result.ExitCode -ne 0 -or $result.Err.Trim()) { throw 'Cannot verify preparation CI.' }
    $runs = @($result.Out | ConvertFrom-Json -AsHashtable)
    $evidence = @()
    foreach ($name in @('Build windows','Build linux')) {
        $latest = @($runs | Where-Object { $_.name -ceq $name -and $_.headSha -ceq $Head } | Sort-Object createdAt -Descending)
        if (-not $latest.Count -or $latest[0].status -cne 'completed' -or $latest[0].conclusion -cne 'success') {
            throw "Preparation CI must be green on exact HEAD $Head ($name)."
        }
        $evidence += $latest[0]
    }
    $evidence
}

function Assert-StreMatrixProtected($State) {
    foreach ($path in $State.Protected.Keys) {
        if ((Get-StreHash $path) -cne $State.Protected[$path]) { throw "Protected original/tool changed: $path" }
    }
    foreach ($path in $State.PrivateMasters.Keys) {
        if ((Get-StreHash $path) -cne $State.PrivateMasters[$path]) { throw "Private master changed: $path" }
    }
}

function Get-StreMatrixCompanionIdentity([string]$GitPath, [string]$LivePath) {
    $gitHash = Get-StreHash $GitPath
    $liveHash = Get-StreHash $LivePath
    $comparison = 'ByteIdentical'
    if ($gitHash -cne $liveHash) {
        if ([IO.Path]::GetExtension($GitPath) -ine '.psc' -or [IO.Path]::GetExtension($LivePath) -ine '.psc') {
            throw "Git/Data companion differs: $GitPath"
        }
        # Git text=auto/core.autocrlf can make the checkout CRLF while Data is LF.
        # Latin1 is a bijection over bytes, not an encoding guess: only CRLF pairs
        # are compared as LF. BOM, all other bytes and final-newline count matter.
        $bytes = [Text.Encoding]::Latin1
        $gitText = $bytes.GetString([IO.File]::ReadAllBytes($GitPath))
        $liveText = $bytes.GetString([IO.File]::ReadAllBytes($LivePath))
        if ($gitText.Contains([char]0) -or $liveText.Contains([char]0) -or
            -not [String]::Equals($gitText.Replace("`r`n","`n"), $liveText.Replace("`r`n","`n"), [StringComparison]::Ordinal)) {
            throw "Git/Data companion differs beyond PSC line endings: $GitPath"
        }
        $comparison = 'PscCrLfOnly'
    }
    @{ GitSha256=$gitHash; DataSha256=$liveHash; Comparison=$comparison }
}

function Copy-StreMatrixAuthoredOutput([string]$Root, [string]$BeforeHash) {
    $candidate = Join-Path $Root 'Data/STRE_AlternateStart.esp'
    $archive = Join-Path $Root 'writer-input.esp'
    if ((Get-StreHash $candidate) -cne $BeforeHash) { throw 'Private writer input changed before archival.' }
    if (Test-Path -LiteralPath $archive) { throw 'Private writer input archive already exists.' }
    # Copy-StreVerified deliberately uses CreateNew. Preserve the checked input
    # beside before.esp before creating the separately audited candidate path.
    [IO.File]::Move($candidate, $archive)
    Copy-StreVerified (Join-Path $Root 'authored.esp') $candidate
    if ((Get-StreHash $archive) -cne $BeforeHash) { throw 'Private writer input archive changed.' }
}

function New-StreValenMatrixSession($Config, [string]$Plugin, $Request, [switch]$DryRun) {
    if (-not $Request.ShowXEdit) { throw 'AddValenDialogueMatrixV1 needs -ShowXEdit for official private authoring.' }
    if ($Request.EditorId -and $Request.EditorId -cne 'STRE_QUEST_ValenHelgen') { throw 'Fixed matrix EditorID contract.' }
    $Request.EditorId = 'STRE_QUEST_ValenHelgen'
    $git = Assert-StreMutationGit $Config $Plugin
    $ci = @(Assert-StreMatrixCI $Config $git.Head)
    $source = Resolve-StrePath (Join-Path $Config.DataRoot $Plugin)
    $repoPlugin = Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$Plugin")
    $before = Get-StreHash $source
    if ($before -cne (Get-StreHash $repoPlugin)) { throw 'Git/Data ESP differ.' }
    $header = Get-StreHeader $source
    Assert-StreFormat $header $header -Normalized
    $papyrus = Invoke-StreAutoMod $Config @('papyrus','status')
    if ($papyrus.result.compilerAvailable -isnot [bool] -or -not $papyrus.result.compilerAvailable -or
        $papyrus.result.decompilerAvailable -isnot [bool] -or -not $papyrus.result.decompilerAvailable) { throw 'Required Papyrus tools unavailable.' }
    $compiler = Resolve-StrePath $papyrus.result.compilerPath
    if ([IO.Path]::GetFileName($compiler) -ine 'PapyrusCompiler.exe') { throw 'Direct PapyrusCompiler.exe required.' }
    $root = Resolve-StrePath (Join-Path $Config.RepoRoot ('_audit/modding/' + [guid]::NewGuid().ToString('N')))
    $data = Join-Path $root 'Data'
    $null = New-Item -ItemType Directory -Path $data
    $state = @{ Schema=1; State='Started'; Plugin=$Plugin; Request=$Request; Git=$git; Config=$Config
        BeforeHash=$before; Header=$header; Protected=@{}; PrivateMasters=@{}; PrivateOriginals=@{}; InputComparisons=@{}; PreparationCI=$ci
        XEditHash=(Get-StreHash $Config.XEditPath); AutoModHash=(Get-StreHash $Config.AutoModDll)
        ImportHash=(Get-StreHash $Config.ImportScript); CompilerHash=(Get-StreHash $compiler) }
    $statePath = Join-Path $root 'session.json'
    try {
        $manifest = Join-Path $Config.RepoRoot 'GameFiles/STRE_AlternateStart.manifest.txt'
        $paths = @(Get-Content -LiteralPath $manifest | Where-Object { $_.Trim() -and -not $_.StartsWith('#') })
        if ($paths | Where-Object { $_ -match '(?i)\.vortex_backup$' -or $_ -match '\.\.' -or [IO.Path]::IsPathRooted($_) }) { throw 'Unsafe managed path.' }
        foreach ($relative in $paths) {
            $gitPath = Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$relative")
            $livePath = Resolve-StrePath (Join-Path $Config.DataRoot $relative)
            $identity = Get-StreMatrixCompanionIdentity $gitPath $livePath
            $state.InputComparisons[$relative.Replace('\','/')] = $identity
            $state.Protected[$gitPath] = $identity.GitSha256
            $state.Protected[$livePath] = $identity.DataSha256
            $state.PrivateOriginals[$relative.Replace('\','/')] = $identity.GitSha256
            $destination = Join-Path $data $relative
            $null = New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force
            Copy-StreVerified $gitPath $destination
        }
        foreach ($path in @($manifest,$Config.XEditPath,$Config.AutoModDll,$Config.ImportScript,$compiler,
            (Join-Path $Config.RepoRoot 'docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json'),
            (Join-Path $Config.DataRoot 'SkyrimTogether.esp'),(Join-Path $Config.RepoRoot 'GameFiles/Skyrim/SkyrimTogether.esp'))) {
            $state.Protected[$path] = Get-StreHash $path
        }
        foreach ($master in $script:Masters) {
            Copy-StreVerified (Join-Path $Config.DataRoot $master) (Join-Path $data $master)
            $state.PrivateMasters[(Join-Path $data $master)] = Get-StreHash (Join-Path $data $master)
        }
        Copy-StreVerified $source (Join-Path $root 'before.esp')
        Copy-StreVerified $repoPlugin (Join-Path $root 'repository-before.esp')
        Write-StreJson (Join-Path $root 'papyrus-status.json') $papyrus
        Write-StreJson (Join-Path $root 'preparation-ci.json') $ci
        Initialize-StreMatrixPhase $Config $root
        $state.PlanHash = Get-StreHash (Join-Path $root 'record-plan.json')
        $state.WriterHash = Get-StreHash (Join-Path $root 'author.ssepas')
        Write-StreJson $statePath $state
        Write-Host "Private matrix session and exact plan: $root"
        $candidate = Join-Path $data $Plugin
        Invoke-StreValenAuthor $Config $root DryRun
        if ((Get-StreHash $candidate) -cne $before -or (Test-Path (Join-Path $root 'authored.esp'))) { throw 'DryRun mutated input.' }
        Move-Item -LiteralPath (Join-Path $root 'authoring-result.txt') -Destination (Join-Path $root 'dry-run-result.txt')
        Assert-StreMatrixProtected $state
        if ($DryRun) { $state.State='DryRunOnly'; return @{Success=$true;Session=$root;State=$state.State;Sha256=$before} }
        $current = Assert-StreMutationGit $Config $Plugin
        if ($current.Head -cne $git.Head -or $current.Branch -cne $git.Branch) { throw 'Git changed before Prepare.' }
        Assert-GameProcessesStopped
        Invoke-StreValenAuthor $Config $root Prepare
        if ((Get-StreHash $candidate) -cne $before) { throw 'Writer modified immutable input.' }
        $authored = Join-Path $root 'authored.esp'
        $null = Invoke-StreMatrixAudit $Config $root $authored
        Copy-StreMatrixAuthoredOutput $root $before
        Assert-StreFormat (Get-StreHeader $candidate) $header -Normalized
        $assets = Invoke-StreMatrixPython $Config 'valen_matrix_assets.py' @('--root',$root)
        $imports = @((Join-Path $data 'Source/Scripts'),(Join-Path $Config.RepoRoot 'GameFiles/Skyrim/Source/Scripts'),
            (Join-Path $Config.RepoRoot 'GameFiles/Skyrim/scripts/source'),(Join-Path $Config.DataRoot 'scripts/source'),
            (Join-Path $Config.DataRoot 'Source/Scripts'))
        foreach ($directory in $imports) { if (-not (Test-Path -LiteralPath $directory -PathType Container)) { throw "Papyrus import missing: $directory" } }
        $flags = Join-Path (Split-Path -Parent $compiler) 'TESV_Papyrus_Flags.flg'
        foreach ($name in $assets.scripts) {
            $args = @((Join-Path $data "Source/Scripts/$name.psc"),"-i=$($imports -join ';')","-o=$data\Scripts","-f=$flags")
            Write-StreJson (Join-Path $root "$name.command.json") @{Executable=$compiler;Arguments=$args;Purpose='Compile Papyrus directly; no ESP writes'}
            Write-Host "Papyrus compiler: $compiler"
            Write-Host "Arguments: $($args -join ' ')"
            $compiled = Invoke-StreProcess $compiler $args $root
            Write-StreJson (Join-Path $root "$name.compile.json") $compiled
            Assert-StrePapyrusCompilation $compiled
            if (-not (Test-Path -LiteralPath (Join-Path $data "Scripts/$name.pex"))) { throw 'Missing compiled script.' }
        }
        $recordAudit = Invoke-StreMatrixAudit $Config $root $candidate
        $assetAudit = Invoke-StreMatrixPython $Config 'valen_matrix_assets.py' @('--root',$root,'--verify')
        $state.Companions = @{}
        foreach ($folder in @('Source/Scripts','Scripts','SEQ','Sound/Voice')) {
            foreach ($file in Get-ChildItem -LiteralPath (Join-Path $data $folder) -File -Recurse) {
                $relative = [IO.Path]::GetRelativePath($data,$file.FullName).Replace('\','/')
                if ($relative -match '(?i)\.vortex_backup$') { throw 'Vortex metadata in private bundle.' }
                $state.Companions[$relative] = Get-StreHash $file.FullName
            }
        }
        $expectedCompanions = @(@($state.PrivateOriginals.Keys | Where-Object { $_ -cne $Plugin }) +
            @($assetAudit.paths) | Sort-Object -Unique)
        if ((($state.Companions.Keys | Sort-Object) -join '|') -cne ($expectedCompanions -join '|')) {
            throw 'Unexpected private companion inventory.'
        }
        $replaced = @("Source/Scripts/$($recordAudit.QF).psc","Scripts/$($recordAudit.QF).pex")
        foreach ($relative in $state.PrivateOriginals.Keys) {
            if ($relative -ceq $Plugin -or $relative -cin $replaced) { continue }
            if ($state.Companions[$relative] -cne $state.PrivateOriginals[$relative]) {
                throw "Unrelated private companion changed: $relative"
            }
        }
        $current = Assert-StreMutationGit $Config $Plugin
        if ($current.Head -cne $git.Head -or $current.Branch -cne $git.Branch) { throw 'Git changed during Prepare.' }
        if ((Get-StreHash (Join-Path $root 'record-plan.json')) -cne $state.PlanHash -or
            (Get-StreHash (Join-Path $root 'author.ssepas')) -cne $state.WriterHash) { throw 'Plan/writer changed during Prepare.' }
        $state.CandidateHash = Get-StreHash $candidate
        $state.State = 'AwaitingHumanXEdit'
        Write-StreJson (Join-Path $root 'validation.json') @{
            Schema=1; PluginSha256=''; XEditSha256=$state.XEditHash; Reviewer='';
            SortMasters=$false; CheckForErrors=$false; SavedAndClosed=$false; LogSha256=''
        }
        return @{Success=$true;Session=$root;State='HUMAN_XEDIT_REQUIRED';CandidateSha256=$state.CandidateHash;
            Next='Human normalization/review/save/reopen/errors required. No import, build, deploy or runtime.'}
    } catch { $state.State='Failed'; throw }
    finally {
        try { Assert-StreMatrixProtected $state }
        catch { $state.State='Failed'; throw }
        finally { Write-StreJson $statePath $state }
    }
}
