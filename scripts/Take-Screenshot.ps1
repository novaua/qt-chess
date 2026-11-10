<#
.SYNOPSIS
    Screenshots of ChessGui views, without clicking through the app.

.DESCRIPTION
    Runs ChessGui.exe in screenshot mode (--screenshot, see main.cpp) once per scene. Each run uses
    a fresh temporary profile (no users, settings or saves of yours are read or changed), opens the
    scene, saves <OutDir>\<scene>[-black][-dark][-Name][-<size>].png and exits. The app window is transparent,
    has no taskbar button and ignores the mouse, so nothing appears on screen. Screenshots are at 100%
    scale (one pixel per QML pixel), whatever the monitor's scaling.

    Sanity gate: Qt's DLLs must be next to the exe or on PATH (checked first). If the first scene
    fails, or the app doesn't run at all (a missing DLL, a crash), the remaining scenes are skipped.

.EXAMPLE
    .\scripts\Take-Screenshot.ps1
    Home -> test-results\screens\home.png

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene game -Moves "f2f3 e7e5 g2g4 d8h4" -Name foolsmate
    Two-player game after Fool's mate, with the result card.

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene 'game@1920x1080', 'drag@1400x900@black'
    Scenes at their own window size (BL-13's player column): game-1920x1080.png, drag-black-1400x900.png.

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene all -Dark -Build
    Build Release first, then every scene in dark mode.
#>
[CmdletBinding()]
param(
    # welcome, create-user, profiles, home, continue, continue-online, game, leave-online, review, result, ended, moves, select, drag, settings, about, restore, levels,
    # profile, profile-local, profile-expired, import, online, online-empty, online-disconnected, computer, players, captures, or all
    # A scene can name its own window size and side: scene@WxH[@black], e.g. game@1920x1080 (all runs
    # the list in $sizedScenes after the others).
    # (not in all: robot, premove-robot, slides, takeback)
    # (login, menu and board: the older names of profiles, home and game)
    [string[]]$Scene = @('home'),
    # game scene: UCI moves to play first, e.g. "e2e4 e7e5"
    [string]$Moves,
    # game scene: play Black (board turned around)
    [switch]$Black,
    [switch]$Dark,
    # The app's high-contrast colours (Theme.highContrast). The Fluent controls keep the normal
    # look: they only follow Windows' own contrast theme.
    [switch]$HighContrast,
    # Window size WxH, e.g. 1400x900 (landscape shows the move list beside the board). Default 768x1054.
    [string]$Size,
    # Extra wait in ms before the screenshot (default 1000)
    [int]$Delay = 1000,
    # Suffix for the file name, e.g. -Name foolsmate -> game-foolsmate.png
    [string]$Name,
    [string]$OutDir = (Join-Path $PSScriptRoot '..\test-results\screens'),
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    # Build ChessGui first (MSBuild found with vswhere)
    [switch]$Build,
    # -platform offscreen: works on a locked screen (where a real window renders no frames and
    # animations freeze), but is always dark: the Fluent style can't switch its palette there.
    [switch]$Offscreen
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$allScenes = 'welcome', 'create-user', 'profiles', 'home', 'continue', 'continue-online', 'game', 'leave-online', 'review', 'result', 'ended', 'moves', 'select', 'drag',
             'settings', 'about', 'restore', 'levels', 'profile', 'profile-local', 'profile-expired', 'import', 'online', 'online-empty', 'online-disconnected', 'computer', 'players',
             'clock', 'clock-low', 'clock-flag', 'clock-flag-draw', 'clock-promotion', 'clock-continue', 'stalemate', 'players-many', 'clock-online', 'clock-correspondence', 'online-blitz', 'online-challenge',
             'premove', 'premove-played', 'premove-dropped', 'reconnecting', 'challenge', 'challenge-unplayable', 'challenge-ingame', 'online-challenges', 'online-challenges-decline',
             'captures', 'check', 'draw-offer', 'saved', 'home-message', 'messages', 'opponent-left', 'opponent-left-claim', 'opponent-left-turn',
             'opponent-left-error'
# BL-13: the game page in wide windows (the player column left of the board), and sizes that must
# keep the cards above and below the board.
$sizedScenes = 'game@1920x1080', 'game@1400x900', 'clock-online@1920x1080', 'captures@1400x900', 'review@1400x900',
               'ended@1400x900', 'result@1400x900', 'challenge-ingame@1400x900', 'drag@1400x900@black',
               'reconnecting@1400x900', 'check@1400x900', 'check@960x1040', 'draw-offer@1400x900', 'draw-offer@640x560', 'saved@1400x900', 'messages@1400x900', 'opponent-left@1400x900', 'opponent-left-claim@1400x900', 'opponent-left-claim@640x560', 'game@1100x900', 'game@960x1040', 'game@1000x560'
if ($Scene -contains 'all') { $Scene = $allScenes + $sizedScenes }
foreach ($s in $Scene) {
    $s = ($s -split '@')[0]
    if ($s -notin ($allScenes + 'robot', 'premove-robot', 'login', 'menu', 'board', 'slides', 'takeback')) { throw "Unknown scene '$s'. Scenes: $($allScenes -join ', '), all" }
}

if ($Build) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if (-not $msbuild) { throw 'MSBuild not found' }
    & $msbuild "$root\ChessCore.sln" /t:ChessGui "/p:Configuration=$Configuration" /p:Platform=x64 /m /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
}

$exe = "$root\x64\$Configuration\ChessGui.exe"
if (-not (Test-Path $exe)) { throw "$exe not found - build it first (-Build)" }
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = Resolve-Path $OutDir

# Without Qt's DLLs Windows shows a "DLL not found" dialog for every run: check once, before the first.
$qtCore = if ($Configuration -eq 'Debug') { 'Qt6Cored.dll' } else { 'Qt6Core.dll' }
$dllDirs = @(Split-Path $exe) + ($env:PATH -split ';' | Where-Object { $_ })
if (-not ($dllDirs | Where-Object { Test-Path (Join-Path $_ $qtCore) } | Select-Object -First 1)) {
    throw "$qtCore not found next to the exe or on PATH. Add Qt's bin first, e.g. " +
          "`$env:PATH = 'D:\Tools\Qt\6.12.0\msvc2022_64\bin;' + `$env:PATH"
}

$exitMeaning = @{ 1 = 'grabbing or saving the window failed'; 2 = 'timed out waiting for the scene';
                  3 = 'QML failed to load'; 4 = 'unknown scene' }
$failed = 0
$done = 0
foreach ($run in $Scene) {
    $parts = @($run -split '@')   # scene[@WxH[@black]]
    $s = $parts[0]
    $runSize = if ($parts.Count -gt 1) { $parts[1] } else { $Size }
    $runBlack = $Black -or ($parts -contains 'black')
    # Built step by step: Windows PowerShell 5.1 dropped the size from a (... | Where-Object) list.
    $nameParts = @($s)
    if ($runBlack) { $nameParts += 'black' }
    if ($Dark -or $Offscreen) { $nameParts += 'dark' }
    if ($HighContrast) { $nameParts += 'hc' }
    if ($Name) { $nameParts += $Name }
    if ($parts.Count -gt 1) { $nameParts += $parts[1] }
    $baseName = $nameParts -join '-'
    $file = Join-Path $OutDir "$baseName.png"
    Remove-Item $file -ErrorAction SilentlyContinue   # a failed scene leaves no old picture behind (Compare-Screenshots.ps1)
    $argList = @('--screenshot', "`"$file`"", '--scene', $s, '--delay', $Delay)
    if ($Offscreen) { $argList = @('-platform', 'offscreen') + $argList }
    if ($Moves) { $argList += '--moves', "`"$Moves`"" }
    if ($runBlack) { $argList += '--black' }
    if ($Dark)  { $argList += '--dark' }
    if ($HighContrast) { $argList += '--high-contrast' }
    if ($runSize) { $argList += '--size', $runSize }

    $p = Start-Process $exe -ArgumentList $argList -Wait -PassThru
    if ($p.ExitCode -eq 0) {
        Remove-Item "$file.log" -ErrorAction SilentlyContinue
        Write-Host "OK    $file"
    } else {
        $failed++
        Write-Host "FAIL  $s - exit code $($p.ExitCode): $($exitMeaning[$p.ExitCode])" -ForegroundColor Red
        if (Test-Path "$file.log") { Write-Host "Log: $file.log"; Get-Content "$file.log" | Select-Object -Last 20 }
    }
    $done++

    # Sanity gate: if the first scene fails, or the app didn't run at all (not one of its own exit
    # codes: a missing DLL, a crash), the other scenes would only fail the same way. Stop here.
    $appFailed = $p.ExitCode -ne 0 -and -not $exitMeaning.ContainsKey($p.ExitCode)
    if ($done -lt $Scene.Count -and $p.ExitCode -ne 0 -and ($done -eq 1 -or $appFailed)) {
        $skipped = $Scene.Count - $done
        Write-Host "STOP  $skipped more scene(s) not run: fix the failure above first" -ForegroundColor Red
        $failed += $skipped
        break
    }
}
exit $failed
