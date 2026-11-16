param([string]$EngineRoot=$env:COI_ENGINE_ROOT,[string]$Filter='CoI.')
$ErrorActionPreference='Stop'
if(!$EngineRoot){$EngineRoot=(Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds').'{E5BA469E-467B-C42B-89EF-82A32B4E5A82}'}
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'ConflictOfInterest.uproject'
$testLog=Join-Path $repo 'Saved/Logs/AP05_Automation.log'
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -unattended -nop4 -nosplash -nullrhi "-ExecCmds=Automation RunTests $Filter" '-TestExit=Automation Test Queue Empty' "-abslog=$testLog"
if($LASTEXITCODE -ne 0){throw "Unreal test process failed: $LASTEXITCODE"}
$results=Select-String -LiteralPath $testLog -Pattern 'Test Completed. Result='
if(!$results -or ($results | Where-Object Line -Match 'Result=\{Fail\}')){throw "Automation failed or no tests ran. See $testLog"}
$results | ForEach-Object {$_.Line}
Write-Output "Passed $($results.Count) tests. Log: $testLog"
