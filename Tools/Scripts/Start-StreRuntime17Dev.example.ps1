#requires -Version 7.2
# Copy this entry into the chosen checkout as build-and-deploy-dev.ps1.
# Keep a pre-existing personal Legacy body below the marker, when applicable.
[CmdletBinding(SupportsShouldProcess)]
param(
    [switch]$Runtime17TwoPc,
    [string]$Runtime17ConfigPath = (Join-Path $PSScriptRoot '_audit/runtime17-dev.local.json'),
    [ValidateSet('BuildDeploy','Verify','RestoreStaging','VerifyRestored')][string]$Runtime17Mode = 'BuildDeploy',
    [switch]$Legacy,
    [string]$Runtime17RepositoryRoot,
    [string]$Runtime17ReceiptRoot,
    [string]$Runtime17ExpectedCommit,
    [ValidateSet('Build','Deploy')][string]$Runtime17Phase = 'Build'
)
$ErrorActionPreference='Stop'
if ($Legacy) {
    if ($Runtime17TwoPc -or $Runtime17RepositoryRoot -or $WhatIfPreference -or
        @($PSBoundParameters.Keys | Where-Object { $_ -like 'Runtime17*' }).Count) {
        throw 'Legacy must be explicitly selected alone; it has no inert WhatIf route.'
    }
} elseif ($Runtime17RepositoryRoot) {
    # Historical lot-4 route is explicit only. It never acts as a fallback.
    if ($Runtime17TwoPc -or $WhatIfPreference -or $PSBoundParameters.ContainsKey('Runtime17ConfigPath') -or $PSBoundParameters.ContainsKey('Runtime17Mode')) {
        throw 'Choose the normal workflow or the explicit historical RepositoryRoot route, not both.'
    }
    $LASTEXITCODE=0
    & (Join-Path $PSScriptRoot '_audit/rt17-lot4-resume/Invoke-LocalRuntime17Dev.ps1') -RepositoryRoot $Runtime17RepositoryRoot -ReceiptRoot $Runtime17ReceiptRoot -ExpectedCommit $Runtime17ExpectedCommit -Phase $Runtime17Phase 2>&1 | ForEach-Object {
        if ($_ -is [System.Management.Automation.ErrorRecord]) { throw $_ }
        $_
    }
    $helperSucceeded=$?
    if (!$helperSucceeded -or $LASTEXITCODE -ne 0) { throw "Historical helper failed (exit $LASTEXITCODE)." }
    return
} else {
    if ($PSBoundParameters.ContainsKey('Runtime17Phase')) { throw 'Runtime17Phase requires the explicit historical RepositoryRoot route.' }
    if ($Runtime17Mode -eq 'BuildDeploy' -and $Runtime17ReceiptRoot) { throw 'BuildDeploy cannot resume a receipt; select a verification/restoration mode.' }
    if ($Runtime17Mode -ne 'BuildDeploy' -and !$Runtime17ReceiptRoot) { throw 'Resume requires Runtime17ReceiptRoot.' }
    if ($Runtime17Mode -in @('RestoreStaging','VerifyRestored') -and $Runtime17ExpectedCommit) { throw 'ExpectedCommit applies to BuildDeploy/Verify, not restoration.' }
    foreach ($localPath in @($PSScriptRoot,$Runtime17ConfigPath) + @($Runtime17ReceiptRoot | Where-Object { $_ })) {
        if (!$localPath -or ![IO.Path]::IsPathFullyQualified($localPath) -or $localPath.StartsWith('\\') -or $localPath.StartsWith('//')) { throw 'Select local script, configuration and receipt paths.' }
        $drive=Get-PSDrive -Name ([IO.Path]::GetPathRoot($localPath).TrimEnd(':','\')) -ErrorAction Stop
        if ($drive.DisplayRoot -like '\\*') { throw 'Mapped network paths cannot select the entry, configuration or receipt.' }
    }
    # Git -C anchors every query to the script, never to the caller's cwd.
    $selectedRoot=@(& git -C $PSScriptRoot rev-parse --show-toplevel)
    if ($LASTEXITCODE -ne 0 -or $selectedRoot.Count -ne 1) { throw 'The entry must belong to a local Git checkout.' }
    $selectedRoot=[IO.Path]::GetFullPath($selectedRoot[0])
    $selectedBranch=(& git -C $selectedRoot branch --show-current) -join ''
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read the entry checkout branch.' }
    $selectedCommit=(& git -C $selectedRoot rev-parse HEAD) -join ''
    if ($LASTEXITCODE -ne 0 -or $selectedCommit -cnotmatch '^[a-f0-9]{40}$') { throw 'Cannot read the entry checkout commit.' }
    Write-Host "Entry checkout: $selectedRoot; branch=$selectedBranch; commit=$selectedCommit; mode=$Runtime17Mode"
    $helper=Join-Path $selectedRoot 'Tools/Scripts/Invoke-StreRuntime17Dev.ps1'
    if (!(Test-Path -LiteralPath $helper -PathType Leaf)) { throw "Selected checkout lacks the Skyrim 1.7 helper: $helper. No fallback; update this checkout separately." }
    $invoke=@{ConfigPath=$Runtime17ConfigPath; Mode=$Runtime17Mode; ReceiptPath=$Runtime17ReceiptRoot; ExpectedCommit=$Runtime17ExpectedCommit; WhatIf=[bool]$WhatIfPreference}
    if ($Runtime17Mode -eq 'BuildDeploy') { $invoke.SourceRepository=$selectedRoot }
    # Resume deliberately loads this checkout's updated helper, but validates
    # the original config/receipt source context. It cannot select a new build.
    $LASTEXITCODE=0
    & $helper @invoke 2>&1 | ForEach-Object {
        if ($_ -is [System.Management.Automation.ErrorRecord]) { throw $_ }
        $_
    }
    $helperSucceeded=$?
    if (!$helperSucceeded -or $LASTEXITCODE -ne 0) { throw "Runtime17 helper failed (exit $LASTEXITCODE)." }
    return
}
# BEGIN PERSONAL LEGACY BODY
throw 'This standalone example has no Legacy implementation.'
