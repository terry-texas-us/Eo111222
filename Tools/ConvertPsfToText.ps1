<#
.SYNOPSIS
    Decompiles a binary Simplex.psf stroke font to a human-editable text file.

.DESCRIPTION
    Reads a v1 or v2 binary .psf file and writes a .psf.txt text representation
    that can be inspected, edited, and recompiled with ConvertTextToPsf.ps1.

    The text format is line-oriented with tab-separated fields:

        ; Comment lines start with semicolons
        VERSION 2
        CHAR <code> <character> <advance> <leftBearing>
            M <dx> <dy>
            D <dx> <dy>

    Stroke displacements are in raw sign-magnitude decoded integers (same units
    as the binary format). Advance width and left bearing are in raw stroke X-units.

    This is the PSF equivalent of AutoCAD's .shx → .shp decompilation.

.PARAMETER InputPath
    Path to the binary .psf file.

.PARAMETER OutputPath
    Path for the text output file. Defaults to InputPath with .txt appended.

.EXAMPLE
    .\ConvertPsfToText.ps1 -InputPath "..\Application\AeSys\res\Simplex.psf"
    # produces ..\Application\AeSys\res\Simplex.psf.txt
#>
param(
    [Parameter(Mandatory)]
    [string]$InputPath,

    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$resolvedInput = (Resolve-Path $InputPath).Path
if (-not $OutputPath) { $OutputPath = "$resolvedInput.txt" }

# --- Constants ---
$FileSizeBytes = 16384
$TotalInt32s   = $FileSizeBytes / 4

$V2Magic = -2

# --- Read binary ---
$rawBytes = [System.IO.File]::ReadAllBytes($resolvedInput)
if ($rawBytes.Length -ne $FileSizeBytes) {
    throw "Input file is $($rawBytes.Length) bytes; expected $FileSizeBytes."
}

$ints = New-Object int[] $TotalInt32s
[System.Buffer]::BlockCopy($rawBytes, 0, $ints, 0, $FileSizeBytes)

# --- Detect version and resolve tables ---
if ($ints[0] -eq $V2Magic) {
    $version            = 2
    $offsetTableStart   = 1
    $offsetTableEntries = 225   # 224 chars + sentinel
    $advanceStart       = 226
    $leftBearingStart   = 450
    $strokeDataStart    = 674
    $charCount          = 224
    $maxCharCode        = 255
} else {
    $version            = 1
    $offsetTableStart   = 0
    $offsetTableEntries = 96    # 95 chars + sentinel
    $advanceStart       = -1    # not present
    $leftBearingStart   = -1    # not present
    $strokeDataStart    = 96
    $charCount          = 95
    $maxCharCode        = 126
}

$sentinel = $ints[$offsetTableStart + $charCount]
$strokeCount = $sentinel - 1

Write-Host "Reading v$version PSF: $charCount characters, $strokeCount strokes"

# --- Decode one stroke int32 using bit operations (matches C++ integer truncation) ---
function Decode-Stroke([int]$raw) {
    $opcode = ($raw -shr 24) -band 0xFF
    $rawX   = ($raw -shr 12) -band 0xFFF
    $rawY   = $raw -band 0xFFF

    $dx = if (($rawX -band 0x800) -ne 0) { -($rawX - 2048) } else { $rawX }
    $dy = if (($rawY -band 0x800) -ne 0) { -($rawY - 2048) } else { $rawY }
    $op = if ($opcode -eq 5) { 'M' } else { 'D' }

    return @{ Op = $op; Dx = $dx; Dy = $dy }
}

# --- Build text output ---
$lines = [System.Collections.Generic.List[string]]::new(2000)

$lines.Add("; Simplex PSF Stroke Font — text representation")
$lines.Add("; Generated from: $resolvedInput")
$lines.Add("; Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
$lines.Add("; See Docs/Simplex PSF Format.md for format specification")
$lines.Add(";")
$lines.Add("; CHAR <code> <printable> <advance> <leftBearing>")
$lines.Add(";   M <dx> <dy>    — move (pen up)")
$lines.Add(";   D <dx> <dy>    — draw (pen down)")
$lines.Add(";")
$lines.Add("; Displacements are relative, in raw stroke units.")
$lines.Add("; Advance width and left bearing are in raw stroke X-units.")
$lines.Add("; Characters with advance=0 use fixed-width fallback.")
$lines.Add("")
$lines.Add("VERSION`t$version")
$lines.Add("")

for ($ci = 0; $ci -lt $charCount; $ci++) {
    $code = 32 + $ci
    $offset1 = $ints[$offsetTableStart + $ci]
    $offset2 = $ints[$offsetTableStart + $ci + 1]
    $numStrokes = $offset2 - $offset1

    # Advance width and left bearing
    $advance = 0
    $leftBearing = 0
    if ($version -eq 2) {
        $advance     = $ints[$advanceStart + $ci]
        $leftBearing = $ints[$leftBearingStart + $ci]
    }

    # Printable representation
    $printable = if ($code -ge 33 -and $code -le 126) { [char]$code } else { '.' }

    $lines.Add("CHAR`t$code`t$printable`t$advance`t$leftBearing")

    for ($i = $offset1; $i -lt $offset2; $i++) {
        $raw = $ints[$strokeDataStart + $i - 1]
        $s = Decode-Stroke $raw
        $lines.Add("`t$($s.Op)`t$($s.Dx)`t$($s.Dy)")
    }

    $lines.Add("")
}

# --- Write output ---
[System.IO.File]::WriteAllLines($OutputPath, $lines, [System.Text.Encoding]::UTF8)

$definedCount = 0
for ($ci = 0; $ci -lt $charCount; $ci++) {
    $offset1 = $ints[$offsetTableStart + $ci]
    $offset2 = $ints[$offsetTableStart + $ci + 1]
    if ($offset2 -gt $offset1) { $definedCount++ }
}

Write-Host "Written: $OutputPath"
Write-Host "  Version:     $version"
Write-Host "  Characters:  $charCount slots ($definedCount with glyph data)"
Write-Host "  Strokes:     $strokeCount total"
