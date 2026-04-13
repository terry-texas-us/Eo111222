"""
GenerateLayoutTests.py — Generate DXF test files for AeSys layout/viewport phases.

All files use imperial units (inches) with ARCH sheet sizes.
Output directory: ../DXF Test Files/

Requires: ezdxf >= 1.4.0
"""

import os
import sys
import ezdxf
from ezdxf import units

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "DXF Test Files")
if len(sys.argv) > 1:
    OUTPUT_DIR = sys.argv[1]


def _ensure_output_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)


# ---------------------------------------------------------------------------
# 1. Single_Viewport_Basic.dxf
#    Simplest case: one layout, one viewport, basic model content.
#    ARCH B sheet (18" x 12"), single centered viewport.
# ---------------------------------------------------------------------------
def generate_single_viewport_basic():
    doc = ezdxf.new(dxfversion="R2018")
    doc.units = units.IN

    msp = doc.modelspace()
    # Rectangle 100 x 50 (lines on layer "0")
    msp.add_line((0, 0), (100, 0), dxfattribs={"color": 7})
    msp.add_line((100, 0), (100, 50), dxfattribs={"color": 7})
    msp.add_line((100, 50), (0, 50), dxfattribs={"color": 7})
    msp.add_line((0, 50), (0, 0), dxfattribs={"color": 7})
    # Circle centered at (50, 25), radius 20
    msp.add_circle((50, 25), radius=20, dxfattribs={"color": 1})

    layout = doc.layouts.new("Simple")
    doc.layouts.delete("Layout1")
    layout.page_setup(size=(18, 12), margins=(0, 0, 0, 0), units="inch")

    # Viewport: centered on sheet, 16" x 10.5" (1" margin all around)
    layout.add_viewport(
        center=(9, 6),
        size=(16, 10.5),
        view_center_point=(50, 25, 0),
        view_height=60.0,
    )

    # Paper-space border: green rectangle with 0.5" margin from sheet edge
    doc.layers.add("Border", color=3)
    border_margin = 0.5
    bx0, by0 = border_margin, border_margin
    bx1, by1 = 18 - border_margin, 12 - border_margin
    layout.add_line((bx0, by0), (bx1, by0), dxfattribs={"color": 3, "layer": "Border"})
    layout.add_line((bx1, by0), (bx1, by1), dxfattribs={"color": 3, "layer": "Border"})
    layout.add_line((bx1, by1), (bx0, by1), dxfattribs={"color": 3, "layer": "Border"})
    layout.add_line((bx0, by1), (bx0, by0), dxfattribs={"color": 3, "layer": "Border"})

    path = os.path.join(OUTPUT_DIR, "Single_Viewport_Basic.dxf")
    doc.saveas(path)
    print(f"  Created {path}")


# ---------------------------------------------------------------------------
# 2. Two_Viewports_Same_Layout.dxf
#    One layout with two side-by-side viewports showing different views.
#    ARCH C sheet (24" x 18").
# ---------------------------------------------------------------------------
def generate_two_viewports_same_layout():
    doc = ezdxf.new(dxfversion="R2018")
    doc.units = units.IN

    msp = doc.modelspace()
    # L-shaped polyline
    pts = [(0, 0), (80, 0), (80, 30), (40, 30), (40, 60), (0, 60)]
    poly = msp.add_lwpolyline(pts, dxfattribs={"color": 3})
    poly.closed = True
    # Circle at (20, 20) r=10
    msp.add_circle((20, 20), radius=10, dxfattribs={"color": 1})
    # Circle at (60, 15) r=8
    msp.add_circle((60, 15), radius=8, dxfattribs={"color": 5})

    layout = doc.layouts.new("TwoVP")
    doc.layouts.delete("Layout1")
    layout.page_setup(size=(24, 18), margins=(0, 0, 0, 0), units="inch")

    # Left viewport: overview of entire model
    layout.add_viewport(
        center=(6.5, 9),
        size=(11, 14),
        view_center_point=(40, 30, 0),
        view_height=70.0,
    )
    # Right viewport: zoomed detail of lower-right area
    layout.add_viewport(
        center=(17.5, 9),
        size=(11, 14),
        view_center_point=(60, 15, 0),
        view_height=30.0,
    )

    path = os.path.join(OUTPUT_DIR, "Two_Viewports_Same_Layout.dxf")
    doc.saveas(path)
    print(f"  Created {path}")


# ---------------------------------------------------------------------------
# 3. Three_Layouts.dxf
#    Three layouts with different ARCH sheet sizes and viewport configs.
#    Tests multi-layout tab bar + per-layout paper-space routing.
# ---------------------------------------------------------------------------
def generate_three_layouts():
    doc = ezdxf.new(dxfversion="R2018")
    doc.units = units.IN

    msp = doc.modelspace()
    # Grid of 4 squares
    for col in range(2):
        for row in range(2):
            ox, oy = col * 50, row * 50
            pts = [(ox, oy), (ox + 40, oy), (ox + 40, oy + 40), (ox, oy + 40)]
            p = msp.add_lwpolyline(pts, dxfattribs={"color": col * 2 + row + 1})
            p.closed = True

    # Layout 1 — ARCH A (12" x 9")
    lay1 = doc.layouts.new("ArchA")
    doc.layouts.delete("Layout1")
    lay1.page_setup(size=(12, 9), margins=(0, 0, 0, 0), units="inch")
    lay1.add_viewport(
        center=(6, 4.5),
        size=(10, 7),
        view_center_point=(45, 45, 0),
        view_height=110.0,
    )

    # Layout 2 — ARCH B (18" x 12")
    lay2 = doc.layouts.new("ArchB")
    lay2.page_setup(size=(18, 12), margins=(0, 0, 0, 0), units="inch")
    lay2.add_viewport(
        center=(9, 6),
        size=(16, 10),
        view_center_point=(45, 45, 0),
        view_height=110.0,
    )

    # Layout 3 — ARCH D (36" x 24")
    lay3 = doc.layouts.new("ArchD")
    lay3.page_setup(size=(36, 24), margins=(0, 0, 0, 0), units="inch")
    lay3.add_viewport(
        center=(18, 12),
        size=(34, 22),
        view_center_point=(45, 45, 0),
        view_height=110.0,
    )

    path = os.path.join(OUTPUT_DIR, "Three_Layouts.dxf")
    doc.saveas(path)
    print(f"  Created {path}")


# ---------------------------------------------------------------------------
# 4. Viewport_Scale_Presets.dxf
#    Tests Phase 12 scale combo: viewport at exact 1:2 scale.
#    ARCH B sheet (18" x 12"), viewport paper height = 10", viewHeight = 20"
#    → scale = 10/20 = 0.5 = 1:2.
# ---------------------------------------------------------------------------
def generate_viewport_scale_presets():
    doc = ezdxf.new(dxfversion="R2018")
    doc.units = units.IN

    msp = doc.modelspace()
    # 30" x 20" rectangle
    pts = [(0, 0), (30, 0), (30, 20), (0, 20)]
    p = msp.add_lwpolyline(pts, dxfattribs={"color": 7})
    p.closed = True
    # Diagonal cross
    msp.add_line((0, 0), (30, 20), dxfattribs={"color": 1})
    msp.add_line((30, 0), (0, 20), dxfattribs={"color": 1})

    layout = doc.layouts.new("Scale1to2")
    doc.layouts.delete("Layout1")
    layout.page_setup(size=(18, 12), margins=(0, 0, 0, 0), units="inch")

    # Viewport: paper size 15" x 10", viewHeight 20" → scale = 10/20 = 0.5 (1:2)
    layout.add_viewport(
        center=(9, 6),
        size=(15, 10),
        view_center_point=(15, 10, 0),
        view_height=20.0,
    )

    path = os.path.join(OUTPUT_DIR, "Viewport_Scale_Presets.dxf")
    doc.saveas(path)
    print(f"  Created {path}")


# ---------------------------------------------------------------------------
# 5. Layout_Limits_Sheet.dxf
#    Tests DisplayPaperSpaceSheet layout-limit discovery path.
#    Explicit LIMMIN/LIMMAX set via page_setup on ARCH C (24" x 18").
# ---------------------------------------------------------------------------
def generate_layout_limits_sheet():
    doc = ezdxf.new(dxfversion="R2018")
    doc.units = units.IN

    msp = doc.modelspace()
    # Simple triangle
    msp.add_line((0, 0), (60, 0), dxfattribs={"color": 3})
    msp.add_line((60, 0), (30, 50), dxfattribs={"color": 3})
    msp.add_line((30, 50), (0, 0), dxfattribs={"color": 3})

    layout = doc.layouts.new("LimitsTest")
    doc.layouts.delete("Layout1")
    layout.page_setup(size=(24, 18), margins=(0, 0, 0, 0), units="inch")

    layout.add_viewport(
        center=(12, 9),
        size=(22, 16),
        view_center_point=(30, 25, 0),
        view_height=60.0,
    )

    path = os.path.join(OUTPUT_DIR, "Layout_Limits_Sheet.dxf")
    doc.saveas(path)
    print(f"  Created {path}")


# ---------------------------------------------------------------------------
# 6. Viewport_Activation_MultiVP.dxf
#    Two viewports at different scales for testing Phase 7 activation
#    and Phase 12 scale display.
#    ARCH D sheet (36" x 24").
# ---------------------------------------------------------------------------
def generate_viewport_activation_multivp():
    doc = ezdxf.new(dxfversion="R2018")
    doc.units = units.IN

    msp = doc.modelspace()
    # Star pattern
    import math
    cx, cy, r = 50, 50, 40
    for i in range(5):
        a1 = math.radians(90 + i * 72)
        a2 = math.radians(90 + ((i + 2) % 5) * 72)
        msp.add_line(
            (cx + r * math.cos(a1), cy + r * math.sin(a1)),
            (cx + r * math.cos(a2), cy + r * math.sin(a2)),
            dxfattribs={"color": 2},
        )
    msp.add_circle((cx, cy), radius=r, dxfattribs={"color": 5})

    layout = doc.layouts.new("MultiVP")
    doc.layouts.delete("Layout1")
    layout.page_setup(size=(36, 24), margins=(0, 0, 0, 0), units="inch")

    # Left viewport: full model, 1:5 scale area
    # Paper 16x20, viewHeight=100 → scale=20/100=0.2 (1:5)
    layout.add_viewport(
        center=(9, 12),
        size=(16, 20),
        view_center_point=(50, 50, 0),
        view_height=100.0,
    )

    # Right viewport: zoomed to center, 1:2 scale area
    # Paper 16x20, viewHeight=40 → scale=20/40=0.5 (1:2)
    layout.add_viewport(
        center=(27, 12),
        size=(16, 20),
        view_center_point=(50, 50, 0),
        view_height=40.0,
    )

    path = os.path.join(OUTPUT_DIR, "Viewport_Activation_MultiVP.dxf")
    doc.saveas(path)
    print(f"  Created {path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    _ensure_output_dir()
    print("Generating DXF layout test files (imperial units)...")
    generate_single_viewport_basic()
    generate_two_viewports_same_layout()
    generate_three_layouts()
    generate_viewport_scale_presets()
    generate_layout_limits_sheet()
    generate_viewport_activation_multivp()
    print("Done.")


if __name__ == "__main__":
    main()