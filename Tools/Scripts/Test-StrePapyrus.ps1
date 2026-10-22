#requires -Version 7.2
# Real compiler test only. Inspect compiler/assembler provenance and child effects first.
# Never calls CK, xEdit, a downloader, authoring adapters, or the production importer.
param(
    [string]$AutoModRoot = $env:STRE_AUTOMOD_ROOT,
    [string]$DataRoot = $env:STRE_SKYRIM_DATA,
    [Parameter(Mandatory)][string]$CompilerPath,
    [Parameter(Mandatory)][string]$FlagsPath,
    [Parameter(Mandatory)][string]$OutputRoot
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$module = Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
try {
    & $module {
        param($Repo, $AutoModRoot, $DataRoot, $CompilerPath, $FlagsPath, $OutputRoot)
        $config = Resolve-StreConfiguration -RepoRoot $Repo -AutoModRoot $AutoModRoot -DataRoot $DataRoot
        # Availability is observed through the supported wrapper before any compilation.
        $status = Invoke-StreModding -Config $config -Action PapyrusStatus
        $inputs = Get-StrePapyrusInputs $status $CompilerPath $FlagsPath
        $compiler = $inputs.Compiler; $flags = $inputs.Flags; $assembler = $inputs.Assembler
        $root = Resolve-StrePath $OutputRoot
        $allowed = (Resolve-StrePath (Join-Path $Repo '_audit')).TrimEnd('\') + '\'
        if (-not $root.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be a new local repository _audit subdirectory.' }
        $null = New-Item -ItemType Directory -Path $root -ErrorAction Stop
        $source = Join-Path $root 'source files'
        $output = Join-Path $root 'compiled PEX'
        $null = New-Item -ItemType Directory -Path $source, $output
        # Same precedence as the existing authoring adapter: private, project (two
        # layouts), installed SKSE-extended headers, installed Bethesda headers.
        $imports = @($source, (Join-Path $Repo 'GameFiles/Skyrim/Source/Scripts'),
            (Join-Path $Repo 'GameFiles/Skyrim/scripts/source'),
            (Join-Path $config.DataRoot 'scripts/source'), (Join-Path $config.DataRoot 'Source/Scripts'))
        foreach ($path in $imports) {
            $null = Resolve-StrePath $path
            if (-not (Get-Item -LiteralPath $path -ErrorAction Stop).PSIsContainer) { throw "Import directory missing: $path" }
        }
        $protected = @{}
        foreach ($path in @($compiler.Path, $flags.Path, $assembler.Path) + @(
            foreach ($import in $imports) { Get-ChildItem -LiteralPath $import -Filter '*.psc' -File | ForEach-Object FullName }
        )) { $protected[$path] = Get-StreHash $path }
        Write-StreJson (Join-Path $root 'inputs.json') @{
            Kind='SYNTHETIC CLI TEST; no CK/game validation'; Runtime=$config.RuntimeIdentity
            Compiler=$compiler; Assembler=$assembler; Flags=$flags; Imports=$imports; Protected=$protected
        }
        Write-StreJson (Join-Path $root 'papyrus-status.json') $status
        @'
Scriptname STRE_CliMinimal Hidden
Int Function Value() Global
    Return 7
EndFunction
'@ | Set-Content -LiteralPath (Join-Path $source 'STRE_CliMinimal.psc') -Encoding ascii
        @'
Scriptname STRE_CliImports extends Quest
STRE_HelgenContinuityController Property Controller Auto
Int Function Probe()
    Actor player = Game.GetPlayer()
    Return SKSE.GetVersion() + player.GetLevel()
EndFunction
'@ | Set-Content -LiteralPath (Join-Path $source 'STRE_CliImports.psc') -Encoding ascii
        try {
            $results = @(foreach ($name in @('STRE_CliMinimal', 'STRE_CliImports')) {
                Assert-GameProcessesStopped
                $arguments = @((Join-Path $source "$name.psc"), "-i=$($imports -join ';')", "-o=$output", "-f=$($flags.Path)")
                $result = Invoke-StreProcess $compiler.Path $arguments $root
                Write-StreJson (Join-Path $root "$name.compile.json") @{ Executable=$compiler.Path; Arguments=$arguments; Result=$result }
                Assert-StrePapyrusCompilation $result
                $pex = Join-Path $output "$name.pex"
                $bytes = [IO.File]::ReadAllBytes($pex)
                # Skyrim PEX 3.2 / game ID 1; independent of Skyrim EXE version.
                if ($bytes.Length -le 16 -or [Convert]::ToHexString($bytes[0..7]) -cne 'FA57C0DE03020001') {
                    throw "Unexpected or missing Skyrim PEX header: $pex"
                }
                @{ Source=Join-Path $source "$name.psc"; Pex=$pex; Sha256=Get-StreHash $pex; Bytes=$bytes.Length; Header='FA57C0DE03020001' }
            })
            if (@(Get-ChildItem -LiteralPath $output -File).Count -ne 2) { throw 'Unexpected compiler outputs' }
        }
        finally {
            foreach ($path in $protected.Keys) {
                if ((Get-StreHash $path) -cne $protected[$path]) { throw "Compiler input/tool changed: $path" }
            }
        }
        Write-StreJson (Join-Path $root 'result.json') @{ Success=$true; Compilations=$results; HumanCK='NOT EXECUTED'; GameLoading='NOT TESTED' }
        Write-Output "PASS: two real Papyrus compilations, two independent PEX outputs: $root"
    } $repo $AutoModRoot $DataRoot $CompilerPath $FlagsPath $OutputRoot
}
finally { Remove-Module $module -Force }
