param(
    [string]$BuildScriptPath = (Join-Path (Split-Path -Parent $PSScriptRoot) '../build-and-deploy-dev.ps1')
)

$ErrorActionPreference = 'Stop'
# Test only real function definitions from the ignored workstation workflow.
# Never dot-source its build/deploy entry point or touch installed Data.
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $BuildScriptPath).Path, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Cannot parse the local build/deploy script.' }
foreach ($name in @('Get-RelativePathFromRoot', 'Get-CkPluginOwnedFiles',
        'Test-CkAutoImportPath', 'Read-ManagedFileManifest', 'Add-NewManagedFilesToManifest')) {
    $definition = $ast.Find({ param($node)
        $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name
    }, $false)
    if (-not $definition) { throw "Missing local helper: $name" }
    . ([scriptblock]::Create($definition.Extent.Text))
}

$passed = 0
function Assert-Case([string]$Name, [bool]$Condition) {
    if (-not $Condition) { throw "FAIL: $Name" }
    $script:passed++
    Write-Output "PASS: $Name"
}

$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
$fixture = [IO.Path]::GetFullPath((Join-Path $tempRoot ('stre-ck-collector-' + [guid]::NewGuid().ToString('N'))))
if (-not $fixture.StartsWith($tempRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid fixture root' }
$plugin = 'STRE_AlternateStart.esp'
$voiceRelative = "Sound\Voice\$plugin\MaleWarlock"
$voiceRoot = Join-Path $fixture $voiceRelative
$wavRelative = "$voiceRelative\foo.wav"
$backupRelative = "$voiceRelative\foo.wav.vortex_backup"
$upperRelative = "$voiceRelative\bar.wav.VoRtEx_BaCkUp"
try {
    New-Item -ItemType Directory -Path $voiceRoot -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $fixture $plugin), 'Collector fixture: existence only, not an ESP parser test.')
    # A valid mono 16-bit PCM WAV: 8 kHz, eight silent samples.
    $stream = [IO.File]::Create((Join-Path $fixture $wavRelative))
    $writer = [IO.BinaryWriter]::new($stream)
    try {
        $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
        $writer.Write([uint32]52)
        $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVEfmt '))
        $writer.Write([uint32]16)
        $writer.Write([uint16]1); $writer.Write([uint16]1)
        $writer.Write([uint32]8000); $writer.Write([uint32]16000)
        $writer.Write([uint16]2); $writer.Write([uint16]16)
        $writer.Write([Text.Encoding]::ASCII.GetBytes('data'))
        $writer.Write([uint32]16); $writer.Write([byte[]]::new(16))
    }
    finally { $writer.Dispose() }
    Copy-Item -LiteralPath (Join-Path $fixture $wavRelative) -Destination (Join-Path $fixture $backupRelative)
    Copy-Item -LiteralPath (Join-Path $fixture $wavRelative) -Destination (Join-Path $fixture $upperRelative)
    $suffixRelative = "$voiceRelative\foo.wav.vortex_backup.extra"
    $otherVoice = @('.fuz', '.lip', '.xwm') | ForEach-Object { "$voiceRelative\future$_" }
    foreach ($relative in @($suffixRelative) + $otherVoice) {
        [IO.File]::WriteAllText((Join-Path $fixture $relative), 'Opaque collector fixture')
    }
    $backupHash = (Get-FileHash -LiteralPath (Join-Path $fixture $backupRelative)).Hash
    $owned = @(Get-CkPluginOwnedFiles -SourceDataRoot $fixture -PluginFileName $plugin -ScriptPrefix 'STRE_')
    Assert-Case 'real WAV remains owned' ($wavRelative -in $owned)
    Assert-Case 'lowercase backup is not owned' ($backupRelative -notin $owned)
    Assert-Case 'mixed-case backup is not owned' ($upperRelative -notin $owned)
    Assert-Case 'only the exact suffix is excluded' ($suffixRelative -in $owned)
    Assert-Case 'future voice extensions remain owned' (@($otherVoice | Where-Object { $_ -notin $owned }).Count -eq 0)
    Assert-Case 'real WAV remains importable' (Test-CkAutoImportPath -RelativePath $wavRelative -PluginFileName $plugin)
    Assert-Case 'direct lowercase backup is not importable' (-not (Test-CkAutoImportPath -RelativePath $backupRelative -PluginFileName $plugin))
    Assert-Case 'direct mixed-case slash path is not importable' (-not (Test-CkAutoImportPath -RelativePath $upperRelative.Replace('\', '/') -PluginFileName $plugin))
    Assert-Case 'future voice extensions remain importable' (@($otherVoice | Where-Object { -not (Test-CkAutoImportPath -RelativePath $_ -PluginFileName $plugin) }).Count -eq 0)
    Assert-Case 'non-terminal backup substring remains importable' (Test-CkAutoImportPath -RelativePath $suffixRelative -PluginFileName $plugin)

    # Inject backups again to exercise the independent auto-import boundary.
    $autoImport = @((@($owned) + @($backupRelative, $upperRelative)) |
        Where-Object { Test-CkAutoImportPath -RelativePath $_ -PluginFileName $plugin } | Sort-Object -Unique)
    $manifest = Join-Path $fixture 'managed.manifest.txt'
    Add-NewManagedFilesToManifest -ManifestPath $manifest -RelativeFiles $autoImport
    $managed = @(Read-ManagedFileManifest -ManifestPath $manifest)
    Assert-Case 'workflow manifest contains WAV and no backup suffix' (($wavRelative -in $managed) -and
        @($managed | Where-Object { $_.EndsWith('.vortex_backup', [StringComparison]::OrdinalIgnoreCase) }).Count -eq 0)
    $manifestHash = (Get-FileHash -LiteralPath $manifest).Hash
    Add-NewManagedFilesToManifest -ManifestPath $manifest -RelativeFiles $autoImport
    Assert-Case 'repeated manifest update is byte-identical' ((Get-FileHash -LiteralPath $manifest).Hash -eq $manifestHash)
    Assert-Case 'fixture backups remain present and unchanged' (
        (Get-FileHash -LiteralPath (Join-Path $fixture $backupRelative)).Hash -eq $backupHash -and
        (Get-FileHash -LiteralPath (Join-Path $fixture $upperRelative)).Hash -eq $backupHash)
    Write-Output "RESULT: $passed passed, 0 failed (local collector fixtures only)."
}
finally {
    $cleanupTarget = [IO.Path]::GetFullPath($fixture)
    if (-not $cleanupTarget.StartsWith($tempRoot + '\stre-ck-collector-', [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup target' }
    if (Test-Path -LiteralPath $cleanupTarget) { Remove-Item -LiteralPath $cleanupTarget -Recurse -Force }
}
