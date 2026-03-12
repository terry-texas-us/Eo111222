# DXF code page fixtures

These fixtures support later DXF round-trip tests for text encoding behavior.

## Files
- `ansi_1252_ascii.dxf`
  - file bytes: Windows-1252
  - header token: `ANSI_1252`
  - non-ASCII payload uses CP1252-mappable characters such as `é`, `€`, and `—`
- `utf8_ascii.dxf`
  - file bytes: UTF-8 without BOM
  - header token: `UTF-8`
  - payload includes Cyrillic, Japanese, and Greek text
- `utf16_token_ascii.dxf`
  - file bytes: ASCII
  - header token: `UTF-16`
  - payload is ASCII-only on purpose so the fixture can exercise header-token preservation and binary UTF-16 rejection without requiring full UTF-16 text-file parsing

## Coverage
Each fixture includes:
- one `999` comment
- one named layer beyond layer `0`
- one `TEXT` entity
- one `MTEXT` entity
- a minimal `STYLE` table entry for `STANDARD`

These files are intentionally minimal and stable so later tests can focus on:
- header `$DWGCODEPAGE` preservation
- text decode and re-encode behavior
- layer-name round trips
- `TEXT` and `MTEXT` round trips
