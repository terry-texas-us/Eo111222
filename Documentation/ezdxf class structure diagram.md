ezdxf.entities.DXFEntity                  # ← ROOT BASE for EVERYTHING
    ├── dxf.handle
    ├── dxf.owner
    ├── dxf namespace (all DXF attributes)
    ├── doc, uuid, is_alive, is_virtual, etc.
    ├── extension dictionary, XDATA, AppData, reactors
    │
    ├── ezdxf.entities.DXFGraphic         # ← GRAPHICAL BASE (what you asked about)
    │       ├── dxf.layer, dxf.color, dxf.linetype, dxf.lineweight
    │       ├── dxf.ltscale, dxf.invisible, dxf.paperspace
    │       ├── dxf.extrusion (210/220/230)
    │       ├── dxf.thickness, true_color, transparency, shadow_mode
    │       ├── ocs(), transform(), graphic_properties(), etc.
    │       │
    │       ├── Text, MText, Line, Circle, Arc, Hatch, Insert, ...
    │       └── (your C++ TextEntity and MTextEntity should live here)
    │
    └── ezdxf.entities.DXFObject          # ← NON-GRAPHICAL OBJECTS
            ├── Dictionary, XRecord, Layout, Material, ...
            └── (no extrusion, no layer, no geometry)