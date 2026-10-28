# Resolve-Stockfish.ps1
# Find the Stockfish downloads the installer should use, at installer build time:
#   Primary  = Windows x86-64 asset of the newest STABLE release (no dev/prerelease builds)
#   Fallback = same for the previous stable release (used only if the primary fails)
# Each comes with the SHA-256 that GitHub publishes for the asset, so the installer can
# verify the download. Output: ISCC /D defines for ChessSetup.iss (see -AsIsccArgs).
#
# Usage:
#   .\installer\Resolve-Stockfish.ps1              # print the resolved downloads
#   $args = .\installer\Resolve-Stockfish.ps1 -AsIsccArgs
#
# Set GITHUB_TOKEN (CI does) to avoid the unauthenticated API rate limit.

param([switch]$AsIsccArgs)

$ErrorActionPreference = 'Stop'

# Stockfish renamed its Windows assets in sf_19 (one "universal" build instead of one zip
# per CPU feature set). Older releases: sse41-popcnt runs on practically any x86-64 CPU.
$PreferredAssets = @(
    'stockfish-windows-x86-64-universal.zip',
    'stockfish-windows-x86-64-sse41-popcnt.zip',
    'stockfish-windows-x86-64.zip'
)

$headers = @{ 'Accept' = 'application/vnd.github+json'; 'User-Agent' = 'qt-chess-installer-build' }
if ($env:GITHUB_TOKEN) { $headers['Authorization'] = "Bearer $env:GITHUB_TOKEN" }

$releases = Invoke-RestMethod -Headers $headers `
    -Uri 'https://api.github.com/repos/official-stockfish/Stockfish/releases?per_page=30'

# Stable releases are tagged sf_<version>; dev builds are prereleases named stockfish-dev-*.
$stable = @($releases | Where-Object { -not $_.prerelease -and -not $_.draft -and $_.tag_name -like 'sf_*' } |
    Sort-Object { [datetime]$_.published_at } -Descending)

function Select-Download($release) {
    foreach ($name in $PreferredAssets) {
        $asset = $release.assets | Where-Object { $_.name -eq $name } | Select-Object -First 1
        if ($asset -and $asset.digest -match '^sha256:([0-9a-f]{64})$') {
            return [pscustomobject]@{
                Tag    = $release.tag_name
                Url    = $asset.browser_download_url
                Sha256 = $Matches[1]
            }
        }
    }
    return $null
}

$found = @()
foreach ($r in $stable) {
    $d = Select-Download $r
    if ($d) { $found += $d }
    if ($found.Count -eq 2) { break }
}
if ($found.Count -lt 2) {
    throw "Could not resolve two stable Stockfish releases with a Windows x86-64 asset and SHA-256 digest"
}

$primary, $fallback = $found
if ($AsIsccArgs) {
    @(
        "/DSfPrimaryTag=$($primary.Tag)",   "/DSfPrimaryUrl=$($primary.Url)",   "/DSfPrimarySha256=$($primary.Sha256)",
        "/DSfFallbackTag=$($fallback.Tag)", "/DSfFallbackUrl=$($fallback.Url)", "/DSfFallbackSha256=$($fallback.Sha256)"
    )
} else {
    [pscustomobject]@{ Role = 'primary';  Tag = $primary.Tag;  Url = $primary.Url;  Sha256 = $primary.Sha256 }
    [pscustomobject]@{ Role = 'fallback'; Tag = $fallback.Tag; Url = $fallback.Url; Sha256 = $fallback.Sha256 }
}
