#Requires -Version 5.1
<#
.SYNOPSIS
    Generates 32x32 toolbar bitmap strips from Segoe Fluent Icons (Windows 11)
    or Segoe MDL2 Assets (Windows 10 fallback).

.DESCRIPTION
    Produces two BMP files pre-rendered against the correct toolbar background
    color for each color scheme so that anti-aliased glyph edges blend seamlessly
    with no "halo" artifact.

    Dark  variant: light glyphs RGB(214,212,207) on dark  bg RGB(40,40,36)
    Light variant: dark  glyphs RGB(34,33,30)   on light bg RGB(240,239,236)

    Both background colors are used as the MFC transparent (mask) color so the
    bitmap background disappears during toolbar rendering.

    Button order matches the IDR_MAINFRAME_32 TOOLBAR resource declaration:
      ID_FILE_NEW  ID_FILE_OPEN  ID_FILE_SAVE  ID_FILE_SAVE_ALL
      ID_EDIT_TRAPCUT  ID_EDIT_TRAPCOPY  ID_EDIT_TRAPPASTE
      ID_FILE_PRINTCURRENTVIEW  ID_APP_ABOUT  ID_CONTEXT_HELP

    Segoe Fluent Icons / Segoe MDL2 Assets — Unicode PUA codepoint reference
    -------------------------------------------------------------------------
    U+E749  Print        Printer outline
    U+E74E  Save         Floppy disk
    U+E77F  Paste        Clipboard with page
    U+E792  SaveLocal    Floppy disk with down-arrow (SaveAll — distinct from plain Save)
    U+E838  OpenWith     Folder with open page (Open)
    U+E897  Help         Circle with question mark
    U+E8A5  Page         Blank document (New)
    U+E8C6  Cut          Scissors
    U+E8C8  Copy         Overlapping pages
    U+E946  Info         Circle with 'i' (About)

.EXAMPLE
    # Run from the repository root:
    .\Tools\GenerateToolbarBitmaps.ps1

.NOTES
    Requires .NET System.Drawing (available in Windows PowerShell 5.1 and
    PowerShell 7+ on Windows with the GDI drawing assemblies present).
#>

Add-Type -AssemblyName System.Drawing

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

$iconSize  = 32   # pixels per icon cell (square)
$outDir    = Join-Path $PSScriptRoot "..\Application\AeSys\res\Toolbar Bitmaps"

# Button definitions — Unicode PUA codepoints for Segoe Fluent Icons / MDL2 Assets.
# The array order MUST match the TOOLBAR resource button order in AeSys.rc.
$buttons = @(
    @{ Id = 'ID_FILE_NEW';              Char = [char]0xE8A5; Label = 'New'       },  # Page / New document
    @{ Id = 'ID_FILE_OPEN';             Char = [char]0xE838; Label = 'Open'      },  # FolderOpen
    @{ Id = 'ID_FILE_SAVE';             Char = [char]0xE74E; Label = 'Save'      },  # Save
    @{ Id = 'ID_FILE_SAVE_ALL';         Char = [char]0xE792; Label = 'SaveAll'   },  # SaveLocal — floppy+down-arrow (distinct from plain Save U+E74E)
    @{ Id = 'ID_EDIT_TRAPCUT';          Char = [char]0xE8C6; Label = 'Cut'       },  # Cut
    @{ Id = 'ID_EDIT_TRAPCOPY';         Char = [char]0xE8C8; Label = 'Copy'      },  # Copy
    @{ Id = 'ID_EDIT_TRAPPASTE';        Char = [char]0xE77F; Label = 'Paste'     },  # Paste
    @{ Id = 'ID_FILE_PRINTCURRENTVIEW'; Char = [char]0xE749; Label = 'Print'     },  # Print
    @{ Id = 'ID_APP_ABOUT';             Char = [char]0xE946; Label = 'About'     },  # Info circle
    @{ Id = 'ID_CONTEXT_HELP';          Char = [char]0xE897; Label = 'Help'      }   # Unknown / Help
)

# Color scheme values — must stay in sync with Eo::darkSchemeColors / lightSchemeColors in Eo.h
$schemes = @(
    @{
        Name      = 'dark'
        GlyphRgb  = [System.Drawing.Color]::FromArgb(214, 212, 207)   # darkSchemeColors.paneText
        BgRgb     = [System.Drawing.Color]::FromArgb(40,  40,  36)    # darkSchemeColors.toolbarBackground (= transparent key)
        OutFile   = 'Toolbar-dark-32.bmp'
    },
    @{
        Name      = 'light'
        GlyphRgb  = [System.Drawing.Color]::FromArgb(34,  33,  30)    # lightSchemeColors.paneText
        BgRgb     = [System.Drawing.Color]::FromArgb(240, 239, 236)   # lightSchemeColors.toolbarBackground (= transparent key)
        OutFile   = 'Toolbar-light-32.bmp'
    }
)

# ---------------------------------------------------------------------------
# Font selection: prefer Segoe Fluent Icons (Win11), fall back to MDL2 Assets
# ---------------------------------------------------------------------------

$availableFamilies = [System.Drawing.FontFamily]::Families | Select-Object -ExpandProperty Name
$iconFontName = if ($availableFamilies -contains 'Segoe Fluent Icons') {
    'Segoe Fluent Icons'
} elseif ($availableFamilies -contains 'Segoe MDL2 Assets') {
    'Segoe MDL2 Assets'
} else {
    Write-Error "Neither 'Segoe Fluent Icons' nor 'Segoe MDL2 Assets' is installed. Aborting."
    exit 1
}
Write-Host "Icon font: $iconFontName"

# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------

function New-ToolbarBitmap {
    param(
        [System.Drawing.Color] $glyphColor,
        [System.Drawing.Color] $bgColor,
        [string]               $outputPath
    )

    # Super-sampling: render at 3x resolution then downsample to $iconSize.
    # At 3x the per-stroke width is ~6–9 px; bicubic downsampling collapses it
    # to ~1–2 px at final size, matching the thin-stroke VS2026 icon style.
    $scale      = 3
    $renderSize = $iconSize * $scale          # 96 px cell
    $totalWidth = $buttons.Count * $iconSize  # final strip width

    # --- High-resolution pass (96 px cells) ---
    $bmpHi = New-Object System.Drawing.Bitmap(
        ($totalWidth * $scale), $renderSize,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

    $g = [System.Drawing.Graphics]::FromImage($bmpHi)
    $g.Clear($bgColor)

    # ClearTypeGridFit snaps glyph stems to sub-pixel boundaries at the 3x size,
    # producing the sharpest possible strokes before downsampling.
    $g.TextRenderingHint  = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
    $g.SmoothingMode      = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.InterpolationMode  = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

    # 60 px font in a 96 px cell → same proportional fill as 20 px in 32 px.
    $font  = New-Object System.Drawing.Font(
        $iconFontName, ($iconSize * $scale * 20 / $iconSize),
        [System.Drawing.FontStyle]::Regular,
        [System.Drawing.GraphicsUnit]::Pixel)
    $brush = New-Object System.Drawing.SolidBrush($glyphColor)

    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment     = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
    $sf.FormatFlags   = [System.Drawing.StringFormatFlags]::NoWrap

    for ($i = 0; $i -lt $buttons.Count; $i++) {
        $rect = [System.Drawing.RectangleF]::new(
            ($i * $renderSize), 0, $renderSize, $renderSize)
        $g.DrawString($buttons[$i].Char, $font, $brush, $rect, $sf)
    }

    $g.Dispose()
    $font.Dispose()
    $brush.Dispose()
    $sf.Dispose()

    # --- Downsample pass (32 px cells) ---
    # HighQualityBicubic averages the 9 source pixels (3x3 kernel) per output pixel,
    # blending glyph strokes cleanly against $bgColor so the MFC transparent-key mask
    # on $bgColor remains accurate at the output resolution.
    $bmpOut = New-Object System.Drawing.Bitmap(
        $totalWidth, $iconSize,
        [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)

    $gOut = [System.Drawing.Graphics]::FromImage($bmpOut)
    $gOut.Clear($bgColor)
    $gOut.InterpolationMode  = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $gOut.SmoothingMode      = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $gOut.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $gOut.CompositingMode    = [System.Drawing.Drawing2D.CompositingMode]::SourceOver

    $srcRect = [System.Drawing.Rectangle]::new(0, 0, $bmpHi.Width, $bmpHi.Height)
    $dstRect = [System.Drawing.Rectangle]::new(0, 0, $totalWidth,   $iconSize)
    $gOut.DrawImage($bmpHi, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)

    $gOut.Dispose()
    $bmpHi.Dispose()

    $bmpOut.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmpOut.Dispose()
}

# ---------------------------------------------------------------------------
# Generate
# ---------------------------------------------------------------------------

if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

foreach ($scheme in $schemes) {
    $outPath = Join-Path $outDir $scheme.OutFile
    New-ToolbarBitmap -glyphColor $scheme.GlyphRgb -bgColor $scheme.BgRgb -outputPath $outPath
    $item = Get-Item $outPath
    Write-Host "  $($scheme.Name): $outPath ($($item.Length) bytes)"
}

Write-Host ""
Write-Host "Done. Rebuild AeSys to pick up the new bitmaps."
