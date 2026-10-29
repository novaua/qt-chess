<#
.SYNOPSIS
    Compares screenshots with saved baselines and writes a diff image for each one that changed.

.DESCRIPTION
    A pre-PR check for the UI (BL-05 layer 2): take the scenes with Take-Screenshot.ps1, then
    compare them with the baselines you accepted earlier. Baselines are local (test-results\ is not
    in git): fonts and scaling differ between machines, so another PC's pictures would never match.

    A pixel counts as changed when a colour channel differs by more than -Tolerance. A screenshot
    fails when more than -MaxChangedPercent of its pixels changed, or its size differs. For each
    failure, <DiffDir>\<name>.png shows the baseline greyed out with the changed pixels in red.

    A baseline without a screenshot fails too (MISSING): its scene failed or wasn't taken. Take the
    same scenes as the baselines, or limit the check with -Name.

    Exit code: the number of screenshots that failed (0 = all match).

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene all; .\scripts\Take-Screenshot.ps1 -Scene all -Dark
    .\scripts\Compare-Screenshots.ps1 -Update
    Take every scene in light and dark, and accept them as the baselines (on develop, before a change).

.EXAMPLE
    .\scripts\Take-Screenshot.ps1 -Scene all; .\scripts\Take-Screenshot.ps1 -Scene all -Dark
    .\scripts\Compare-Screenshots.ps1
    After the change: what looks different? Open test-results\diffs\ for the failures.
#>
#Requires -Version 7
[CmdletBinding()]
param(
    [string]$ScreensDir  = (Join-Path $PSScriptRoot '..\test-results\screens'),
    [string]$BaselineDir = (Join-Path $PSScriptRoot '..\test-results\baselines'),
    [string]$DiffDir     = (Join-Path $PSScriptRoot '..\test-results\diffs'),
    # Only these screenshots (file names without .png, wildcards allowed), e.g. home*, game-dark
    [string[]]$Name = @('*'),
    # Largest colour difference (0-255 per channel) that still counts as the same pixel
    [int]$Tolerance = 24,
    # Changed pixels allowed before a screenshot fails, in percent of the image
    [double]$MaxChangedPercent = 0.05,
    # Copy the screenshots to the baselines instead of comparing (accept the current look)
    [switch]$Update
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $ScreensDir)) { throw "$ScreensDir not found - take screenshots first (Take-Screenshot.ps1)" }
$shots = Get-ChildItem $ScreensDir -Filter *.png |
    Where-Object { $n = $_.BaseName; $Name | Where-Object { $n -like $_ } }
if (-not $shots) { throw "No screenshots in $ScreensDir match $($Name -join ', ')" }

if ($Update) {
    New-Item -ItemType Directory -Force $BaselineDir | Out-Null
    $shots | Copy-Item -Destination $BaselineDir -Force
    Write-Host "$($shots.Count) baselines saved to $(Resolve-Path $BaselineDir)"
    exit 0
}

Add-Type -AssemblyName System.Drawing
# .NET spreads Bitmap over several assemblies (System.Drawing.Common, System.Private.Windows.GdiPlus,
# ...): the C# below references the loaded ones.
$drawing = [AppDomain]::CurrentDomain.GetAssemblies() |
    Where-Object { $_.GetName().Name -match '^System\.(Drawing|Private\.Windows|Runtime(\.InteropServices)?$|Primitives)' -and $_.Location } |
    ForEach-Object Location
Add-Type -ReferencedAssemblies $drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class ScreenshotDiff
{
    static int[] Pixels(Bitmap bmp)
    {
        var data = bmp.LockBits(new Rectangle(0, 0, bmp.Width, bmp.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        var pixels = new int[bmp.Width * bmp.Height];
        for (int y = 0; y < bmp.Height; y++)
            Marshal.Copy(data.Scan0 + y * data.Stride, pixels, y * bmp.Width, bmp.Width);
        bmp.UnlockBits(data);
        return pixels;
    }

    // Returns the changed pixels in percent (-1: the size differs) and writes the diff image to
    // diffPath when that is more than maxPercent.
    public static double Compare(string actualPath, string baselinePath, int tolerance, double maxPercent, string diffPath)
    {
        using (var actual = new Bitmap(actualPath))
        using (var baseline = new Bitmap(baselinePath))
        {
            if (actual.Width != baseline.Width || actual.Height != baseline.Height)
                return -1;
            int[] a = Pixels(actual), b = Pixels(baseline);
            var diff = new int[a.Length];
            long changed = 0;
            for (int i = 0; i < a.Length; i++)
            {
                int pa = a[i], pb = b[i];
                bool same = true;
                for (int shift = 0; shift < 32 && same; shift += 8)
                    same = Math.Abs(((pa >> shift) & 0xFF) - ((pb >> shift) & 0xFF)) <= tolerance;
                if (same)
                {
                    // The baseline, greyed and faded, so the red stands out.
                    int grey = (((pb >> 16) & 0xFF) + ((pb >> 8) & 0xFF) + (pb & 0xFF)) / 3;
                    grey = 255 - (255 - grey) / 4;
                    diff[i] = unchecked((int)0xFF000000) | (grey << 16) | (grey << 8) | grey;
                }
                else
                {
                    changed++;
                    diff[i] = unchecked((int)0xFFFF0000);
                }
            }
            double percent = 100.0 * changed / a.Length;
            if (percent > maxPercent)
            {
                using (var img = new Bitmap(actual.Width, actual.Height, PixelFormat.Format32bppArgb))
                {
                    var data = img.LockBits(new Rectangle(0, 0, img.Width, img.Height), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                    for (int y = 0; y < img.Height; y++)
                        Marshal.Copy(diff, y * img.Width, data.Scan0 + y * data.Stride, img.Width);
                    img.UnlockBits(data);
                    img.Save(diffPath, ImageFormat.Png);
                }
            }
            return percent;
        }
    }
}
'@

New-Item -ItemType Directory -Force $DiffDir | Out-Null
$DiffDir = Resolve-Path $DiffDir
$failed = 0; $missing = 0
foreach ($shot in $shots) {
    $baseline = Join-Path $BaselineDir $shot.Name
    $diffFile = Join-Path $DiffDir $shot.Name
    Remove-Item $diffFile -ErrorAction SilentlyContinue
    if (-not (Test-Path $baseline)) {
        $missing++
        Write-Host "NEW   $($shot.BaseName) - no baseline (accept it with -Update)" -ForegroundColor Yellow
        continue
    }
    $percent = [ScreenshotDiff]::Compare($shot.FullName, (Resolve-Path $baseline).Path, $Tolerance, $MaxChangedPercent, $diffFile)
    if ($percent -lt 0) {
        $failed++
        Write-Host "FAIL  $($shot.BaseName) - the size changed" -ForegroundColor Red
        continue
    }
    if ($percent -gt $MaxChangedPercent) {
        $failed++
        Write-Host ("FAIL  {0} - {1:N2} % of the pixels changed: {2}" -f $shot.BaseName, $percent, $diffFile) -ForegroundColor Red
    } else {
        Write-Host ("OK    {0}{1}" -f $shot.BaseName, $(if ($percent) { " ({0:N3} % of the pixels changed)" -f $percent }))
    }
}
# A baseline without a new screenshot: the scene failed (Take-Screenshot.ps1 deletes the old picture
# first) or wasn't taken. It fails, so a broken scene can't pass unnoticed.
$gone = Get-ChildItem $BaselineDir -Filter *.png -ErrorAction SilentlyContinue |
    Where-Object { $n = $_.BaseName; ($Name | Where-Object { $n -like $_ }) -and -not (Test-Path (Join-Path $ScreensDir $_.Name)) }
foreach ($b in $gone) {
    $failed++
    Write-Host "MISSING $($b.BaseName) - a baseline without a screenshot: the scene failed or wasn't taken" -ForegroundColor Red
}
Write-Host ("{0} compared, {1} failed, {2} without a baseline" -f ($shots.Count - $missing), $failed, $missing)
exit $failed
