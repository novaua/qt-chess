# Test-StockfishDownload.ps1
# Dry-run the installer's Stockfish steps without running the installer, in the same order:
# for the newest stable release (then the previous one only if a step fails):
#   download -> SHA-256 check -> unzip + UCI check (Prepare-Stockfish.ps1) -> copy
# Writes the same log the installer leaves in {app}\engines\stockfish-install.log.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File installer\Test-StockfishDownload.ps1
#
# Optional parameters:
#   -EnginesDir   Where to copy stockfish.exe and the log (default: .\test-engines next to this script)
#   -SkipDownload Re-use an existing zip in -ZipPath for the first candidate instead of downloading
#   -ZipPath      Path to an already-downloaded zip (used with -SkipDownload)

param(
    [string]$EnginesDir   = "$PSScriptRoot\test-engines",
    [switch]$SkipDownload,
    [string]$ZipPath      = "$env:TEMP\sf_test.zip"
)

$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$tmpDir = Join-Path $env:TEMP "sf_test_$(Get-Random)"
$log    = "$tmpDir\stockfish-install.log"

function Log($m) { "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  $m" | Out-File $log -Append -Encoding ASCII }

$ok = $false
try {
    New-Item -ItemType Directory -Path $tmpDir | Out-Null
    New-Item -ItemType Directory -Path $EnginesDir -Force | Out-Null

    # Same candidates the installer gets from the build (see Build-Installer.ps1).
    $candidates = @(& "$PSScriptRoot\Resolve-Stockfish.ps1")
    Log "Test-StockfishDownload.ps1"

    for ($i = 0; $i -lt $candidates.Count -and -not $ok; $i++) {
        $c = $candidates[$i]
        Log ($(if ($i -eq 0) { "Stockfish $($c.Tag) (newest stable release)" } else { "Fallback: Stockfish $($c.Tag) (previous stable release)" }))
        Log "  url        : $($c.Url)"
        Log "  sha256     : $($c.Sha256)"
        $zip = "$tmpDir\stockfish-$($c.Tag).zip"

        try {
            if ($i -eq 0 -and $SkipDownload -and (Test-Path $ZipPath)) {
                Copy-Item $ZipPath $zip
            } else {
                (New-Object System.Net.WebClient).DownloadFile($c.Url, $zip)
                if ($i -eq 0) { Copy-Item $zip $ZipPath -Force } # cache for -SkipDownload
            }
            $hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
            if ($hash -ne $c.Sha256) { throw "SHA-256 mismatch: got $hash" }
            Log "  download   : OK, checksum verified"
        } catch {
            Log "  download   : FAILED - $($_.Exception.Message)"
            continue
        }

        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot\Prepare-Stockfish.ps1" `
            -Zip $zip -WorkDir "$tmpDir\sf_$($c.Tag)" -ReadyExe "$EnginesDir\stockfish.exe" -Log $log
        if ($LASTEXITCODE -eq 0) {
            Log "Ready: Stockfish $($c.Tag)"
            $ok = $true
        } else {
            Log "  result     : FAILED (exit code $LASTEXITCODE)"
        }
    }
    if (-not $ok) { Log "No working Stockfish engine" }
}
catch {
    Log "ERROR: $($_.Exception.Message)"
}
finally {
    if (Test-Path $log) {
        Copy-Item $log "$EnginesDir\stockfish-install.log" -Force
        Get-Content $log
    }
    Remove-Item $tmpDir -Recurse -Force -ErrorAction SilentlyContinue
}
if (-not $ok) { exit 1 }
