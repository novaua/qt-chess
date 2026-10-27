# Test-StockfishDownload.ps1
# Dry-run the Stockfish download/extract/copy steps without running the installer.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File installer\Test-StockfishDownload.ps1
#
# Optional parameters:
#   -EnginesDir   Where to copy stockfish.exe  (default: .\test-engines next to this script)
#   -SkipDownload Re-use an existing sf.zip in -ZipPath instead of downloading
#   -ZipPath      Path to an already-downloaded zip (used with -SkipDownload)

param(
    [string]$EnginesDir   = "$PSScriptRoot\test-engines",
    [switch]$SkipDownload,
    [string]$ZipPath      = "$env:TEMP\sf_test.zip"
)

$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# Keep in sync with the candidate list in ChessSetup.iss (tried in this order).
$latest = 'https://github.com/official-stockfish/Stockfish/releases/latest/download/'
$Candidates = @(
    @{ Url = "${latest}stockfish-windows-x86-64-universal.zip"; Sha256 = '' },
    @{ Url = "${latest}stockfish-windows-x86-64-avx2.zip";      Sha256 = '' },
    @{ Url = 'https://github.com/official-stockfish/Stockfish/releases/download/sf_19/stockfish-windows-x86-64-universal.zip';
       Sha256 = '3c8bf1f9ea66a09350a40df4f632288285ac206d99f33ab5842c408fc30b48a7' }
)
$tmpDir      = Join-Path $env:TEMP "sf_test_$(Get-Random)"
$sfDir        = "$tmpDir\sf"

function Log($msg) {
    $ts = Get-Date -Format "HH:mm:ss"
    Write-Host "[$ts] $msg"
}

try {
    New-Item -ItemType Directory -Path $tmpDir | Out-Null
    New-Item -ItemType Directory -Path $EnginesDir -Force | Out-Null

    Log "Engines dir : $EnginesDir"

    # --- Step 1: download (skippable for faster re-runs) ---
    if ($SkipDownload -and (Test-Path $ZipPath)) {
        Log "Skipping download, using existing zip: $ZipPath"
        Copy-Item $ZipPath "$tmpDir\sf.zip"
    } else {
        $ok = $false
        foreach ($c in $Candidates) {
            Log "Downloading : $($c.Url)"
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            try {
                (New-Object System.Net.WebClient).DownloadFile($c.Url, "$tmpDir\sf.zip")
                if ($c.Sha256 -and (Get-FileHash "$tmpDir\sf.zip" -Algorithm SHA256).Hash -ne $c.Sha256) {
                    throw "SHA-256 mismatch"
                }
                $ok = $true
                break
            } catch {
                Log "  failed    : $($_.Exception.Message)"
                Remove-Item "$tmpDir\sf.zip" -Force -ErrorAction SilentlyContinue
            }
        }
        if (-not $ok) { throw "All Stockfish download candidates failed" }
        $sw.Stop()
        $sizeMB = [math]::Round((Get-Item "$tmpDir\sf.zip").Length / 1MB, 1)
        Log "Download OK : $sizeMB MB in $($sw.Elapsed.TotalSeconds.ToString('0.0'))s"

        # Cache for -SkipDownload on next run
        Copy-Item "$tmpDir\sf.zip" $ZipPath -Force
        Log "Cached at   : $ZipPath  (use -SkipDownload to reuse)"
    }

    # --- Step 2: extract ---
    Log "Extracting to $sfDir ..."
    Expand-Archive "$tmpDir\sf.zip" -DestinationPath $sfDir -Force
    Log "Extraction OK"

    # --- Step 3: find exe ---
    Log "Searching for stockfish*.exe..."
    $exe = Get-ChildItem $sfDir -Recurse -Filter "stockfish*.exe" | Select-Object -First 1
    if (-not $exe) { throw "No stockfish*.exe found inside the zip" }
    Log "Found       : $($exe.FullName)"

    # --- Step 4: copy ---
    $dest = "$EnginesDir\stockfish.exe"
    Copy-Item $exe.FullName $dest -Force
    Log "Copied to   : $dest"

    Log ""
    Log "=== All steps passed ==="
}
catch {
    Write-Host ""
    Write-Host "FAILED: $_" -ForegroundColor Red
    Write-Host "Stack : $($_.ScriptStackTrace)" -ForegroundColor Yellow
    exit 1
}
finally {
    if (Test-Path $tmpDir) {
        Remove-Item $tmpDir -Recurse -Force -ErrorAction SilentlyContinue
        Log "Cleaned up  : $tmpDir"
    }
}
