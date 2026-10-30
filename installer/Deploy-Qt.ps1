# Deploy-Qt.ps1
# Put the Qt files ChessGui.exe needs next to it (windeployqt), minus what the app never uses.
# Used by Build-Installer.ps1 and .github/workflows/release.yml.
#
#   deploy\ChessGui.exe, LiChessConnector.dll, Qt6*.dll, vc_redist.x64.exe
#   deploy\qt.conf          tells Qt where the rest is:
#   deploy\qt\plugins\      platform, image format, TLS, media... plugins
#   deploy\qt\qml\          QML modules
#
# Left out (pass -Full to keep them, e.g. to check whether a problem comes from this):
#   translations      the app loads no Qt translations
#   opengl32sw.dll    software OpenGL; Qt Quick draws with Direct3D 11
#   dxcompiler/dxil   Direct3D 12 shader compiler
#   FFmpeg            the sounds are WAVs, played by Qt's Windows media backend (main.cpp)
#   qmltooling        QML debugger; generic: touch input (TUIO)
#   Controls styles   all but FluentWinUI3 (main.cpp) and the ones it falls back to: Fusion, Basic
#
# Usage: .\installer\Deploy-Qt.ps1 deploy\ChessGui.exe [-Windeployqt <path>] [-WindeployqtVerbose 2]

param(
    [Parameter(Mandatory)][string]$Exe,
    [string]$Windeployqt = 'windeployqt.exe',
    [int]$WindeployqtVerbose = 1,
    [switch]$Full
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dir = Split-Path -Parent (Resolve-Path $Exe)

$arguments = @('--release', '--verbose', $WindeployqtVerbose, '--qmldir', "$root\ChessGui\ChessGui",
    '--plugindir', "$dir\qt\plugins", '--qml-deploy-dir', "$dir\qt\qml")
if (-not $Full) {
    $arguments += '--no-translations', '--no-opengl-sw', '--no-system-dxc-compiler', '--no-ffmpeg',
        '--exclude-plugins', 'ffmpegmediaplugin', '--skip-plugin-types', 'qmltooling,generic'
}
& $Windeployqt @arguments $Exe
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE" }

if (-not $Full) {
    # windeployqt deploys every Controls style; the app sets FluentWinUI3, which falls back to Fusion,
    # and Fusion to Basic (their qmldir files). Windows needs NativeStyle, so that goes too.
    $unused = @('Imagine', 'Material', 'Universal', 'Windows' | ForEach-Object {
            "$dir\qt\qml\QtQuick\Controls\$_"
            "$dir\Qt6QuickControls2$_.dll"
            "$dir\Qt6QuickControls2${_}StyleImpl.dll"
        }) + "$dir\qt\qml\QtQuick\NativeStyle" + "$dir\Qt6QuickNativeStyle.dll"
    foreach ($path in $unused) {
        if (Test-Path $path) { Remove-Item $path -Recurse -Force }
    }
    $left = Get-ChildItem $dir -Recurse -File -Include 'av*.dll', 'sw*.dll', 'opengl32sw.dll', 'dxcompiler.dll', 'dxil.dll', '*.qm'
    if ($left) { throw "Not left out as expected: $($left.Name -join ', ')" }
}

# Relative to the folder of qt.conf. Qt6*.dll stay next to the exe: Windows loads them from there.
Set-Content "$dir\qt.conf" -Encoding ascii -Value @'
[Paths]
Prefix = qt
Plugins = plugins
QmlImports = qml
'@

$files = Get-ChildItem $dir -Recurse -File | Where-Object Name -ne 'vc_redist.x64.exe'
Write-Host ("Deployed {0} files, {1:N1} MB (without vc_redist.x64.exe)" -f $files.Count, (($files | Measure-Object Length -Sum).Sum / 1MB))
