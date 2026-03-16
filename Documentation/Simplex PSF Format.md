# Simplex PSF Stroke Font Format

## Overview

The **Simplex.psf** file is a fixed-size binary stroke font used by AeSys for all
vector-based text rendering. Each printable character is defined as a sequence of
pen-move and pen-draw instructions with relative displacements, stored as packed
32-bit integers.

The file is always **16 384 bytes** (4 096 `int32` values).

Two format versions exist:

| Version | Characters | Proportional | Detection |
|---------|------------|-------------|-----------|
| **v1** (legacy) | ASCII 32–126 (95 chars) | No — fixed-width advance | `int32[0] > 0` |
| **v2** (current) | Code points 32–255 (224 slots) | Yes — per-character advance width and left bearing | `int32[0] == −2` |

The v2 format is backward-compatible at the stroke level: glyph data is byte-identical
to v1; only the file header layout differs.

---

## File Layout

### v1 Layout

| Int32 Index | Count | Content |
|-------------|-------|---------|
| `[0 .. 95]` | 96 | **Offset table** — 95 character entries + 1 sentinel |
| `[96 .. 4095]` | 4 000 | **Stroke data** (capacity; ~1 137 used in Simplex) |

### v2 Layout

| Int32 Index | Count | Content |
|-------------|-------|---------|
| `[0]` | 1 | **Magic number** = `−2` (`0xFFFFFFFE`) |
| `[1 .. 225]` | 225 | **Offset table** — 224 character entries + 1 sentinel |
| `[226 .. 449]` | 224 | **Advance width table** (raw stroke X-units) |
| `[450 .. 673]` | 224 | **Left bearing table** (raw stroke X-units) |
| `[674 .. 4095]` | 3 422 | **Stroke data** (capacity; ~1 137 used in Simplex) |

These index values are defined as named constants in `Eo.h`:

```cpp
constexpr int strokeFontFileSizeInBytes       = 16384;
constexpr int strokeFontV1OffsetTableSize     = 96;
constexpr int strokeFontV1MaxCharacterCode    = 126;
constexpr int strokeFontV2MagicNumber         = -2;
constexpr int strokeFontV2OffsetTableStart    = 1;
constexpr int strokeFontV2OffsetTableEntries  = 225;
constexpr int strokeFontV2AdvanceWidthTableStart = 226;
constexpr int strokeFontV2LeftBearingTableStart  = 450;
constexpr int strokeFontV2StrokeDataStart     = 674;
constexpr int strokeFontV2MaxCharacterCode    = 255;
```

---

## Version Detection

```
if (int32[0] == −2)  → v2
else                  → v1   (int32[0] is always positive in v1: the offset for space)
```

---

## Offset Table

Each entry is a **1-based index** into the stroke data array. Character `C` (code point
32–126 for v1, 32–255 for v2) uses table index `C − 32`.

The strokes for character `C` are located at:
```
strokeData[ offsetTable[C − 32] − 1 ]   through
strokeData[ offsetTable[C − 32 + 1] − 2 ]
```

The last entry is a **sentinel** that marks one past the last stroke of the final
defined character. For characters without glyph data (128–255 initially), offset
entries equal the sentinel — producing an empty stroke range.

---

## Advance Width Table (v2 only)

224 `int32` values at indices `[226 .. 449]`. Each gives the **total cell width** of
the corresponding character in raw stroke X-units.

| Value | Meaning |
|-------|---------|
| `> 0` | Proportional advance width |
| `0` | Undefined character — renderer falls back to fixed advance |

The default advance for the Simplex font was computed as:
```
advance = max(28, drawnWidth + 12)
```
where `drawnWidth = maxCumulativeX − minCumulativeX` from the stroke data, and 12 raw
units provides 6 units of margin on each side. Space (code 32) is overridden to 50.

The minimum of 28 ensures narrow characters like `I` (drawnWidth = 1) and `l`
(drawnWidth = 0) maintain readable separation.

---

## Left Bearing Table (v2 only)

224 `int32` values at indices `[450 .. 673]`. Each gives the **horizontal offset** to
subtract from the pen position before rendering the character's strokes, in raw stroke
X-units.

```
leftBearing = max(0, minCumulativeX − 6)
```

This aligns all characters to a consistent 6-unit left margin within their proportional
cell. Without this offset, narrow characters (whose strokes begin near the center of
the old fixed-width cell) would appear displaced to the right.

---

## Stroke Data

### Packed Format

Each stroke is a single `int32` with three fields:

```
Bits  31–24  (8 bits):  Opcode
Bits  23–12  (12 bits): X displacement (sign-magnitude)
Bits  11–0   (12 bits): Y displacement (sign-magnitude)
```

Equivalently, using integer arithmetic:

```
opcode = value / 16777216             (value >> 24)
rawX   = (value / 4096) % 4096       (value >> 12) & 0xFFF
rawY   = value % 4096                 value & 0xFFF
```

### Sign-Magnitude Decoding

Each 12-bit displacement uses **sign-magnitude** encoding:

- Bit 11 (value `2048`): sign flag — `0` = positive, `1` = negative
- Bits 10–0: magnitude (0–2047)

```
if (raw & 0x800) != 0:
    decoded = -(raw - 2048)       # negative
else:
    decoded = raw                  # positive
```

Valid range: −2047 to +2047.

### Opcode

| Opcode | Meaning |
|--------|---------|
| `5` | **Move** — lift pen, reposition without drawing |
| Any other | **Draw** — draw a line segment to the new position |

In practice, opcode `0` means draw and `5` means move. The first stroke of every
character is a MOVE.

### Rendering Coordinates

Displacements are **relative** and accumulate from the character's origin. The renderer
converts raw units to normalized coordinates using `Eo::defaultCharacterCellAspectRatio`
(0.6):

```
renderX += rawX × 0.01 / 0.6        (÷ aspect ratio)
renderY += rawY × 0.01              (direct scale)
```

This means the X axis is stretched by `1/0.6 ≈ 1.667×` relative to Y, matching the
0.6 width-to-height character cell aspect ratio.

### Rendering with Left Bearing (v2)

Before the stroke loop for each character, the renderer applies:
```
penX −= leftBearing × 0.01 / 0.6
```

### Character Advance

After rendering all strokes, the pen position advances by:

```
v1 (fixed):   advance = 1.0 + (0.32 + characterSpacing) / 0.6
v2 (proportional):
    cellWidth = advanceWidth × 0.01 / 0.6
    advance   = cellWidth + (0.32 + characterSpacing) / 0.6
    (falls back to v1 fixed advance if advanceWidth == 0)
```

---

## Hershey Origin

The Simplex.psf glyphs derive from the **Hershey Simplex** vector font (public domain,
Dr. Allen V. Hershey, 1967). The coordinate mapping from Hershey to PSF is:

```
psf_x = hershey_x × 3.75 + 50
psf_y = hershey_y × 4.714 + 43.43  (approximate)
```

The original Hershey proportional width information (left/right bearing values) was
discarded in the v1 conversion. The v2 advance width and left bearing tables
reconstruct proportional metrics from the stroke geometry.

---

## Example Glyphs

### `!` (code 33) — 7 strokes, advance=28, leftBearing=44

| # | Raw (hex) | dX | dY | Op |
|---|-----------|----|----|-----|
| 1 | `0x05032064` | +50 | +100 | MOVE |
| 2 | `0x00000842` | +1 | −66 | DRAW |
| 3 | `0x05000818` | +1 | −24 | MOVE |
| 4 | `0x00003805` | +4 | −5 | DRAW |
| 5 | `0x00803804` | −4 | −4 | DRAW |
| 6 | `0x00003004` | +3 | +4 | DRAW |
| 7 | `0x00803005` | −3 | +5 | DRAW |

### `A` (code 65) — 6 strokes, advance=72, leftBearing=14

| # | Raw (hex) | dX | dY | Op |
|---|-----------|----|----|-----|
| 1 | `0x05032064` | +50 | +100 | MOVE |
| 2 | `0x0081E863` | −31 | −99 | DRAW |
| 3 | `0x0501E063` | +30 | +99 | MOVE |
| 4 | `0x0001E863` | +31 | −99 | DRAW |
| 5 | `0x05831021` | −49 | +33 | DRAW |
| 6 | `0x00026000` | +38 | +0 | DRAW |

### `I` (code 73) — 2 strokes, advance=28, leftBearing=44

| # | Raw (hex) | dX | dY | Op |
|---|-----------|----|----|-----|
| 1 | `0x05032064` | +50 | +100 | MOVE |
| 2 | `0x00000863` | +1 | −99 | DRAW |

---

## Capacity

| Metric | v1 | v2 |
|--------|----|----|
| Character slots | 95 (ASCII 32–126) | 224 (code points 32–255) |
| Stroke data capacity | 4 000 int32s | 3 422 int32s |
| Simplex strokes used | 1 137 | 1 137 |
| Free stroke entries | 2 863 | 2 285 |
| Approx. new glyphs (at 12 strokes avg) | ~238 | ~190 |

---

## Tooling

| Script | Purpose |
|--------|---------|
| `Tools/ConvertStrokeFontV1ToV2.ps1` | One-time migration from v1 binary to v2 binary |
| `Tools/ConvertPsfToText.ps1` | Decompile binary `.psf` to human-editable `.psf.txt` |
| `Tools/ConvertTextToPsf.ps1` | Compile `.psf.txt` back to binary `.psf` |

The text round-trip tools enable glyph authoring and inspection without a hex editor.

---

## Related Source Files

| File | Role |
|------|------|
| `Application/AeSys/Eo.h` | Format constants |
| `Application/AeSys/AeSys.h` | `m_StrokeFontVersion`, `StrokeFontVersion()` |
| `Application/AeSys/AeSys.cpp` | `LoadSimplexStrokeFont()` — loader with v2 detection |
| `Application/AeSys/EoDbText.cpp` | `DisplayTextSegmentUsingStrokeFont()` — renderer |
