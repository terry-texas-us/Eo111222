"""
Generate Tools/test_fixtures/aesys_test.dxf — deterministic AeSys pipe-test fixture.

Geometry layout (all in model space, layer "Rooms"):
  ROOM_A  closed polygon  (0,0)→(10,0)→(10,8)→(0,8)→(0,0)   area = 80.0
  ROOM_B  closed polygon  (20,0)→(30,0)→(30,6)→(20,6)→(20,0) area = 60.0

Layer "Walls":
  One open polyline  (0,0)→(30,0)   — not a room, should not be matched by QUERY ROOM

Block table (seeded so QUERY BLOCK AT works):
  Block "DESK" defined at origin — a 2×1 rectangle
  One INSERT of DESK at (5,4,0) — centre of ROOM_A
  ATTDEF "LABEL" on DESK with default value "DESK-1"

Layer "Work" (name matches AeSys work-layer convention):
  One line (50,0)→(60,0) — used for QUERY LAYER name test

Run this script once to (re)generate the fixture:
    python Tools/test_fixtures/make_test_drawing.py
"""
from __future__ import annotations

import math
import os
import sys

try:
    import ezdxf
except ImportError:
    print("ezdxf not installed — run: pip install ezdxf", file=sys.stderr)
    sys.exit(1)

OUTPUT = os.path.join(os.path.dirname(__file__), "aesys_test.dxf")

# ---------------------------------------------------------------------------
# Create drawing
# ---------------------------------------------------------------------------
doc = ezdxf.new("R2010", setup=True)
msp = doc.modelspace()

# ---------------------------------------------------------------------------
# Layer definitions
# ---------------------------------------------------------------------------
doc.layers.add("Rooms",  color=3)   # green
doc.layers.add("Walls",  color=7)   # white
doc.layers.add("Work",   color=1)   # red

# ---------------------------------------------------------------------------
# ROOM_A — closed polygon 10 × 8, lower-left at (0,0), area = 80
# ---------------------------------------------------------------------------
room_a_pts = [(0, 0, 0), (10, 0, 0), (10, 8, 0), (0, 8, 0), (0, 0, 0)]
msp.add_lwpolyline(
    [(x, y) for x, y, *_ in room_a_pts],
    dxfattribs={"layer": "Rooms", "closed": True},
)

# ---------------------------------------------------------------------------
# ROOM_B — closed polygon 10 × 6, lower-left at (20,0), area = 60
# ---------------------------------------------------------------------------
room_b_pts = [(20, 0, 0), (30, 0, 0), (30, 6, 0), (20, 6, 0), (20, 0, 0)]
msp.add_lwpolyline(
    [(x, y) for x, y, *_ in room_b_pts],
    dxfattribs={"layer": "Rooms", "closed": True},
)

# ---------------------------------------------------------------------------
# Walls — open polyline spanning both rooms; NOT a closed room
# ---------------------------------------------------------------------------
msp.add_lwpolyline(
    [(0, 0), (30, 0)],
    dxfattribs={"layer": "Walls"},
)

# ---------------------------------------------------------------------------
# Block "DESK" — 2 × 1 rectangle with one ATTDEF "LABEL"
# ---------------------------------------------------------------------------
blk = doc.blocks.new(name="DESK")
blk.add_lwpolyline(
    [(0, 0), (2, 0), (2, 1), (0, 1), (0, 0)],
    dxfattribs={"closed": True},
)
blk.add_attdef(
    tag="LABEL",
    insert=(1, -0.3),
    text="DESK-1",
    dxfattribs={"height": 0.25, "layer": "0"},
)

# INSERT of DESK at (5, 4, 0) — inside ROOM_A
# Use add_blockref (plain INSERT) so the entity name is "DESK", not "*Unn".
insert = msp.add_blockref(
    name="DESK",
    insert=(5, 4, 0),
    dxfattribs={"layer": "Rooms"},
)
# Add the ATTRIB manually so it survives as a direct INSERT with attributes.
attrib = insert.add_attrib(
    tag="LABEL",
    text="DESK-1",
    insert=(6, 3.7, 0),
    dxfattribs={"height": 0.25, "layer": "0"},
)

# ---------------------------------------------------------------------------
# Work layer — one line
# ---------------------------------------------------------------------------
msp.add_line(start=(50, 0, 0), end=(60, 0, 0), dxfattribs={"layer": "Work"})

# ---------------------------------------------------------------------------
# Save
# ---------------------------------------------------------------------------
doc.saveas(OUTPUT)
print(f"Written: {OUTPUT}")
print(f"  ROOM_A area = {10 * 8} (expected 80)")
print(f"  ROOM_B area = {10 * 6} (expected 60)")
print("  Block DESK inserted at (5,4,0) inside ROOM_A")
print("  Layer 'Work': 1 line")
