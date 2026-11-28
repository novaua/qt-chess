<#
.SYNOPSIS
    LI-18: the online servers stay plugins. Checks that nothing but the plugin itself (and ChessQtTests,
    which compiles the classes it tests) uses a server plugin's code, so a plugin can be left out without
    breaking the app, the UI tests or another plugin.

.DESCRIPTION
    For each plugin folder (LiChessConnector: the Lichess plugin, LichessOnline.dll):
      - no source of ChessGui, ChessGuiTests, OnlineCore or another plugin includes one of its headers;
      - no project but ChessQtTests has its folder on the include path;
      - a project that references it (ChessGui, ChessGuiTests: built with them) doesn't link it.
    OnlineCore references no plugin at all. Run by Run-UiTests.ps1 (and so by GitHub). The exit code is the
    number of problems.
#>
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')

# The server plugins' folders. A new server's plugin is added here.
$plugins = @('LiChessConnector')
# Where a plugin's code must not be used: the app, its UI tests, the shared core.
$apps = @('ChessGui\ChessGui', 'ChessGuiTests', 'OnlineCore')

$problems = [System.Collections.Generic.List[string]]::new()

foreach ($plugin in $plugins) {
    $dir = Join-Path $root $plugin
    $headers = Get-ChildItem $dir -Filter *.h | ForEach-Object Name
    $others = $apps + ($plugins | Where-Object { $_ -ne $plugin })
    foreach ($folder in $others) {
        $sources = Get-ChildItem (Join-Path $root $folder) -File -Include *.h, *.cpp -Recurse |
            Where-Object { $_.FullName -notmatch '\\(x64|Debug|Release|GeneratedFiles)\\' }
        foreach ($file in $sources) {
            foreach ($match in Select-String -Path $file.FullName -Pattern '^\s*#\s*include\s*"([^"]+)"') {
                $name = Split-Path $match.Matches[0].Groups[1].Value -Leaf
                # stdafx.h and targetver.h are every project's own
                if ($headers -contains $name -and $name -notin 'stdafx.h', 'targetver.h') {
                    $problems.Add("$($file.FullName.Substring($root.Path.Length + 1)):$($match.LineNumber) includes $plugin's $name")
                }
            }
        }
    }
    foreach ($project in Get-ChildItem $root -Filter *.vcxproj -Recurse | Where-Object { $_.FullName -notmatch '\\x64\\' }) {
        $name = $project.BaseName
        if ($name -eq (Split-Path $plugin -Leaf) -or $name -eq 'ChessQtTests') { continue }
        [xml]$xml = Get-Content $project.FullName -Raw
        $ns = @{ m = 'http://schemas.microsoft.com/developer/msbuild/2003' }
        foreach ($inc in Select-Xml -Xml $xml -XPath '//m:AdditionalIncludeDirectories' -Namespace $ns) {
            if ($inc.Node.InnerText -match "\\$plugin(;|$)") { $problems.Add("$name.vcxproj: $plugin on the include path") }
        }
        foreach ($ref in Select-Xml -Xml $xml -XPath '//m:ProjectReference' -Namespace $ns) {
            if ($ref.Node.Include -notmatch "\\$plugin\.vcxproj$") { continue }
            if ($name -eq 'OnlineCore') { $problems.Add("OnlineCore.vcxproj references $plugin"); continue }
            $linkNode = $ref.Node.ChildNodes | Where-Object { $_.LocalName -eq 'LinkLibraryDependencies' }
            if (-not $linkNode -or $linkNode.InnerText -ne 'false') {
                $problems.Add("$name.vcxproj links $plugin (only build it: LinkLibraryDependencies false)")
            }
        }
    }
}

if ($problems.Count -eq 0) {
    Write-Host "Online isolation: OK ($($plugins -join ', ') used by nothing else)"
} else {
    Write-Host "Online isolation: $($problems.Count) problem(s)" -ForegroundColor Red
    $problems | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
}
exit $problems.Count
