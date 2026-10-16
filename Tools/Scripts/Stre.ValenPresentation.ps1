# Bounded content iteration; the full CI gate is deferred to the merge HEAD by
# the maintainer's 2026-10-16 instruction. Human xEdit review is still required.
function New-StreValenPresentationSession($Config, [string]$Plugin, $Request, [switch]$DryRun) {
    if (-not $Request.ShowXEdit) { throw 'FixValenPresentation needs -ShowXEdit.' }
    if ($Request.Variant -notin @('A','B')) { throw 'Only presentation variants A/B are supported.' }
    if ($Request.EditorId -or $Request.Name) { throw 'Presentation has no arbitrary record/text override.' }
    $git = Assert-StreMutationGit $Config $Plugin
    $repoPlugin = Join-Path $Config.RepoRoot "GameFiles/Skyrim/$Plugin"
    $source = Join-Path $Config.DataRoot $Plugin
    $before = Get-StreHash $repoPlugin
    if ($before -cne '2ECEA346231CF6F6453205948FEF4E6C5CE5882C19C1F9141D9873A41DEE355E' -or
        (Get-StreHash $source) -cne $before) { throw 'Presentation requires the identical tested c48be63 Git/Data baseline.' }
    $root = Resolve-StrePath (Join-Path $Config.RepoRoot ('_audit/modding/' + [guid]::NewGuid().ToString('N')))
    $data = Join-Path $root 'Data'
    $null = New-Item -ItemType Directory -Path $data
    $state = @{ Schema=1; State='Started'; Plugin=$Plugin; Request=$Request; Git=$git; Config=$Config;
        BeforeHash=$before; Header=(Get-StreHeader $repoPlugin); Protected=@{}; PrivateMasters=@{};
        PrivateOriginals=@{}; XEditHash=(Get-StreHash $Config.XEditPath);
        CI='Full CI deferred to merge HEAD by maintainer; local gates required before DEV smoke' }
    $statePath = Join-Path $root 'session.json'
    try {
        $manifest = Join-Path $Config.RepoRoot 'GameFiles/STRE_AlternateStart.manifest.txt'
        $paths = @(Get-Content -LiteralPath $manifest | Where-Object { $_.Trim() -and -not $_.StartsWith('#') })
        foreach ($relative in $paths) {
            if ($relative -match '(?i)\.vortex_backup$|\.\.' -or [IO.Path]::IsPathRooted($relative)) { throw 'Unsafe managed path.' }
            $gitPath = Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$relative")
            $livePath = Resolve-StrePath (Join-Path $Config.DataRoot $relative)
            $hash = Get-StreHash $gitPath
            if ((Get-StreHash $livePath) -cne $hash) { throw "Git/Data differs: $relative" }
            $state.Protected[$gitPath]=$hash; $state.Protected[$livePath]=$hash
            $state.PrivateOriginals[$relative.Replace('\','/')]=$hash
            $destination = Join-Path $data $relative
            $null = New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force
            Copy-StreVerified $gitPath $destination
        }
        foreach ($path in @($manifest,$Config.XEditPath,$Config.AutoModDll,$Config.ImportScript,
            (Join-Path $Config.RepoRoot 'docs/narrative/valen/VALEN_DIALOGUE_MATRIX_V1.json'),
            (Join-Path $Config.RepoRoot 'docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json'),
            (Join-Path $Config.RepoRoot 'Tools/Scripts/valen_presentation.py'),
            (Join-Path $Config.RepoRoot 'Tools/Scripts/xedit/STRE_ValenPresentation.pas'),
            (Join-Path $Config.DataRoot 'SkyrimTogether.esp'),(Join-Path $Config.RepoRoot 'GameFiles/Skyrim/SkyrimTogether.esp'))) {
            $state.Protected[$path]=Get-StreHash $path
        }
        foreach ($master in $script:Masters) {
            $path=Join-Path $Config.DataRoot $master
            $hash=Get-StreHash $path
            $state.Protected[$path]=$hash
            Copy-StreVerified $path (Join-Path $data $master)
            $state.PrivateMasters[(Join-Path $data $master)]=$hash
        }
        Copy-StreVerified $repoPlugin (Join-Path $root 'before.esp')
        Copy-StreVerified $repoPlugin (Join-Path $root 'repository-before.esp')
        Initialize-StreValenPhase $Config $root
        $null=Invoke-StreMatrixPython $Config 'valen_presentation.py' @('--root',$root,'--variant',$Request.Variant)
        foreach ($name in @('presentation-plan.json','author.ssepas','before.esp','repository-before.esp')) {
            $state.Protected[(Join-Path $root $name)]=Get-StreHash (Join-Path $root $name)
        }
        Write-StreJson $statePath $state
        Write-Host "Private presentation session: $root"
        Invoke-StreValenAuthor $Config $root DryRun
        if ((Get-StreHash (Join-Path $data $Plugin)) -cne $before -or (Test-Path (Join-Path $root 'authored.esp'))) { throw 'DryRun mutated input.' }
        Move-Item -LiteralPath (Join-Path $root 'authoring-result.txt') -Destination (Join-Path $root 'dry-run-result.txt')
        Assert-StreMatrixProtected $state
        if ($DryRun) { $state.State='DryRunOnly'; return @{Success=$true;Session=$root;State=$state.State} }
        $current=Assert-StreMutationGit $Config $Plugin
        if ($current.Head -cne $git.Head -or $current.Branch -cne $git.Branch) { throw 'Git changed before authoring.' }
        Assert-GameProcessesStopped
        Invoke-StreValenAuthor $Config $root Prepare
        if ((Get-StreHash (Join-Path $data $Plugin)) -cne $before) { throw 'Writer input changed.' }
        $null=Invoke-StreMatrixPython $Config 'valen_presentation.py' @('--root',$root,'--variant',$Request.Variant,'--audit')
        Copy-StreMatrixAuthoredOutput $root $before
        Assert-StreFormat (Get-StreHeader (Join-Path $data $Plugin)) $state.Header -Normalized
        foreach ($relative in $state.PrivateOriginals.Keys) {
            if ($relative -cne $Plugin -and (Get-StreHash (Join-Path $data $relative)) -cne $state.PrivateOriginals[$relative]) { throw "Companion changed: $relative" }
        }
        $current=Assert-StreMutationGit $Config $Plugin
        if ($current.Head -cne $git.Head -or $current.Branch -cne $git.Branch) { throw 'Git changed during authoring.' }
        $state.CandidateHash=Get-StreHash (Join-Path $data $Plugin)
        $state.State='AwaitingHumanXEdit'
        return @{Success=$true;Session=$root;State='HUMAN_XEDIT_REQUIRED';Variant=$Request.Variant;
            CandidateSha256=$state.CandidateHash;Next='Review this exact new candidate in official xEdit; old receipt does not apply. Runtime A load evidence then B smoke; no C/generalization yet.'}
    } catch { $state.State='Failed'; throw }
    finally {
        try { Assert-StreMatrixProtected $state } catch { $state.State='Failed'; throw }
        finally { Write-StreJson $statePath $state }
    }
}

function Import-StrePresentationValidated($Config, [string]$Root, $State) {
    if ($State.Schema -ne 1 -or $State.State -cne 'AwaitingHumanXEdit' -or
        $State.Request.Operation -cne 'FixValenPresentation' -or $State.Request.Variant -notin @('A','B')) {
        throw 'Presentation session is not awaiting human review.'
    }
    $git=Assert-StreMutationGit $Config $State.Plugin
    if ($git.Head -cne $State.Git.Head -or $git.Branch -cne $State.Git.Branch) { throw 'Presentation Git changed since preparation.' }
    foreach ($key in @('RepoRoot','DataRoot','XEditPath','AutoModRoot','ImportScript')) {
        if ($Config[$key] -ine $State.Config[$key]) { throw "Presentation configuration changed: $key" }
    }
    Assert-StreMatrixProtected $State
    $candidate=Join-Path $Root "Data/$($State.Plugin)"
    $receiptPath=Join-Path $Root 'validation.json'
    if (-not (Test-Path -LiteralPath $receiptPath)) { throw 'HUMAN_XEDIT_REQUIRED: new presentation receipt missing.' }
    $receipt=Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json -AsHashtable
    foreach ($field in @('SortMasters','CheckForErrors','SavedAndClosed')) {
        if ($receipt[$field] -isnot [bool] -or -not $receipt[$field]) { throw "HUMAN_XEDIT_REQUIRED: $field" }
    }
    if ($receipt.Schema -ne 1 -or [string]::IsNullOrWhiteSpace($receipt.Reviewer) -or
        $receipt.XEditSha256 -cne $State.XEditHash -or $receipt.PluginSha256 -cne (Get-StreHash $candidate)) {
        throw 'Presentation receipt does not identify saved bytes/tool.'
    }
    $log=Join-Path $Root 'xedit-validation.log'
    if (-not (Test-Path -LiteralPath $log) -or (Get-Item -LiteralPath $log).Length -eq 0 -or
        (Get-StreHash $log) -cne $receipt.LogSha256) { throw 'Presentation human log missing or changed.' }
    $null=Invoke-StreMatrixPython $Config 'valen_presentation.py' @('--root',$Root,'--variant',$State.Request.Variant,'--audit','--saved')
    Assert-StreFormat (Get-StreHeader $candidate) $State.Header -Normalized
    foreach ($relative in $State.PrivateOriginals.Keys) {
        if ($relative -cne $State.Plugin -and (Get-StreHash (Join-Path $Root "Data/$relative")) -cne $State.PrivateOriginals[$relative]) {
            throw "Presentation companion changed: $relative"
        }
    }
    $snapshot=Join-Path $Root 'validated'
    if (Test-Path -LiteralPath $snapshot) { throw 'Presentation snapshot already exists; do not replay.' }
    $null=New-Item -ItemType Directory -Path $snapshot
    $frozen=Join-Path $snapshot $State.Plugin
    Copy-StreVerified $candidate $frozen
    if ((Get-StreHash $frozen) -cne $receipt.PluginSha256) { throw 'Presentation changed while freezing.' }
    $null=Assert-StreMutationGit $Config $State.Plugin
    Assert-GameProcessesStopped
    Assert-StreMatrixProtected $State
    $live=Join-Path $Config.DataRoot $State.Plugin
    $repoPlugin=Join-Path $Config.RepoRoot "GameFiles/Skyrim/$($State.Plugin)"
    $State.State='ImportStarted'
    Write-StreJson (Join-Path $Root 'session.json') $State
    try {
        # Replace only this Data entry, avoiding writes through Vortex hardlinks.
        # before.esp/repository-before.esp retain both identical original bytes.
        $temporary=Join-Path $Config.DataRoot ('STRE-presentation-'+[guid]::NewGuid().ToString('N')+'.tmp')
        Copy-StreVerified $frozen $temporary
        if ((Get-StreHash $live) -cne $State.BeforeHash) { throw 'Data changed before presentation promotion.' }
        [IO.File]::Move($temporary,$live,$true)
        $import=Invoke-StreProcess (Join-Path $PSHOME 'pwsh.exe') @('-NoProfile','-File',$Config.ImportScript,
            '-RelativePath',$State.Plugin,'-SourceDataRoot',$snapshot) $Config.RepoRoot
        Write-StreJson (Join-Path $Root 'import-result.json') $import
        if ($import.ExitCode -ne 0 -or (Get-StreHash $repoPlugin) -cne $receipt.PluginSha256 -or
            (Get-StreHash $live) -cne $receipt.PluginSha256) { throw 'Presentation import failed; preserve backups and stop.' }
        foreach ($path in $State.Protected.Keys) {
            if ($path -ine $live -and $path -ine $repoPlugin -and (Get-StreHash $path) -cne $State.Protected[$path]) {
                throw "Unexpected protected change after presentation import: $path"
            }
        }
        $State.State='Imported'; $State.CandidateHash=$receipt.PluginSha256
        return @{Success=$true;State=$State.State;Session=$Root;Sha256=$receipt.PluginSha256;CompanionsChanged=0}
    } catch { $State.State='ImportFailed'; throw }
    finally { Write-StreJson (Join-Path $Root 'session.json') $State }
}
