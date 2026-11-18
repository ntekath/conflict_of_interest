param([ValidateSet('Editor','Game')][string]$Target='Editor',[string]$EngineRoot=$env:COI_ENGINE_ROOT,[ValidateSet('Development','Shipping')][string]$Configuration='Development')
$ErrorActionPreference='Stop'
if(!$EngineRoot){$EngineRoot=(Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds').'{E5BA469E-467B-C42B-89EF-82A32B4E5A82}'}
if(!$EngineRoot -or !(Test-Path "$EngineRoot\Engine\Build\Build.version")){throw 'Set COI_ENGINE_ROOT or pass -EngineRoot (e.g. E:\UE_5.8).'}
$project=Join-Path (Split-Path $PSScriptRoot -Parent) 'ConflictOfInterest.uproject'
$engineVersion=Get-Content "$EngineRoot\Engine\Build\Build.version" -Raw | ConvertFrom-Json
if($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8){throw 'This project targets UE 5.8.'}
$dotnet=Get-ChildItem "$EngineRoot\Engine\Binaries\ThirdParty\DotNet" -Recurse -Filter dotnet.exe | Where-Object FullName -Match 'win-x64' | Select-Object -First 1
if(!$dotnet){throw 'Bundled Windows dotnet not found.'}
if($Target -eq 'Editor' -and $Configuration -eq 'Shipping'){throw 'Shipping requires -Target Game.'}
$buildTarget=if($Target -eq 'Editor'){'ConflictOfInterestEditor'}else{'ConflictOfInterest'}
& $dotnet.FullName "$EngineRoot\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" $buildTarget Win64 $Configuration "-Project=$project" -NoHotReloadFromIDE -NoUBA -NoUBALocal -WaitMutex
if($LASTEXITCODE -ne 0){throw "Unreal build failed: $LASTEXITCODE"}
