<#
.SYNOPSIS
    Compiles a human-editable .psf.txt stroke font back to binary .psf format.

.DESCRIPTION
    Reads a text representation produced by ConvertPsfToText.ps1 (or hand-authored)
    and writes a v2 binary .psf file.

    The text format is line-oriented:

        VERSION 2
        CHAR <code> <printable> <advance> <leftBearing>
            M <dx> <dy>
            D <dx> <dy>

    Lines starting with ; are comments. Blank lines are ignored.
    Fields are tab-separated. The <printable> field is for human readability only
    and is not used during compilation.

    This is the PSF equivalent of AutoCAD's .shp → .shx compilation.

    Output is always v2 format. If VERSION 1 is specified in the input, advance
    widths and left bearings default to 0 (fixed-width fallback).

.PARAMETER InputPath
    Path to the .psf.txt text file.

.PARAMETER OutputPath
    Path for the binary .psf output file. Defaults to InputPath with .txt removed.

.EXAMPLE
    .\ConvertTextToPsf.ps1 -InputPath "..\Application\AeSys\res\Simplex.psf.txt"
    # produces ..\Application\AeSys\res\Simplex.psf
#>
param(
    [Parameter(Mandatory)]
    [string]$InputPath,

    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$resolvedInput = (Resolve-Path $InputPath).Path
if (-not $OutputPath) {
    if ($resolvedInput -match '\.psf\.txt$') {
        $OutputPath = $resolvedInput -replace '\.txt$', ''
    } else {
        $OutputPath = "$resolvedInput.psf"
    }
}

# --- Constants matching Eo.h ---
$FileSizeBytes         = 16384
$TotalInt32s           = $FileSizeBytes / 4  # 4096

$V2Magic               = -2
$V2OffsetTableStart    = 1
$V2OffsetTableEntries  = 225     # 224 chars + sentinel
$V2AdvanceStart        = 226
$V2LeftBearingStart    = 450
$V2StrokeDataStart     = 674
$V2CharCount           = 224     # chars 32-255

# --- Encode sign-magnitude ---
function Encode-SignMagnitude([int]$value) {
    if ($value -lt 0) {
        return ([Math]::Abs($value) + 2048)
    }
    return $value
}

# --- Encode a stroke to int32 ---
function Encode-Stroke([string]$op, [int]$dx, [int]$dy) {
    $opcode = if ($op -eq 'M') { 5 } else { 0 }
    $rawX = Encode-SignMagnitude $dx
    $rawY = Encode-SignMagnitude $dy
    return ($opcode * 16777216 + $rawX * 4096 + $rawY)
}

# --- Parse text file ---
$lines = [System.IO.File]::ReadAllLines($resolvedInput, [System.Text.Encoding]::UTF8)

$version = 2  # default output version
$characters = @{}  # code -> hashtable with Advance, LeftBearing, Strokes (list of M/D + dx + dy)

$currentCode = -1

foreach ($rawLine in $lines) {
    $line = $rawLine.Trim()

    # Skip blanks and comments
    if ($line -eq '' -or $line.StartsWith(';')) { continue }

    $fields = $line -split '[\t ]+'

    if ($fields[0] -eq 'VERSION') {
        $version = [int]$fields[1]
        continue
    }

    if ($fields[0] -eq 'CHAR') {
        $currentCode = [int]$fields[1]
        # fields[2] = printable (ignored)
        $advance     = if ($fields.Length -ge 4) { [int]$fields[3] } else { 0 }
        $leftBearing = if ($fields.Length -ge 5) { [int]$fields[4] } else { 0 }

        $characters[$currentCode] = @{
            Advance     = $advance
            LeftBearing = $leftBearing
            Strokes     = [System.Collections.Generic.List[hashtable]]::new()
        }
        continue
    }

    if ($fields[0] -eq 'M' -or $fields[0] -eq 'D') {
        if ($currentCode -lt 0) { throw "Stroke data before any CHAR definition at line: $rawLine" }
        $characters[$currentCode].Strokes.Add(@{
            Op = $fields[0]
            Dx = [int]$fields[1]
            Dy = [int]$fields[2]
        })
        continue
    }

    throw "Unrecognized line: $rawLine"
}

Write-Host "Parsed: $($characters.Count) characters from text input"

# --- Build binary ---
$output = New-Object byte[] $FileSizeBytes  # zero-initialized

function Write-Int32([int]$index, [int]$value) {
    $b = [BitConverter]::GetBytes($value)
    [Array]::Copy($b, 0, $output, $index * 4, 4)
}

# Magic
Write-Int32 0 $V2Magic

# Build stroke data and offset table together
$strokeIndex = 1  # 1-based offset into stroke data
$totalStrokeCount = 0

for ($ci = 0; $ci -lt $V2CharCount; $ci++) {
    $code = 32 + $ci

    # Write offset table entry (1-based stroke index)
    Write-Int32 ($V2OffsetTableStart + $ci) $strokeIndex

    if ($characters.ContainsKey($code)) {
        $charData = $characters[$code]

        # Write advance width
        Write-Int32 ($V2AdvanceStart + $ci) $charData.Advance

        # Write left bearing
        Write-Int32 ($V2LeftBearingStart + $ci) $charData.LeftBearing

        # Write strokes
        foreach ($stroke in $charData.Strokes) {
            $encoded = Encode-Stroke $stroke.Op $stroke.Dx $stroke.Dy
            $dataIndex = $V2StrokeDataStart + $strokeIndex - 1
            if ($dataIndex -ge $TotalInt32s) {
                throw "Stroke data overflow at character code $code (stroke index $strokeIndex). File capacity is $TotalInt32s int32s with stroke data starting at index $V2StrokeDataStart."
            }
            Write-Int32 $dataIndex $encoded
            $strokeIndex++
            $totalStrokeCount++
        }
    }
    # else: offset stays at current strokeIndex (empty range), advance/leftBearing stay 0
}

# Sentinel (one past last stroke)
Write-Int32 ($V2OffsetTableStart + $V2CharCount) $strokeIndex

# --- Write output ---
[System.IO.File]::WriteAllBytes($OutputPath, $output)

$freeEntries = $TotalInt32s - $V2StrokeDataStart - $totalStrokeCount

Write-Host "Written: $OutputPath"
Write-Host "  Format:      v2"
Write-Host "  Characters:  $($characters.Count) defined of $V2CharCount slots"
Write-Host "  Strokes:     $totalStrokeCount"
Write-Host "  Free:        $freeEntries entries (~$([Math]::Floor($freeEntries / 12)) average glyphs)"
Write-Host "  File size:   $FileSizeBytes bytes"
