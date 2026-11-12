<#
.SYNOPSIS
    The UI tests (BL-05 Layer 1): the app's pages played with the mouse by Qt Quick Test, offscreen.

.DESCRIPTION
    Runs ChessGuiTests.exe once per test file (ChessGuiTests\qml\tst_*.qml), each in its own process
    on a fresh temporary profile, with -platform offscreen: nothing shows on screen, and it works
    with the screen locked. Each file's results go to test-results\ui\<file>.txt (and .xml, JUnit);
    the failures are printed. See docs/UiTestSuitePlan.md.

    -Screens adds the screenshot check: every scene in light and dark (Take-Screenshot.ps1), then
    Compare-Screenshots.ps1 against your baselines. That is the full pre-PR check; it needs an
    unlocked screen (light pictures can't be taken offscreen) and PowerShell 7 for the comparison.

    A test file with a line "// ui-test-runs: 2" runs twice on the same data folder, as the app
    restarted (TestRun.run is 1, then 2); its results are <file>-run1.txt, <file>-run2.txt.

    A failed test function leaves a picture of the app's window: test-results\ui\<TestCase>-<function>.png
    (its path is printed with the failure).

    -Visible shows the run in a real window on the screen instead, a pause after each click or drag
    (-StepDelay), and after a failure the window stays up with a note until you close it.

    Sanity gate: Qt's DLLs must be next to the exe or on PATH (checked first).
    Exit code: the number of test files that failed (plus the failed screenshots with -Screens).

.EXAMPLE
    .\scripts\Run-UiTests.ps1 -Build
    Build ChessGuiTests (Release), then run every test file.

.EXAMPLE
    .\scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Function test_2_newGameAndDrag
    One file, one test function (the file's initTestCase and cleanupTestCase still run).

.EXAMPLE
    .\scripts\Run-UiTests.ps1 -Name tst_twoPlayers -Visible
    Watch it: a real window, half a second after each action; a failure waits for you to close it.

.EXAMPLE
    .\scripts\Run-UiTests.ps1 -Build -Screens
    The pre-PR check: the UI tests, then all screenshots compared with the baselines.
#>
[CmdletBinding()]
param(
    # Test files (names without .qml, wildcards allowed), e.g. tst_twoPlayers, tst_first*
    [string[]]$Name = @('tst_*'),
    # Only these test functions of the file(s), e.g. test_1_foolsMateByClicks
    [string[]]$Function = @(),
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    # Build ChessGuiTests first (MSBuild found with vswhere)
    [switch]$Build,
    # Also take every screenshot (light and dark) and compare them with the baselines
    [switch]$Screens,
    # A real window on the screen instead of offscreen, paced, waiting after a failure until closed
    [switch]$Visible,
    # The pause after each action in ms (default: 500 with -Visible, else 0)
    [int]$StepDelay = -1,
    # Seconds one test file may take before it is stopped and counted as failed (default 180;
    # with -Visible 1800, as a failure waits for you)
    [int]$Timeout = 0,
    [string]$OutDir = (Join-Path $PSScriptRoot '..\test-results\ui')
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$testsDir = Join-Path $root 'ChessGuiTests\qml'

if ($Build) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if (-not $msbuild) { throw 'MSBuild not found' }
    $targets = if ($Screens) { 'ChessGuiTests;ChessGui' } else { 'ChessGuiTests' }
    & $msbuild "$root\ChessCore.sln" "/t:$targets" "/p:Configuration=$Configuration" /p:Platform=x64 /m /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
}

$exe = "$root\x64\$Configuration\ChessGuiTests.exe"
if (-not (Test-Path $exe)) { throw "$exe not found - build it first (-Build)" }

# Without Qt's DLLs Windows shows a "DLL not found" dialog for every run: check once, before the first.
$qtCore = if ($Configuration -eq 'Debug') { 'Qt6Cored.dll' } else { 'Qt6Core.dll' }
$dllDirs = @(Split-Path $exe) + ($env:PATH -split ';' | Where-Object { $_ })
if (-not ($dllDirs | Where-Object { Test-Path (Join-Path $_ $qtCore) } | Select-Object -First 1)) {
    throw "$qtCore not found next to the exe or on PATH. Add Qt's bin first, e.g. " +
          "`$env:PATH = 'D:\Tools\Qt\6.12.0\msvc2022_64\bin;' + `$env:PATH"
}

if ($StepDelay -lt 0) { $StepDelay = if ($Visible) { 500 } else { 0 } }
if ($Timeout -le 0) { $Timeout = if ($Visible) { 1800 } else { 180 } }

# The results file, once readable: an engine the test started (FakeUci, still "thinking") may hold it
# a moment after the test process ends, as it inherited its handle.
function Read-Results([string]$path) {
    if (-not (Test-Path $path)) { return @() }
    for ($try = 0; $try -lt 50; $try++) {
        try { return @(Get-Content $path -ErrorAction Stop) } catch { Start-Sleep -Milliseconds 200 }
    }
    return @(Get-Content $path)
}

$files = @(Get-ChildItem $testsDir -Filter 'tst_*.qml' |
    Where-Object { $n = $_.BaseName; $Name | Where-Object { $n -like $_ } } | Sort-Object Name)
if ($files.Count -eq 0) { throw "No test files in $testsDir match $($Name -join ', ')" }

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = Resolve-Path $OutDir
# Read by the tests (TestSetup, TestRun in QML)
$env:CHESS_UI_TEST_RESULTS = "$OutDir"
$env:CHESS_UI_TEST_STEP_MS = "$StepDelay"
$env:CHESS_UI_TEST_WATCHED = if ($Visible) { '1' } else { '0' }
$failedFiles = 0
$started = Get-Date

# One run of a test file: its results' base name, the run number and the data folder (empty: its own).
function Invoke-TestRun($file, [string]$baseName, [int]$run, [string]$dataDir) {
    $txt = Join-Path $OutDir "$baseName.txt"
    $xml = Join-Path $OutDir "$baseName.xml"
    Remove-Item $txt, $xml -ErrorAction SilentlyContinue
    $env:CHESS_UI_TEST_RUN = "$run"
    $env:CHESS_UI_TEST_DATA_DIR = $dataDir
    # Qt Quick Test names a function <TestCase name>::<function>: the name comes from the file.
    $caseName = ([regex]::Match((Get-Content $file.FullName -Raw), '(?m)^\s*name:\s*"([^"]+)"')).Groups[1].Value
    $functions = @($Function | ForEach-Object { if ($_ -like '*::*' -or -not $caseName) { $_ } else { "${caseName}::$_" } })
    $argList = @('-input', "`"$($file.FullName)`"", '-o', "`"$txt,txt`"", '-o', "`"$xml,junitxml`"") + $functions
    if (-not $Visible) { $argList = @('-platform', 'offscreen') + $argList }
    $process = Start-Process $exe -ArgumentList $argList -WorkingDirectory $root -NoNewWindow -PassThru
    $handle = $process.Handle   # keeps the exit code readable after the process is gone
    if (-not $process.WaitForExit($Timeout * 1000)) {
        $process.Kill()
        $process.WaitForExit()
        Write-Host "FAIL  $baseName - stopped after $Timeout s" -ForegroundColor Red
        return $false
    }
    $code = $process.ExitCode
    $log = Read-Results $txt
    $totals = $log | Where-Object { $_ -like 'Totals:*' } | Select-Object -First 1
    if ($code -eq 0 -and $totals) {
        Write-Host "OK    $baseName - $($totals -replace '^Totals: ', '')"
        return $true
    }
    $why = if ($totals) { $totals -replace '^Totals: ', '' } else { "exit code $code, no results (see $txt)" }
    Write-Host "FAIL  $baseName - $why" -ForegroundColor Red
    # Each failure with its message and location (the lines after FAIL! up to the next result).
    $inFailure = $false
    foreach ($line in $log) {
        if ($line -like 'FAIL!*') { $inFailure = $true }
        elseif ($line -match '^(PASS|SKIP|QINFO|QDEBUG|QWARN|Totals|\*{9})') { $inFailure = $false }
        if ($inFailure -or $line -like 'QWARN*') { Write-Host "      $line" }
    }
    if ($caseName) {
        Get-ChildItem (Join-Path $OutDir "$caseName-*.png") -ErrorAction SilentlyContinue |
            ForEach-Object { Write-Host "      Picture: $($_.FullName)" }
    }
    return $false
}

foreach ($file in $files) {
    $text = Get-Content $file.FullName -Raw
    $caseName = ([regex]::Match($text, '(?m)^\s*name:\s*"([^"]+)"')).Groups[1].Value
    if ($caseName) { Remove-Item (Join-Path $OutDir "$caseName-*.png") -ErrorAction SilentlyContinue }   # the last run's failures
    $runs = [regex]::Match($text, '(?m)^\s*//\s*ui-test-runs:\s*(\d+)')
    $runCount = if ($runs.Success) { [int]$runs.Groups[1].Value } else { 1 }
    if ($runCount -le 1) {
        if (-not (Invoke-TestRun $file $file.BaseName 1 '')) { $failedFiles++ }
        continue
    }
    # The runs share one data folder: what the first leaves (settings, profiles, an autosaved game)
    # the next one finds, as after a restart. A failed run ends the file.
    $dataDir = Join-Path ([IO.Path]::GetTempPath()) ("chess-ui-test-" + [guid]::NewGuid().ToString('N'))
    try {
        for ($run = 1; $run -le $runCount; $run++) {
            if (-not (Invoke-TestRun $file "$($file.BaseName)-run$run" $run $dataDir)) { $failedFiles++; break }
        }
    } finally {
        Remove-Item $dataDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
Remove-Item Env:CHESS_UI_TEST_RUN, Env:CHESS_UI_TEST_DATA_DIR -ErrorAction SilentlyContinue

$seconds = [int]((Get-Date) - $started).TotalSeconds
Write-Host "$($files.Count) test files, $failedFiles failed, $seconds s. Results: $OutDir"
$failed = $failedFiles

if ($Screens) {
    Write-Host 'Screenshots (light, then dark), then the comparison with the baselines...'
    # Only what isn't OK: the scripts write their lines with Write-Host, so all streams are merged first.
    function Show-NotOk { process { $line = "$_"; if ($line -and $line -notlike 'OK*') { Write-Host $line } } }
    $take = Join-Path $PSScriptRoot 'Take-Screenshot.ps1'
    & $take -Scene all -Configuration $Configuration *>&1 | Show-NotOk
    $failed += $LASTEXITCODE
    & $take -Scene all -Dark -Configuration $Configuration *>&1 | Show-NotOk
    $failed += $LASTEXITCODE
    $compare = Join-Path $PSScriptRoot 'Compare-Screenshots.ps1'
    if ($PSVersionTable.PSVersion.Major -ge 7) { & $compare *>&1 | Show-NotOk }
    else { & pwsh -NoProfile -File $compare *>&1 | Show-NotOk }
    $failed += $LASTEXITCODE
}

exit $failed
