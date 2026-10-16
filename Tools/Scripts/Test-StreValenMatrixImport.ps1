#requires -Version 7.2
# Real disposable Git histories and CK bridge; simulated audit/CI and invented bytes.
# Binary/receipt/PCM rejection is independently exercised by test_valen_matrix_import.py.
$ErrorActionPreference='Stop'
$source=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$module=Import-Module (Join-Path $PSScriptRoot 'Stre.Modding.psm1') -Force -PassThru
$temporaryParent=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$name='stre-matrix-promotion-'+[guid]::NewGuid().ToString('N')
$testRoot=[IO.Path]::GetFullPath((Join-Path $temporaryParent $name))
$null=New-Item -ItemType Directory -Path $testRoot
try {
    & $module {
        param($TestRoot,$Source)
        $script:passed=0
        function Assert($condition,$message) { if(-not $condition){throw $message} }
        function Reject($body,$pattern) {
            try { & $body | Out-Null } catch {
                if($_.Exception.Message -notmatch $pattern){throw "Expected $pattern; got $($_.Exception.Message)"}
                $script:passed++; return
            }
            throw "Expected rejection: $pattern"
        }
        function script:Assert-GameProcessesStopped { }
        function script:Assert-StreMatrixCI($Config,$Head) {
            if($script:ci -ne 'success'){throw 'CI must be green on exact import HEAD.'}
            @(@{name='Build windows';headSha=$Head;conclusion='success'},@{name='Build linux';headSha=$Head;conclusion='success'})
        }
        function script:Get-StreMatrixImportInventory($Config,$Root) {
            # Deliberately simulated content audit for orchestration tests only.
            $result=Get-Content (Join-Path $Root 'fixture-inventory.json') -Raw | ConvertFrom-Json -AsHashtable
            foreach($p in $result.Files.Keys){
                if((Get-StreHash (Join-Path $Root "Data/$p")) -cne $result.Files[$p]){throw 'Fixture candidate/companion changed'}
            }
            $result
        }
        function script:Assert-StreFormat { }
        function script:Get-StreHeader { @{} }
        function New-Fixture {
            $repo=Join-Path $TestRoot ([guid]::NewGuid().ToString('N'))
            $null=New-Item -ItemType Directory -Path "$repo/GameFiles/Skyrim","$repo/game/Data","$repo/Tools/Scripts","$repo/docs/features/alternate-start"
            Copy-Item -LiteralPath (Join-Path $Source 'Tools/Scripts/Invoke-StreMatrixCkImport.ps1') -Destination "$repo/Tools/Scripts"
            [IO.File]::WriteAllText("$repo/.gitignore","_audit/`ngame/`n")
            foreach($relative in @('STRE_AlternateStart.esp','Source/Scripts/QF_fixture.psc','Scripts/QF_fixture.pex','Scripts/untouched.pex','SkyrimTogether.esp')){
                foreach($base in @("$repo/GameFiles/Skyrim","$repo/game/Data")){
                    $p=Join-Path $base $relative
                    $null=New-Item -ItemType Directory -Path (Split-Path -Parent $p) -Force
                    [IO.File]::WriteAllText($p,"original fixture $relative")
                }
            }
            [IO.File]::WriteAllText("$repo/GameFiles/STRE_AlternateStart.manifest.txt","STRE_AlternateStart.esp`n")
            [IO.File]::WriteAllText("$repo/docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json",'original manifest fixture')
            @'
param([string[]]$RelativePath,[string]$SourceDataRoot)
$ErrorActionPreference='Stop'
foreach($p in $RelativePath){
 $destination=Join-Path $PSScriptRoot "GameFiles/Skyrim/$p"
 $null=New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force
 Copy-Item -LiteralPath (Join-Path $SourceDataRoot $p) -Destination $destination -Force
}
'@ | Set-Content "$repo/import-from-ck-dev.ps1"
            @'
throw 'The deployment entry point must NEVER execute during import.'
function Read-ManagedFileManifest { param([string]$ManifestPath) @(Get-Content -LiteralPath $ManifestPath) }
function Add-NewManagedFilesToManifest {
 param([string]$ManifestPath,[string[]]$RelativeFiles)
 @(@(Read-ManagedFileManifest $ManifestPath)+$RelativeFiles | Sort-Object -Unique) | Set-Content -LiteralPath $ManifestPath
}
'@ | Set-Content "$repo/build-and-deploy-dev.ps1"
            $script:config=@{RepoRoot=$repo;DataRoot="$repo/game/Data";ImportScript="$repo/import-from-ck-dev.ps1";XEditPath="$repo/xedit.fixture";AutoModRoot="$repo/tools"}
            $null=Invoke-StreGit $config @('init','-b','feat/fixture')
            $null=Invoke-StreGit $config @('config','user.email','fixture@example.invalid')
            $null=Invoke-StreGit $config @('config','user.name','Matrix fixture')
            $null=Invoke-StreGit $config @('add','.')
            $null=Invoke-StreGit $config @('commit','-m','test: preparation source')
            $null=Invoke-StreGit $config @('update-ref','refs/remotes/origin/main','HEAD')
            $head=Invoke-StreGit $config @('rev-parse','HEAD')
            $script:root=Join-Path $repo ('_audit/modding/'+[guid]::NewGuid().ToString('N'))
            $null=New-Item -ItemType Directory -Path "$root/Data"
            $originals=@{}; $protected=@{}
            foreach($p in @('STRE_AlternateStart.esp','Source/Scripts/QF_fixture.psc','Scripts/QF_fixture.pex','Scripts/untouched.pex')){
                $originals[$p]=Get-StreHash "$repo/GameFiles/Skyrim/$p"
            }
            foreach($p in @('STRE_AlternateStart.esp','Source/Scripts/QF_fixture.psc','Scripts/QF_fixture.pex','Scripts/untouched.pex','SkyrimTogether.esp')){
                foreach($base in @("$repo/GameFiles/Skyrim","$repo/game/Data")){
                    $path=Resolve-StrePath (Join-Path $base $p)
                    $protected[$path]=Get-StreHash $path
                }
            }
            foreach($p in @('GameFiles/STRE_AlternateStart.manifest.txt','docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json')){
                $path=Resolve-StrePath (Join-Path $repo $p); $protected[$path]=Get-StreHash $path
            }
            $files=@{}
            foreach($p in @('STRE_AlternateStart.esp','Source/Scripts/QF_fixture.psc','Scripts/QF_fixture.pex','Scripts/STRE_Observer.pex','Sound/Voice/STRE_AlternateStart.esp/MaleWarlock/test.wav')){
                $path=Join-Path $root "Data/$p"
                $null=New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force
                [IO.File]::WriteAllText($path,"reviewed fixture $p")
                $files[$p]=Get-StreHash $path
            }
            $script:state=@{Schema=1;State='AwaitingHumanXEdit';Plugin='STRE_AlternateStart.esp';Config=$config;Header=@{};
                Git=@{Branch='feat/fixture';Head=$head};Request=@{Operation='AddValenDialogueMatrixV1'};
                Protected=$protected;PrivateMasters=@{};PrivateOriginals=$originals}
            Write-StreJson "$root/session.json" $state
            $script:sessionHash=Get-StreHash "$root/session.json"
            [IO.File]::WriteAllText("$root/import-evidence.json",'SIMULATED checksum evidence')
            [IO.File]::WriteAllText("$root/review-proposed-record-manifest.json",'reviewed manifest fixture')
            Write-StreJson "$root/fixture-inventory.json" @{success=$true;Files=$files;PreparationHead=$head;PluginSha256=$files['STRE_AlternateStart.esp'];
                ManifestPath='docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json';ManifestSha256=(Get-StreHash "$root/review-proposed-record-manifest.json")}
            $script:ci='success'
        }
        function Import-Fixture { Import-StreValidated $config $root }
        function Check-NoPromotion {
            Assert (-not (Test-Path "$root/validated")) 'Rejection created snapshot'
            Assert ((Get-StreHash "$root/session.json") -ceq $script:sessionHash) 'Rejection rewrote session'
        }
        New-Fixture
        $prepared=$state.Git.Head
        $null=Invoke-StreGit $config @('commit','--allow-empty','-m','test: reviewed importer descendant')
        $importHead=Invoke-StreGit $config @('rev-parse','HEAD')
        $result=Import-Fixture
        Assert ($result.State -ceq 'Imported' -and $result.ImportHead -ceq $importHead -and $result.PreparationHead -ceq $prepared -and $prepared -cne $importHead) 'Descendant provenance failed'
        Assert ((Get-StreHash "$root/preparation-session.json") -ceq $script:sessionHash) 'Historical session not preserved'
        Assert (@(Get-ChildItem "$root/validated" -File -Recurse).Count -eq 5) 'Snapshot includes non-delta files'
        $inventory=Get-Content "$root/fixture-inventory.json" -Raw | ConvertFrom-Json -AsHashtable
        Assert-StreMatrixPromoted $config $inventory "$root/validated"
        foreach($p in @('Scripts/untouched.pex','SkyrimTogether.esp')){
            foreach($base in @($config.DataRoot,"$($config.RepoRoot)/GameFiles/Skyrim")){
                $path=Resolve-StrePath (Join-Path $base $p)
                Assert ((Get-StreHash $path) -ceq $state.Protected[$path]) 'Out-of-scope file changed'
            }
        }
        $script:passed++
        Reject { Import-Fixture } 'no reimport'
        New-Fixture
        $null=Invoke-StreGit $config @('branch','-m','feat/other')
        Reject { Import-Fixture } 'branch changed'; Check-NoPromotion
        New-Fixture
        # Keep origin/main ancestor, but make the recorded preparation a sibling commit.
        $null=Invoke-StreGit $config @('checkout','-b','feat/sibling')
        $null=Invoke-StreGit $config @('commit','--allow-empty','-m','test: unrelated preparation')
        $state.Git.Head=Invoke-StreGit $config @('rev-parse','HEAD')
        $null=Invoke-StreGit $config @('checkout','feat/fixture')
        Write-StreJson "$root/session.json" $state; $script:sessionHash=Get-StreHash "$root/session.json"
        Reject { Import-Fixture } 'must be an ancestor'; Check-NoPromotion
        foreach($ciState in @('missing','failure')){
            New-Fixture; $script:ci=$ciState
            Reject { Import-Fixture } 'CI must be green'; Check-NoPromotion
        }
        foreach($baseKind in @('Data','Git')){
            New-Fixture
            $base=if($baseKind -eq 'Data'){$config.DataRoot}else{"$($config.RepoRoot)/GameFiles/Skyrim"}
            [IO.File]::AppendAllText((Join-Path $base 'Scripts/QF_fixture.pex'),'drift')
            Reject { Import-Fixture } 'dirty|changed'; Check-NoPromotion
        }
        New-Fixture
        [IO.File]::WriteAllText((Join-Path $config.DataRoot 'Scripts/STRE_Observer.pex'),'collision')
        Reject { Import-Fixture } 'already exists'; Check-NoPromotion
        New-Fixture
        [IO.File]::AppendAllText("$root/Data/STRE_AlternateStart.esp",'drift')
        Reject { Import-Fixture } 'candidate/companion changed'; Check-NoPromotion
        New-Fixture
        $state.State='ImportFailed'; Write-StreJson "$root/session.json" $state
        Reject { Import-Fixture } 'no reimport'
        Write-Output "PASS: $script:passed matrix import orchestration cases"
    } $testRoot $source
} finally {
    if([IO.Path]::GetFileName($testRoot) -cne $name -or
       [IO.Path]::GetFullPath((Split-Path -Parent $testRoot)).TrimEnd('\') -cne $temporaryParent.TrimEnd('\')) { throw 'Unsafe fixture cleanup path.' }
    Remove-Item -LiteralPath $testRoot -Recurse -Force
}
