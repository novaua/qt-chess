# Prepare-Stockfish.ps1
# Run by ChessSetup.iss during installation (and by Test-StockfishDownload.ps1):
# unzip a downloaded Stockfish release, find the engine, check that it answers the UCI
# handshake ("uci" -> "id name ..." ... "uciok"), and copy it to -ReadyExe.
# Appends to -Log; exit code 0 = engine ready, 1 = failed (the installer then tries the
# fallback release). Must run on Windows PowerShell 5.1.

param(
    [Parameter(Mandatory)] [string]$Zip,
    [Parameter(Mandatory)] [string]$WorkDir,
    [Parameter(Mandatory)] [string]$ReadyExe,
    [Parameter(Mandatory)] [string]$Log,
    [int]$UciTimeoutSec = 15
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

function Log($m) { "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  $m" | Out-File $Log -Append -Encoding ASCII }

$proc = $null
try {
    if (Test-Path $WorkDir) { Remove-Item $WorkDir -Recurse -Force }
    Expand-Archive -Path $Zip -DestinationPath $WorkDir -Force
    Log "  unzip      : OK"

    $exe = Get-ChildItem $WorkDir -Recurse -Filter 'stockfish*.exe' | Select-Object -First 1
    if (-not $exe) { throw "no stockfish*.exe inside the zip" }
    Log "  engine     : $($exe.Name) ($([math]::Round($exe.Length / 1MB, 1)) MB)"

    # UCI handshake: proves the binary starts on this CPU and speaks UCI.
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe.FullName
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $proc = [System.Diagnostics.Process]::Start($psi)
    # On PowerShell 5.1 the stdin pipe starts with a UTF-8 BOM (written as soon as the
    # process starts), so a first "uci" would arrive as "﻿uci", an unknown command.
    # An empty first line takes the BOM; the engine ignores it.
    $stdin = $proc.StandardInput
    $stdin.WriteLine('')
    $stdin.WriteLine('uci')

    $name = $null
    $uciok = $false
    $deadline = (Get-Date).AddSeconds($UciTimeoutSec)
    while (-not $uciok) {
        $left = [int]($deadline - (Get-Date)).TotalMilliseconds
        if ($left -le 0) { break }
        $read = $proc.StandardOutput.ReadLineAsync()
        if (-not $read.Wait($left)) { break }
        $line = $read.Result
        if ($null -eq $line) { break } # engine exited
        if ($line -like 'id name *') { $name = $line.Substring(8).Trim() }
        if ($line.Trim() -eq 'uciok') { $uciok = $true }
    }
    if (-not $uciok) {
        $why = if ($proc.HasExited) { "engine exited with code $($proc.ExitCode)" } else { "no 'uciok' within $UciTimeoutSec s" }
        throw "UCI check failed: $why"
    }
    $stdin.WriteLine('quit')
    if (-not $proc.WaitForExit(3000)) { $proc.Kill() }
    Log "  uci        : OK - $name"

    Copy-Item $exe.FullName $ReadyExe -Force
    exit 0
}
catch {
    Log "  ERROR      : $($_.Exception.Message)"
    exit 1
}
finally {
    if ($proc -and -not $proc.HasExited) { try { $proc.Kill() } catch { } }
}
