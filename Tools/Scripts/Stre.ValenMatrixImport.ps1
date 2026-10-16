# Dedicated #96 import. Preparation evidence is immutable; only lifecycle state evolves.
function Assert-StreMatrixImportGit($Config, $State) {
    $git = Assert-StreMutationGit $Config $State.Plugin
    if ($git.Branch -cne $State.Git.Branch) { throw 'Matrix preparation branch changed.' }
    $preparation = $State.Git.Head
    if ($preparation -notmatch '^[a-f0-9]{40}$') { throw 'Invalid preparation HEAD.' }
    try { $null = Invoke-StreGit $Config @('merge-base','--is-ancestor',$preparation,$git.Head) }
    catch { throw 'Preparation HEAD must be an ancestor of import HEAD.' }
    @{ Git=$git; CI=@(Assert-StreMatrixCI $Config $git.Head) }
}

function Get-StreMatrixImportInventory($Config, [string]$Root) {
    $python = @(Get-Command python -CommandType Application -ErrorAction Stop)[0].Source
    $result = Invoke-StreProcess $python @('-B',(Join-Path $PSScriptRoot 'valen_matrix_import.py'),'--root',$Root) $Config.RepoRoot
    if ($result.ExitCode -ne 0 -or $result.Err.Trim()) { throw "Matrix import audit failed: $($result.Err) $($result.Out)" }
    ConvertFrom-StreAutoModJson $result.Out $result.ExitCode
}

function Assert-StreMatrixTargets($Config, $State, $Inventory) {
    foreach ($relative in $Inventory.Files.Keys) {
        foreach ($base in @($Config.DataRoot, (Join-Path $Config.RepoRoot 'GameFiles/Skyrim'))) {
            $target = Resolve-StrePath (Join-Path $base $relative)
            if ($State.PrivateOriginals.Contains($relative)) {
                if (-not $State.Protected.Contains($target) -or (Get-StreHash $target) -cne $State.Protected[$target]) {
                    throw "Existing import target changed: $target"
                }
            } elseif (Test-Path -LiteralPath $target) { throw "New import target already exists: $target" }
        }
    }
}

function Assert-StreMatrixPromoted($Config, $Inventory, [string]$Snapshot) {
    foreach ($relative in $Inventory.Files.Keys) {
        foreach ($base in @($Snapshot,$Config.DataRoot,(Join-Path $Config.RepoRoot 'GameFiles/Skyrim'))) {
            if ((Get-StreHash (Resolve-StrePath (Join-Path $base $relative))) -cne $Inventory.Files[$relative]) {
                throw "Matrix promoted hash mismatch: $base/$relative"
            }
        }
    }
    $actual = @(Get-ChildItem -LiteralPath $Snapshot -Recurse -File | ForEach-Object {
        [IO.Path]::GetRelativePath($Snapshot,$_.FullName).Replace('\','/')
    } | Sort-Object)
    if (($actual -join '|') -cne (($Inventory.Files.Keys | Sort-Object) -join '|')) { throw 'Unexpected snapshot file.' }
}

function Assert-StreMatrixUnchanged($State, [string[]]$Allowed) {
    foreach ($path in $State.Protected.Keys) {
        if ($path -notin $Allowed -and (Get-StreHash $path) -cne $State.Protected[$path]) {
            throw "Out-of-scope protected file changed: $path"
        }
    }
    foreach ($path in $State.PrivateMasters.Keys) {
        if ((Get-StreHash $path) -cne $State.PrivateMasters[$path]) { throw "Private master changed: $path" }
    }
}

function Update-StreMatrixPackaging($Config, $Inventory) {
    # Preserve the workstation's established manifest formatting and ownership helper.
    # Parse and load ONLY these two pure functions, never the deployment entry point.
    $build = Join-Path $Config.RepoRoot 'build-and-deploy-dev.ps1'
    $tokens=$null; $errors=$null
    $ast = [Management.Automation.Language.Parser]::ParseFile($build,[ref]$tokens,[ref]$errors)
    if ($errors.Count) { throw 'Cannot parse packaging helper.' }
    foreach ($name in @('Read-ManagedFileManifest','Add-NewManagedFilesToManifest')) {
        $found = @($ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -ceq $name},$true))
        if ($found.Count -ne 1) { throw "Missing/ambiguous packaging helper: $name" }
        . ([scriptblock]::Create($found[0].Extent.Text))
    }
    Add-NewManagedFilesToManifest -ManifestPath (Join-Path $Config.RepoRoot 'GameFiles/STRE_AlternateStart.manifest.txt') `
        -RelativeFiles @($Inventory.Files.Keys | ForEach-Object { $_.Replace('/','\') })
}

function Get-StreMatrixPackaging([string]$Path) {
    @(Get-Content -LiteralPath $Path | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') } |
        ForEach-Object { $_.Replace('\','/') })
}

function Import-StreValenMatrixValidated($Config, [string]$Root, $State) {
    if ($State.Schema -ne 1 -or $State.State -cne 'AwaitingHumanXEdit') { throw 'Matrix session is not awaiting import; no reimport.' }
    Assert-StrePlugin $State.Plugin -Writable
    foreach ($key in @('RepoRoot','DataRoot','XEditPath','AutoModRoot','ImportScript')) {
        if ($Config[$key] -ine $State.Config[$key]) { throw "Matrix configuration changed: $key" }
    }
    $provenance = Assert-StreMatrixImportGit $Config $State
    $inventory = Get-StreMatrixImportInventory $Config $Root
    Assert-StreFormat (Get-StreHeader (Join-Path $Root "Data/$($State.Plugin)")) $State.Header -Normalized
    Assert-StreMatrixTargets $Config $State $inventory
    Assert-StreMatrixProtected $State
    Assert-GameProcessesStopped
    foreach ($path in @('validated','import-backups','preparation-session.json','import-inventory.json')) {
        if (Test-Path -LiteralPath (Join-Path $Root $path)) { throw "Import output already exists: $path. Inspect; do not retry a partial import." }
    }
    # Parse helper and validate its actual parameter contract before any live mutation.
    $tokens=$null; $errors=$null
    $buildPath=Join-Path $Config.RepoRoot 'build-and-deploy-dev.ps1'
    $buildAst=[Management.Automation.Language.Parser]::ParseFile($buildPath,[ref]$tokens,[ref]$errors)
    if ($errors.Count) { throw 'Invalid local build helper.' }
    foreach ($name in @('Read-ManagedFileManifest','Add-NewManagedFilesToManifest')) {
        $nodes=@($buildAst.FindAll({param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -ceq $name},$true))
        if ($nodes.Count -ne 1) { throw "Missing packaging helper: $name" }
        $parameters=@($nodes[0].Body.ParamBlock.Parameters | ForEach-Object { $_.Name.VariablePath.UserPath })
        $required=if($name -eq 'Read-ManagedFileManifest'){@('ManifestPath')}else{@('ManifestPath','RelativeFiles')}
        if (@($required | Where-Object { $_ -notin $parameters }).Count) { throw "Packaging helper contract changed: $name" }
    }
    $buildHash=Get-StreHash $buildPath
    $packagingPath=Join-Path $Config.RepoRoot 'GameFiles/STRE_AlternateStart.manifest.txt'
    $packagingBefore=@(Get-StreMatrixPackaging $packagingPath)
    $packagingExpected=@(@($packagingBefore)+@($inventory.Files.Keys) | Sort-Object -Unique)
    $snapshot=Join-Path $Root 'validated'
    $backup=Join-Path $Root 'import-backups'
    $null=New-Item -ItemType Directory -Path $snapshot,$backup
    foreach ($relative in $inventory.Files.Keys) {
        $frozen=Join-Path $snapshot $relative
        $null=New-Item -ItemType Directory -Path (Split-Path -Parent $frozen) -Force
        Copy-StreVerified (Join-Path $Root "Data/$relative") $frozen
        if ((Get-StreHash $frozen) -cne $inventory.Files[$relative]) { throw 'Snapshot changed during copy.' }
        if ($State.PrivateOriginals.Contains($relative)) {
            foreach ($kind in @('Data','Git')) {
                $base=if($kind -eq 'Data'){$Config.DataRoot}else{Join-Path $Config.RepoRoot 'GameFiles/Skyrim'}
                $destination=Join-Path $backup "$kind/$relative"
                $null=New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force
                Copy-StreVerified (Join-Path $base $relative) $destination
            }
        }
    }
    $inventoryPath=Join-Path $Root 'import-inventory.json'
    Write-StreJson $inventoryPath $inventory
    Copy-StreVerified (Join-Path $Root 'session.json') (Join-Path $Root 'preparation-session.json')
    # Final current-auditor read immediately before promotion. Historical files remain untouched.
    $reread=Get-StreMatrixImportInventory $Config $Root
    if (($reread | ConvertTo-Json -Depth 32 -Compress) -cne ($inventory | ConvertTo-Json -Depth 32 -Compress)) { throw 'Final import reread differs.' }
    Assert-StreMatrixTargets $Config $State $inventory
    $current=Assert-StreMutationGit $Config $State.Plugin
    if ($current.Head -cne $provenance.Git.Head -or $current.Branch -cne $provenance.Git.Branch) { throw 'Git changed before import.' }
    Assert-GameProcessesStopped
    $State.PreparationHead=$State.Git.Head
    $State.ImportHead=$current.Head
    $State.ImportCI=$provenance.CI
    $State.ImportEvidenceHash=Get-StreHash (Join-Path $Root 'import-evidence.json')
    $State.ImportInventoryHash=Get-StreHash $inventoryPath
    $State.State='ImportStarted'
    Write-StreJson (Join-Path $Root 'session.json') $State
    try {
        foreach ($relative in $inventory.Files.Keys) {
            Assert-GameProcessesStopped
            $inputFile=[IO.File]::OpenRead((Join-Path $snapshot $relative))
            try {
                $sha=[Security.Cryptography.SHA256]::Create()
                try { $frozenHash=[Convert]::ToHexString($sha.ComputeHash($inputFile)) } finally { $sha.Dispose() }
                if ($frozenHash -cne $inventory.Files[$relative]) { throw "Frozen snapshot changed: $relative" }
                $inputFile.Position=0
            $live=Resolve-StrePath (Join-Path $Config.DataRoot $relative)
            $null=New-Item -ItemType Directory -Path (Split-Path -Parent $live) -Force
            $mode=if($State.PrivateOriginals.Contains($relative)){'Open'}else{'CreateNew'}
            $handle=[IO.File]::Open($live,$mode,'ReadWrite','None')
            try {
                if ($mode -eq 'Open') {
                    $sha=[Security.Cryptography.SHA256]::Create()
                    try { $actual=[Convert]::ToHexString($sha.ComputeHash($handle)) } finally { $sha.Dispose() }
                    if ($actual -cne $State.Protected[$live]) { throw "Data target changed before promotion: $relative" }
                    $handle.Position=0
                }
                $inputFile.CopyTo($handle); $handle.SetLength($inputFile.Length)
            } finally { $handle.Dispose() }
            } finally { $inputFile.Dispose() }
        }
        Assert-GameProcessesStopped
        if ((Get-StreHash $inventoryPath) -cne $State.ImportInventoryHash) { throw 'Import inventory changed.' }
        $result=Invoke-StreProcess (Join-Path $PSHOME 'pwsh.exe') @('-NoProfile','-File',
            (Join-Path $PSScriptRoot 'Invoke-StreMatrixCkImport.ps1'),'-ImportScript',$Config.ImportScript,
            '-Inventory',$inventoryPath,'-SourceDataRoot',$snapshot) $Config.RepoRoot @{} 600000
        Write-StreJson (Join-Path $Root 'import-result.json') $result
        if ($result.ExitCode -ne 0) { throw "Existing CK importer failed: $($result.Err). Retain backups; no retry." }
        Assert-StreMatrixPromoted $Config $inventory $snapshot
        $allowed=@(foreach($relative in $inventory.Files.Keys){
            Resolve-StrePath (Join-Path $Config.DataRoot $relative)
            Resolve-StrePath (Join-Path $Config.RepoRoot "GameFiles/Skyrim/$relative")
        })
        Assert-StreMatrixUnchanged $State $allowed
        $manifest=Join-Path $Root 'review-proposed-record-manifest.json'
        if ((Get-StreHash $manifest) -cne $inventory.ManifestSha256 -or (Get-StreHash $buildPath) -cne $buildHash) { throw 'Manifest/packaging helper changed.' }
        $recordTarget=Join-Path $Config.RepoRoot $inventory.ManifestPath
        Copy-Item -LiteralPath $manifest -Destination $recordTarget
        if ((Get-StreHash $recordTarget) -cne $inventory.ManifestSha256) { throw 'Promoted records manifest hash mismatch.' }
        Update-StreMatrixPackaging $Config $inventory
        $packagingAfter=@(Get-StreMatrixPackaging $packagingPath | Sort-Object)
        if (($packagingAfter -join '|') -cne ($packagingExpected -join '|') -or
            @($packagingAfter | Where-Object { $_ -match '(?i)\.vortex_backup$' }).Count) { throw 'Packaging manifest differs from exact imported inventory.' }
        $allowed+=@($recordTarget,(Join-Path $Config.RepoRoot 'GameFiles/STRE_AlternateStart.manifest.txt'))
        Assert-StreMatrixUnchanged $State $allowed
        Assert-StreMatrixPromoted $Config $inventory $snapshot
        $State.State='Imported'
        Write-StreJson (Join-Path $Root 'session.json') $State
        @{Success=$true;State='Imported';Session=$Root;PreparationHead=$State.PreparationHead;ImportHead=$State.ImportHead;
          Sha256=$inventory.PluginSha256;ImportedFiles=$inventory.Files.Count;Companions=($inventory.Files.Count-1)}
    } catch {
        $State.State='ImportFailed'
        Write-StreJson (Join-Path $Root 'session.json') $State
        throw
    }
}
