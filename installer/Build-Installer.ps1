# Build-Installer.ps1
# Build the Chess++ installer locally, mirroring the steps in .github/workflows/release.yml:
#   MSBuild Release|x64  ->  deploy\ (windeployqt)  ->  Inno Setup  ->  installer\Output\ChessSetup-<version>.exe
#
# The local installer is NOT code-signed (CI signs releases).
#
# Usage (from anywhere):
#   .\installer\Build-Installer.ps1
#   .\installer\Build-Installer.ps1 -Version 0.2.0
#   .\installer\Build-Installer.ps1 -SkipBuild        # reuse x64\Release binaries
#
# Parameters:
#   -Version    Product/installer version. Default: latest git tag + "-local" (e.g. 0.1.2-local).
#               Only major.minor.patch is stamped into the exe/dll version resources.
#   -SkipBuild  Skip the MSBuild step and package the existing x64\Release binaries.
#   -QtBinDir   Folder containing windeployqt.exe (default: found on PATH).

param(
    [string]$Version,
    [switch]$SkipBuild,
    [string]$QtBinDir
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Step($msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }

function Find-Tool($name, [string[]]$extraPaths) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($p in $extraPaths) {
        if ($p -and (Test-Path $p)) { return $p }
    }
    return $null
}

function Invoke-Native($exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$([IO.Path]::GetFileName($exe)) failed with exit code $LASTEXITCODE" }
}

# --- Version -----------------------------------------------------------------
if (-not $Version) {
    $tag = git -C $root describe --tags --abbrev=0 2>$null
    $Version = if ($LASTEXITCODE -eq 0 -and $tag) { ($tag -replace '^v', '') + '-local' } else { '0.0.0-local' }
}
if ($Version -notmatch '^\d+\.\d+\.\d+') { throw "Version '$Version' must look like MAJOR.MINOR.PATCH[-suffix]" }
$appVersion = ($Version -split '-')[0]
Write-Host "Version: $Version (binaries stamped $appVersion)"

# --- Locate tools ------------------------------------------------------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = $null
if (Test-Path $vswhere) {
    $msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild `
        -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
}
if (-not $msbuild) { $msbuild = Find-Tool 'msbuild.exe' @() }

$windeployqt = Find-Tool 'windeployqt.exe' @($(if ($QtBinDir) { Join-Path $QtBinDir 'windeployqt.exe' }))
if ($QtBinDir -and (Test-Path (Join-Path $QtBinDir 'windeployqt.exe'))) { $windeployqt = Join-Path $QtBinDir 'windeployqt.exe' }

$iscc = Find-Tool 'ISCC.exe' @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe")

if (-not $SkipBuild -and -not $msbuild) { throw "MSBuild not found (install Visual Studio 2022+ with C++ workload)" }
if (-not $windeployqt) { throw "windeployqt.exe not found: put the Qt bin folder on PATH or pass -QtBinDir" }
if (-not $iscc) { throw "Inno Setup 6 (ISCC.exe) not found: https://jrsoftware.org/isdl.php" }
Write-Host "MSBuild    : $msbuild"
Write-Host "windeployqt: $windeployqt"
Write-Host "ISCC       : $iscc"

Push-Location $root
try {
    # --- Build ---------------------------------------------------------------
    if ($SkipBuild) {
        Step "Skipping build (using existing x64\Release binaries)"
    } else {
        Step "Build ChessCore.sln Release|x64"
        Invoke-Native $msbuild @('ChessCore.sln', '/p:Configuration=Release', '/p:Platform=x64',
            "/p:AppVersion=$appVersion", '/m', '/v:minimal', '/nologo')
    }
    foreach ($f in 'x64\Release\ChessGui.exe', 'x64\Release\LiChessConnector.dll') {
        if (-not (Test-Path $f)) { throw "$f not found - run without -SkipBuild" }
    }

    # --- Collect deployment files ---------------------------------------------
    Step "Collect deployment files (deploy\)"
    if (Test-Path deploy) { Remove-Item deploy -Recurse -Force }
    New-Item -ItemType Directory deploy | Out-Null
    Copy-Item x64\Release\ChessGui.exe, x64\Release\LiChessConnector.dll deploy\
    Invoke-Native $windeployqt @('--release', '--qmldir', 'ChessGui\ChessGui', 'deploy\ChessGui.exe')

    # --- Verify version metadata ----------------------------------------------
    Step "Verify version metadata"
    foreach ($f in 'deploy\ChessGui.exe', 'deploy\LiChessConnector.dll') {
        $vi = (Get-Item $f).VersionInfo
        Write-Host "$f : $($vi.ProductName) $($vi.ProductVersion) / $($vi.CompanyName)"
        if ($vi.ProductName -ne 'Chess++') { throw "$f is missing product metadata" }
        if (-not $SkipBuild -and $vi.ProductVersion -ne "$appVersion.0") {
            throw "$f version '$($vi.ProductVersion)' does not match $appVersion"
        }
    }

    # --- Inno Setup (unsigned) -------------------------------------------------
    # /DNoSign=1 turns off the SignTool/SignedUninstaller directives in ChessSetup.iss.
    Step "Build installer with Inno Setup (unsigned)"
    $env:APP_VERSION = $Version
    Invoke-Native $iscc @('/DNoSign=1', 'installer\ChessSetup.iss')

    $out = Get-ChildItem installer\Output -Filter "ChessSetup-$Version.exe" | Select-Object -First 1
    if (-not $out) { throw "Installer not found in installer\Output" }
    Write-Host "`nInstaller: $($out.FullName)  ($([math]::Round($out.Length / 1MB, 1)) MB)" -ForegroundColor Green
}
finally {
    Pop-Location
    Remove-Item Env:\APP_VERSION -ErrorAction SilentlyContinue
}
