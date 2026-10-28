<#
.SYNOPSIS
    Screenshots of ChessGui views, without clicking through the app.

.DESCRIPTION
    Runs ChessGui.exe in screenshot mode (--screenshot, see main.cpp) once per scene. Each run uses
    a fresh temporary profile (no users, settings or saves of yours are read or changed), opens the
    scene, saves <OutDir>\<scene>[-black][-dark][-Name].png and exits. The app window is transparent,
    has no taskbar button and ignores the mouse, so nothing appears on screen. Screenshots are at 100%
    scale (one pixel per QML pixel), whatever the monitor's scaling.

.EXAMPLE
    .\scripts\Take-Screenshot.ps1
    Home -> test-results\screens\home.png

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene game -Moves "f2f3 e7e5 g2g4 d8h4" -Name foolsmate
    Two-player game after Fool's mate, with the result card.

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene all -Dark -Build
    Build Release first, then every scene in dark mode.
#>
[CmdletBinding()]
param(
    # create-user, login, home, continue, game, settings, levels, profile, online, computer, players, or all
    # (menu and board: the older names of home and game)
    [string[]]$Scene = @('home'),
    # game scene: UCI moves to play first, e.g. "e2e4 e7e5"
    [string]$Moves,
    # game scene: play Black (board turned around)
    [switch]$Black,
    [switch]$Dark,
    # Window size WxH, e.g. 1400x900 (landscape shows the move-history panel). Default 768x1054.
    [string]$Size,
    # Extra wait in ms before the screenshot (default 1000)
    [int]$Delay = 1000,
    # Suffix for the file name, e.g. -Name foolsmate -> game-foolsmate.png
    [string]$Name,
    [string]$OutDir = (Join-Path $PSScriptRoot '..\test-results\screens'),
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    # Build ChessGui first (MSBuild found with vswhere)
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$allScenes = 'create-user', 'login', 'home', 'continue', 'game', 'settings', 'levels', 'profile', 'online', 'computer', 'players'
if ($Scene -contains 'all') { $Scene = $allScenes }
foreach ($s in $Scene) {
    if ($s -notin ($allScenes + 'menu', 'board')) { throw "Unknown scene '$s'. Scenes: $($allScenes -join ', '), all" }
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

$exitMeaning = @{ 1 = 'grabbing or saving the window failed'; 2 = 'timed out waiting for the scene';
                  3 = 'QML failed to load'; 4 = 'unknown scene' }
$failed = 0
foreach ($s in $Scene) {
    $baseName = ($s, $(if ($Black) { 'black' }), $(if ($Dark) { 'dark' }), $Name | Where-Object { $_ }) -join '-'
    $file = Join-Path $OutDir "$baseName.png"
    $argList = @('--screenshot', "`"$file`"", '--scene', $s, '--delay', $Delay)
    if ($Moves) { $argList += '--moves', "`"$Moves`"" }
    if ($Black) { $argList += '--black' }
    if ($Dark)  { $argList += '--dark' }
    if ($Size)  { $argList += '--size', $Size }

    $p = Start-Process $exe -ArgumentList $argList -Wait -PassThru
    if ($p.ExitCode -eq 0) {
        Remove-Item "$file.log" -ErrorAction SilentlyContinue
        Write-Host "OK    $file"
    } else {
        $failed++
        Write-Host "FAIL  $s - exit code $($p.ExitCode): $($exitMeaning[$p.ExitCode])" -ForegroundColor Red
        if (Test-Path "$file.log") { Write-Host "Log: $file.log"; Get-Content "$file.log" | Select-Object -Last 20 }
    }
}
exit $failed
