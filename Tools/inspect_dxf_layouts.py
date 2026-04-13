"""Inspect DXF layout and viewport data for debugging."""
import sys
import ezdxf

def inspect(filepath):
    doc = ezdxf.readfile(filepath)
    hdr = doc.header
    print(f"File: {filepath}")
    print(f"  $INSUNITS:    {hdr.get('$INSUNITS', 'not set')}")
    print(f"  $MEASUREMENT: {hdr.get('$MEASUREMENT', 'not set')}")
    print()

    for layout in doc.layouts:
        name = layout.name
        ps = layout.dxf_layout
        print(f"Layout: '{name}'")
        print(f"  limmin:           {ps.dxf.limmin}")
        print(f"  limmax:           {ps.dxf.limmax}")
        print(f"  paper_width:      {ps.dxf.paper_width} mm")
        print(f"  paper_height:     {ps.dxf.paper_height} mm")
        print(f"  plot_paper_units: {ps.dxf.plot_paper_units}  (0=inch, 1=mm, 2=pixel)")

        # Find viewports in this layout
        for entity in layout:
            if entity.dxftype() == "VIEWPORT":
                vid = entity.dxf.get("id", "?")
                cx, cy, cz = entity.dxf.center
                w = entity.dxf.width
                h = entity.dxf.height
                vh = entity.dxf.get("view_height", 0)
                vcx = entity.dxf.get("view_center_point", (0, 0))
                print(f"  VIEWPORT id={vid}")
                print(f"    center:      ({cx:.4f}, {cy:.4f}, {cz:.4f})")
                print(f"    size:        {w:.4f} x {h:.4f}")
                print(f"    viewCenter:  {vcx}")
                print(f"    viewHeight:  {vh}")
        print()

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "DXF Test Files/Single_Viewport_Basic.dxf"
    inspect(path)
