<#
.SYNOPSIS
    Converts a Simplex.psf stroke font from v1 format to v2 format.

.DESCRIPTION
    v1 layout (16384 bytes = 4096 int32s):
        int32[0..95]     Offset table for chars 32-126 (95 entries + 1 sentinel)
        int32[96..4095]  Stroke data (4000 entry capacity)

    v2 layout (16384 bytes = 4096 int32s):
        int32[0]         Magic number = -2
        int32[1..225]    Offset table for chars 32-255 (224 entries + 1 sentinel)
        int32[226..449]  Advance width table for chars 32-255 (224 entries, raw stroke X-units)
        int32[450..673]  Left bearing table for chars 32-255 (224 entries, raw stroke X-units)
        int32[674..4095] Stroke data (3422 entry capacity)

    Stroke offset values are 1-based indices into the stroke data array and remain
    unchanged between v1 and v2 — only the physical position of the stroke data
    moves within the file.

    Advance widths are computed from the drawn stroke extents using:
        advance = max(28, drawnWidth + 12)
    with space hardcoded to 50. These are first-pass approximations derived from
    the original Hershey Simplex geometry; edit the $advanceWidthOverrides hashtable
    below to refine individual character widths.

.PARAMETER InputPath
    Path to the v1 Simplex.psf file.

.PARAMETER OutputPath
    Path for the v2 output file. Defaults to overwriting the input file.

.EXAMPLE
    .\ConvertStrokeFontV1ToV2.ps1 -InputPath "..\Application\AeSys\res\Simplex.psf"
#>
param(
    [Parameter(Mandatory)]
    [string]$InputPath,

    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $OutputPath) { $OutputPath = $InputPath }

# --- Constants matching Eo.h ---
$FileSizeBytes        = 16384
$TotalInt32s          = $FileSizeBytes / 4  # 4096

$V1OffsetTableSize    = 96
$V1CharCount          = 95      # chars 32-126

$V2Magic              = -2
$V2OffsetTableStart   = 1
$V2OffsetTableEntries = 225     # 224 chars (32-255) + 1 sentinel
$V2AdvanceStart       = 226
$V2LeftBearingStart   = 450
$V2StrokeDataStart    = 674
$V2CharCount          = 224     # chars 32-255

# --- Read v1 file ---
$rawBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $InputPath).Path)
if ($rawBytes.Length -ne $FileSizeBytes) {
    throw "Input file is $($rawBytes.Length) bytes; expected $FileSizeBytes."
}

# Verify not already v2
$firstInt32 = [BitConverter]::ToInt32($rawBytes, 0)
if ($firstInt32 -eq $V2Magic) {
    throw "Input file is already v2 format (magic = $V2Magic at offset 0)."
}

# Parse v1 offset table (96 int32s)
$v1Offsets = New-Object int[] $V1OffsetTableSize
for ($i = 0; $i -lt $V1OffsetTableSize; $i++) {
    $v1Offsets[$i] = [BitConverter]::ToInt32($rawBytes, $i * 4)
}
$sentinel = $v1Offsets[$V1CharCount]  # entry [95] = one past last stroke of '~'

# Parse v1 stroke data (starts at int32 index 96)
$strokeCount = $sentinel - 1  # number of stroke int32s actually used
$v1StrokeData = New-Object int[] $strokeCount
for ($i = 0; $i -lt $strokeCount; $i++) {
    $v1StrokeData[$i] = [BitConverter]::ToInt32($rawBytes, ($V1OffsetTableSize + $i) * 4)
}

Write-Host "v1 parsed: $V1CharCount characters, $strokeCount strokes, sentinel=$sentinel"

# --- Compute stroke extents per character ---
# Returns a hashtable with DrawnWidth and MinCumX for the given character index.
function Get-StrokeExtents {
    param([int]$charIndex)  # 0-based: char = 32 + charIndex
    $startOff = $v1Offsets[$charIndex]
    $endOff   = if ($charIndex -lt $V1CharCount) { $v1Offsets[$charIndex + 1] } else { $sentinel }

    if ($endOff -le $startOff) { return @{ DrawnWidth = 0; MinCumX = 0 } }  # no strokes

    $cx = 0; $minX = [int]::MaxValue; $maxX = [int]::MinValue
    for ($i = $startOff; $i -lt $endOff; $i++) {
        $val = $v1StrokeData[$i - 1]
        $x = ($val -shr 12) -band 0xFFF
        if (($x -band 0x800) -ne 0) { $x = -($x - 2048) }
        $cx += $x
        if ($cx -lt $minX) { $minX = $cx }
        if ($cx -gt $maxX) { $maxX = $cx }
    }
    return @{ DrawnWidth = ($maxX - $minX); MinCumX = $minX }
}

# Optional per-character overrides (ASCII code -> raw advance width)
# Edit these to refine individual character spacing.
$advanceWidthOverrides = @{
    32 = 50   # space
}

$LeftBearingMargin = 6  # raw stroke X-units of margin on the left side of each character cell

$advanceWidths = New-Object int[] $V2CharCount  # 224 entries for chars 32-255
$leftBearings  = New-Object int[] $V2CharCount  # 224 entries for chars 32-255

for ($ci = 0; $ci -lt $V2CharCount; $ci++) {
    $ascii = 32 + $ci
    if ($advanceWidthOverrides.ContainsKey($ascii)) {
        $advanceWidths[$ci] = $advanceWidthOverrides[$ascii]
        $leftBearings[$ci] = 0  # override characters use 0 left bearing
    } elseif ($ci -lt $V1CharCount) {
        # Character exists in v1 — compute from stroke extents
        $extents = Get-StrokeExtents $ci
        $advanceWidths[$ci] = [Math]::Max(28, $extents.DrawnWidth + 12)
        $leftBearings[$ci]  = [Math]::Max(0, $extents.MinCumX - $LeftBearingMargin)
    } else {
        # Undefined character (128-255) — 0 signals fallback to fixed advance
        $advanceWidths[$ci] = 0
        $leftBearings[$ci] = 0
    }
}

# Print advance widths and left bearings for verification
Write-Host ""
Write-Host "=== Advance widths and left bearings (raw stroke X-units) ==="
for ($ci = 0; $ci -lt $V1CharCount; $ci++) {
    $ascii = 32 + $ci
    $ch = [char]$ascii
    $extents = Get-StrokeExtents $ci
    Write-Host ("  '$ch' (ASCII $ascii): minCumX=$($extents.MinCumX.ToString().PadLeft(3))  drawnWidth=$($extents.DrawnWidth.ToString().PadLeft(3))  advance=$($advanceWidths[$ci].ToString().PadLeft(3))  leftBearing=$($leftBearings[$ci].ToString().PadLeft(3))")
}

# --- Build v2 file ---
$v2 = New-Object byte[] $FileSizeBytes  # zero-initialized

function Write-Int32([int]$index, [int]$value) {
    $b = [BitConverter]::GetBytes($value)
    [Array]::Copy($b, 0, $v2, $index * 4, 4)
}

# Magic number
Write-Int32 0 $V2Magic

# Offset table: entries [1..225]
# Chars 32-126 (indices 0..94): copy from v1
for ($ci = 0; $ci -lt $V1CharCount; $ci++) {
    Write-Int32 ($V2OffsetTableStart + $ci) $v1Offsets[$ci]
}
# Sentinel for char 126 and all undefined chars 127-255: fill with sentinel
for ($ci = $V1CharCount; $ci -lt $V2OffsetTableEntries; $ci++) {
    Write-Int32 ($V2OffsetTableStart + $ci) $sentinel
}

# Advance width table: entries [226..449]
for ($ci = 0; $ci -lt $V2CharCount; $ci++) {
    Write-Int32 ($V2AdvanceStart + $ci) $advanceWidths[$ci]
}

# Left bearing table: entries [450..673]
for ($ci = 0; $ci -lt $V2CharCount; $ci++) {
    Write-Int32 ($V2LeftBearingStart + $ci) $leftBearings[$ci]
}

# Stroke data: entries [674..674+strokeCount-1]
for ($i = 0; $i -lt $strokeCount; $i++) {
    Write-Int32 ($V2StrokeDataStart + $i) $v1StrokeData[$i]
}

# Verify capacity
$totalUsed = $V2StrokeDataStart + $strokeCount
if ($totalUsed -gt $TotalInt32s) {
    throw "Stroke data overflow: need $totalUsed int32s but file holds $TotalInt32s."
}

Write-Host ""
Write-Host "=== v2 file layout ==="
Write-Host "  Magic:          int32[0] = $V2Magic"
Write-Host "  Offset table:   int32[1..225] ($V2OffsetTableEntries entries)"
Write-Host "  Advance table:  int32[226..449] ($V2CharCount entries)"
Write-Host "  LBearing table: int32[450..673] ($V2CharCount entries)"
Write-Host "  Stroke data:    int32[674..$($V2StrokeDataStart + $strokeCount - 1)] ($strokeCount strokes)"
Write-Host "  Free entries:   $($TotalInt32s - $totalUsed) (capacity for ~$([Math]::Floor(($TotalInt32s - $totalUsed) / 12)) new average glyphs)"
Write-Host "  File size:      $FileSizeBytes bytes"

# --- Write output ---
[System.IO.File]::WriteAllBytes((Resolve-Path $OutputPath -ErrorAction SilentlyContinue) ?? $OutputPath, $v2)
Write-Host ""
Write-Host "v2 file written to: $OutputPath"
