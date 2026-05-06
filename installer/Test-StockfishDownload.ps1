# Test-StockfishDownload.ps1
# Dry-run the Stockfish download/unzip steps without running the installer.
# Run from any directory:
#   powershell -ExecutionPolicy Bypass -File installer\Test-StockfishDownload.ps1
# Optional: pass a custom destination to test the final copy step:
#   powershell -ExecutionPolicy Bypass -File installer\Test-StockfishDownload.ps1 -EnginesDir "C:\MyApp\engines"

param(
    [string]$EnginesDir = "$PSScriptRoot\test-engines"
)

$ProgressPreference = 'SilentlyContinue'
$tmpDir  = Join-Path $env:TEMP "sf_test_$(Get-Random)"
$zipFile = "$tmpDir\sf.zip"
$sfDir   = "$tmpDir\sf"

function Log($msg) {
    $ts = Get-Date -Format "HH:mm:ss"
    Write-Host "[$ts] $msg"
}

try {
    New-Item -ItemType Directory -Path $tmpDir | Out-Null
    New-Item -ItemType Directory -Path $EnginesDir -Force | Out-Null

    Log "Working dir : $tmpDir"
    Log "Engines dir : $EnginesDir"

    # --- Step 1: resolve latest release tag ---
    Log "Querying GitHub API for latest Stockfish release..."
    $rel = (Invoke-RestMethod "https://api.github.com/repos/official-stockfish/Stockfish/releases/latest" -TimeoutSec 30).tag_name
    Log "Release tag : $rel"

    # --- Step 2: download zip ---
    $zip = "stockfish-windows-x86-64-avx2.zip"
    $url = "https://github.com/official-stockfish/Stockfish/releases/download/$rel/$zip"
    Log "Downloading : $url"
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    Invoke-WebRequest $url -OutFile $zipFile -UseBasicParsing -TimeoutSec 300
    $sw.Stop()
    $sizeMB = [math]::Round((Get-Item $zipFile).Length / 1MB, 1)
    Log "Download OK : $sizeMB MB in $($sw.Elapsed.TotalSeconds.ToString('0.0'))s"

    # --- Step 3: extract ---
    Log "Extracting to $sfDir ..."
    Expand-Archive $zipFile -DestinationPath $sfDir -Force
    Log "Extraction OK"

    # --- Step 4: find exe ---
    Log "Searching for stockfish*.exe inside zip..."
    $exe = Get-ChildItem $sfDir -Recurse -Filter "stockfish*.exe" | Select-Object -First 1
    if (-not $exe) { throw "No stockfish*.exe found inside the zip" }
    Log "Found       : $($exe.FullName)"

    # --- Step 5: copy to engines dir ---
    $dest = "$EnginesDir\stockfish.exe"
    Copy-Item $exe.FullName $dest -Force
    Log "Copied to   : $dest"

    Log ""
    Log "=== All steps passed ==="
}
catch {
    Write-Host ""
    Write-Host "ERROR: $_" -ForegroundColor Red
    Write-Host "Stack: $($_.ScriptStackTrace)" -ForegroundColor Yellow
    exit 1
}
finally {
    if (Test-Path $tmpDir) {
        Remove-Item $tmpDir -Recurse -Force -ErrorAction SilentlyContinue
        Log "Cleaned up  : $tmpDir"
    }
}
