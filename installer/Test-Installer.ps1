<#
.SYNOPSIS
    The installer's smoke test: install Chess++ silently, start the installed app on a few scenes,
    uninstall it. Run by release.yml on every PR and release tag, after the installer is built.

.DESCRIPTION
    The UI tests run the app's code with all of Qt at hand; they can't see an installer that lacks
    something (a Qt DLL, a plugin, a QML module windeployqt missed). This test runs what a user gets:

      1. ChessSetup-*.exe /VERYSILENT into a temporary folder (no desktop icon, no Stockfish download).
      2. The installed ChessGui.exe in screenshot mode (its own fresh profile) on the scenes below, with
         Qt taken out of PATH and Qt's plugin and QML paths unset, so it finds only what was installed.
         Each scene must exit with 0 (1: grab failed, 2: scene timed out, 3: QML failed to load, e.g. a
         module not installed; a missing DLL or a crash: a Windows code), leave a picture that isn't
         blank, and log no warning (a missing multimedia backend only warns). The robot scene plays a
         game against FakeUci from the build (the installer has no engine of its own): the installed app
         can start an engine.
      3. The files Lichess needs that no scene loads (the TLS backend: HTTPS).
      4. The uninstaller, silently: the folder and its Add/Remove Programs entry gone.

    Pictures and logs: test-results\installer\. Exit code: the number of failed checks.

    It installs over and then removes a Chess++ already on the machine (the same AppId), so with an
    installer it refuses to run where Chess++ is installed: it is for GitHub's machines. -AppDir tests a
    folder instead (e.g. windeployqt's deploy\), without installing anything.

.EXAMPLE
    .\installer\Test-Installer.ps1 -Installer installer\Output\ChessSetup-pr-160.exe

.EXAMPLE
    .\installer\Test-Installer.ps1 -AppDir deploy
    The scenes and file checks on a deploy folder; nothing installed.
#>
[CmdletBinding()]
param(
    # The installer to test (default: the newest installer\Output\ChessSetup-*.exe)
    [string]$Installer,
    # Test this folder as the installed app instead (no install, no uninstall)
    [string]$AppDir,
    # The stand-in engine for the robot scene
    [string]$FakeUci,
    [string]$OutDir,
    # Seconds one scene may take
    [int]$SceneTimeout = 120
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $FakeUci) { $FakeUci = Join-Path $root 'x64\Release\FakeUci.exe' }
if (-not $OutDir) { $OutDir = Join-Path $root 'test-results\installer' }
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
Get-ChildItem $OutDir -File | Remove-Item -Force   # the last run's pictures and logs

# ChessSetup.iss's AppId: its Add/Remove Programs entry is "<AppId>_is1".
$uninstallKey = '{F3A8B2C1-5D47-4E89-B123-9A0C1D2E3F4A}_is1'
$uninstallRoots = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',
                  'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall',
                  'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall'
function Test-Registered { [bool]($uninstallRoots | Where-Object { Test-Path (Join-Path $_ $uninstallKey) }) }

$failed = 0
function Pass([string]$what) { Write-Host "OK    $what" }
function Fail([string]$what) { Write-Host "FAIL  $what" -ForegroundColor Red; $script:failed++ }

# ── 1. Install ──────────────────────────────────────────────────────────
$installed = $false
if ($AppDir) {
    $AppDir = (Resolve-Path $AppDir).Path
    Write-Host "Testing $AppDir (no install)"
} else {
    if (-not $Installer) {
        $Installer = Get-ChildItem (Join-Path $root 'installer\Output') -Filter 'ChessSetup-*.exe' -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
        if (-not $Installer) { throw 'No installer\Output\ChessSetup-*.exe: build one first, or pass -Installer' }
    }
    $Installer = (Resolve-Path $Installer).Path
    if (Test-Registered) {
        throw 'Chess++ is installed on this machine: this test would install over it and then remove it. ' +
              'Run it where Chess++ is not installed (GitHub), or test a folder with -AppDir.'
    }
    $AppDir = Join-Path ([IO.Path]::GetTempPath()) ('chess-installer-test-' + [guid]::NewGuid().ToString('N'))
    Write-Host "Installing $Installer into $AppDir"
    $setup = Start-Process $Installer -Wait -PassThru -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-',
        "/DIR=`"$AppDir`"", '/MERGETASKS="!desktopicon,!stockfish"', "/LOG=`"$OutDir\install.log`"")
    $installed = $true
    if ($setup.ExitCode -ne 0) { Fail "install - setup exit code $($setup.ExitCode) (see install.log)" }
    elseif (-not (Test-Path "$AppDir\ChessGui.exe")) { Fail "install - no ChessGui.exe in $AppDir" }
    elseif (-not (Test-Registered)) { Fail 'install - no Add/Remove Programs entry' }
    else { Pass 'install' }
}

# ── 2. The installed app's scenes ───────────────────────────────────────
# Only what was installed: Qt's own folders (install-qt-action puts them on PATH and sets its plugin
# and QML paths) would hide a file the installer lacks. The shell's values come back at the end.
$envNames = 'PATH', 'QT_PLUGIN_PATH', 'QML2_IMPORT_PATH', 'QML_IMPORT_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH',
            'CHESS_ENGINE_PATH', 'FAKEUCI_MOVES', 'FAKEUCI_DELAY_MS'
$savedEnv = @{}
foreach ($n in $envNames) { $savedEnv[$n] = [Environment]::GetEnvironmentVariable($n, 'Process') }
function Set-Env([string]$name, [string]$value) { [Environment]::SetEnvironmentVariable($name, $value, 'Process') }

# A DLL missing makes Windows show "... was not found" and wait for a click, which no one gives on
# GitHub: the error mode, inherited by the processes this one starts, makes the app fail at once.
Add-Type -Namespace ChessTest -Name Native -MemberDefinition '[DllImport("kernel32.dll")] public static extern uint SetErrorMode(uint mode);'
[void][ChessTest.Native]::SetErrorMode(0x0001 -bor 0x0002)   # SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX

# A picture with something drawn on it: more than a few colours over a grid of points.
Add-Type -AssemblyName System.Drawing
function Test-Drawn([string]$png) {
    $bitmap = [System.Drawing.Bitmap]::FromFile($png)
    try {
        $colours = @{}
        for ($i = 1; $i -lt 24; $i++) {
            for ($j = 1; $j -lt 24; $j++) {
                $colours[$bitmap.GetPixel([int]($bitmap.Width * $i / 24), [int]($bitmap.Height * $j / 24)).ToArgb()] = 1
            }
        }
        return $colours.Count -ge 8
    } finally { $bitmap.Dispose() }
}

# The app's own exit codes in screenshot mode, and Windows' for a process that didn't run.
$exitMeaning = @{ '0x00000001' = 'grabbing or saving the window failed'; '0x00000002' = 'timed out waiting for the scene';
                  '0x00000003' = 'QML failed to load (a module not installed?)'; '0x00000004' = 'unknown scene';
                  '0xC0000135' = 'a DLL is missing'; '0xC0000005' = 'it crashed (access violation)';
                  '0xC000007B' = 'a DLL of the wrong architecture'; '0xC0000409' = 'it crashed (stack buffer overrun)' }

# The robot scene: White mates with Qh5# (as in Take-Screenshot.ps1).
$scenes = [ordered]@{ 'home' = $null; 'game' = $null; 'settings' = $null; 'online' = $null;
                      'robot' = 'e2e4 f7f6 d2d4 g7g5 d1h5' }
try {
    if (Test-Path "$AppDir\ChessGui.exe") {
        Set-Env 'PATH' (($savedEnv['PATH'] -split ';' | Where-Object { $_ -and -not (Test-Path (Join-Path $_ 'Qt6Core.dll')) }) -join ';')
        foreach ($n in 'QT_PLUGIN_PATH', 'QML2_IMPORT_PATH', 'QML_IMPORT_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH') { Set-Env $n $null }
        foreach ($scene in $scenes.Keys) {
            $engineLine = $scenes[$scene]
            if ($null -ne $engineLine) {
                if (-not (Test-Path $FakeUci)) { Fail "$scene - $FakeUci not found (build FakeUci)"; continue }
                Set-Env 'CHESS_ENGINE_PATH' $FakeUci
                Set-Env 'FAKEUCI_MOVES' $engineLine
                Set-Env 'FAKEUCI_DELAY_MS' '0'
            } else {
                foreach ($n in 'CHESS_ENGINE_PATH', 'FAKEUCI_MOVES', 'FAKEUCI_DELAY_MS') { Set-Env $n $null }
            }
            $png = Join-Path $OutDir "$scene.png"
            $app = Start-Process "$AppDir\ChessGui.exe" -PassThru -ArgumentList '--screenshot', "`"$png`"", '--scene', $scene
            $handle = $app.Handle   # keeps the exit code readable after the process is gone
            if (-not $app.WaitForExit($SceneTimeout * 1000)) {
                try { & taskkill.exe /T /F /PID $app.Id 2>&1 | Out-Null } catch { }
                Fail "$scene - still running after $SceneTimeout s"
                continue
            }
            $log = if (Test-Path "$png.log") { @(Get-Content "$png.log") } else { @() }
            $warnings = @($log | Where-Object { $_ -match '\[(WARN |CRIT |FATAL)\]' })
            if ($app.ExitCode -ne 0) {
                $code = '0x{0:X8}' -f $app.ExitCode
                $why = if ($exitMeaning.ContainsKey($code)) { $exitMeaning[$code] } else { 'see its log' }   # (no ?? in 5.1)
                Fail "$scene - exit code $($app.ExitCode) ($code): $why"
                # Not one of the app's own codes (1-4): it didn't run (a DLL missing, a crash), and the
                # other scenes would only fail the same way.
                if ($app.ExitCode -lt 1 -or $app.ExitCode -gt 4) {
                    ($log | Select-Object -Last 10) | ForEach-Object { Write-Host "      $_" }
                    $rest = @($scenes.Keys)[([array]::IndexOf(@($scenes.Keys), $scene) + 1)..($scenes.Count)] | Where-Object { $_ }
                    if ($rest) { Fail "$($rest -join ', ') - not run: the app didn't start" }
                    break
                }
            } elseif (-not (Test-Path $png)) {
                Fail "$scene - no picture"
            } elseif (-not (Test-Drawn $png)) {
                Fail "$scene - the picture is blank"
            } elseif ($warnings.Count -gt 0) {
                Fail "$scene - the log has warnings"
            } else {
                Pass $scene
                continue
            }
            ($warnings + ($log | Select-Object -Last 10)) | Select-Object -Unique | ForEach-Object { Write-Host "      $_" }
        }
    } else {
        Fail "scenes - no ChessGui.exe in $AppDir"
    }
} finally {
    foreach ($n in $envNames) { Set-Env $n $savedEnv[$n] }
}

# ── 3. Files no scene loads ─────────────────────────────────────────────
# Lichess is HTTPS: without a TLS backend every request fails, though the app starts fine.
foreach ($file in 'LiChessConnector.dll', 'tls\qschannelbackend.dll') {
    if (Test-Path (Join-Path $AppDir $file)) { Pass $file } else { Fail "$file - not installed" }
}

# ── 4. Uninstall ────────────────────────────────────────────────────────
if ($installed) {
    $uninstaller = Get-ChildItem $AppDir -Filter 'unins*.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $uninstaller) {
        Fail 'uninstall - no uninstaller in the folder'
    } else {
        $un = Start-Process $uninstaller.FullName -Wait -PassThru -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
        # It runs on from a copy of itself in %TEMP%: wait for what it removes.
        $deadline = (Get-Date).AddSeconds(60)
        while (((Test-Path "$AppDir\ChessGui.exe") -or (Test-Registered)) -and (Get-Date) -lt $deadline) {
            Start-Sleep -Milliseconds 500
        }
        if ($un.ExitCode -ne 0) { Fail "uninstall - exit code $($un.ExitCode)" }
        elseif (Test-Path "$AppDir\ChessGui.exe") { Fail 'uninstall - ChessGui.exe still there' }
        elseif (Test-Registered) { Fail 'uninstall - the Add/Remove Programs entry is still there' }
        else { Pass 'uninstall' }
    }
    Remove-Item $AppDir -Recurse -Force -ErrorAction SilentlyContinue   # what the uninstaller leaves (none expected)
}

Write-Host "$failed failed. Pictures and logs: $OutDir"
exit $failed
