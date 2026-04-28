**AeSys (PegAESys) User Interface Architectural Roadmap**

PegAESys (also referred to as AeSys or PegAEsys in the documentation) is a professional CADD application for architectural-engineering systems in the construction industry. Its UI is deliberately **keystroke- and menu-driven** (no command line, no icons, no modal dialogs for core drafting). The design philosophy prioritizes keeping hands on controls and eyes on the drawing area at all times, using **Graphic Input (GIN) modes**, transparent overlays, unique cursors per mode, and numeric-keypad command sets. It employs a **cylindrical coordinate system** (polar + Cartesian on the XY plane, pure Cartesian altitude/Z).

The architecture is built around **persistent state** (Trap buffer, engaged segments, home points, render properties) that survives mode switches. All parameters are configurable via **MenuBar → Setup → Options** (or shortcuts like **P** for pen color, **L** for line type, **T** for text, **O** for options). The system supports Windows (7+ 64-bit), mouse + numeric keypad (recommended), and 2+ monitors.

### 1. Core Display Architecture (Maximal Drawing Area, Transparent Overlays)
The screen maximizes drawing real estate with minimal chrome:

- **Caption Bar (Line 1, Windows-mandated)**: PegAESys icon + version + **current [Mode]** + engaged-segment report (primitive type, color #, style #, dimension @ angle). Also hosts standard Windows controls.
- **MenuBar (Line 2)**: Accessed via **<Alt>** or mouse. Top-level: **File** (New/Open/Save/Save As/Close/Import DWG/Export/Print Manager/Plot Manager/Recent/Recover/Run/Exit), **Edit** (clipboard), **View**, **Mode** (all GIN modes), **Tools** (relative moves/rotation, segment/primitive ops, home points), **Setup** (Profiles, Options, Tasks, Cursor, Mouse, Constraints), **Help**, **Proj:Top** (projected view), **Clear All**.
- **Status Bar (transparent overlay in drawing area)**: Real-time reports — Segment count | Trap count | Pen color/style | Text height | Scale (e.g., 1:96) | Screen ratio | Tab length/angle (ortho/skew). Toggle **F3**. Transparency & colors via Setup > Options > Appearance.
- **Tool Bar (transparent pseudo-bar at bottom)**: Dynamic menu showing the **10 numeric commands** for the current GIN mode (changes with mode; pending ops highlighted in different color). Toggle **F3**.
- **Odometer (transparent top-right box, auto for 3D)**: Cursor tracking — Layer #, Pen #, Line # + Cartesian (X/Y/Z from working origin) + Polar (distance/angle from home point). Toggle **F4**.
- **Background**: Default solid black (best for colored pens). Customizable (white for monotone/print preview **F8**); bitmap import (**File > Manage > Add BMP**, toggle **F9**).
- **Cursors**: One unique cursor **per GIN mode** for instant visual feedback (e.g., + cross for Draw, different variants for Trap_IN/OUT, Edit, Annotate, Cut, Nodal_IN/OUT, Dimension, Fixup, Tasks, Draw2). 

All display elements (transparency, colors, markers, fonts, scrollers, origin markers) are set in **MenuBar → Setup → Options → Appearance** (or General/Profile tabs). Layers: colored = editable; gray = non-editable.

### 2. Input Architecture (Keyboard-Centric, Minimal Mouse)
- **Mouse**: Primary cursor movement and picking. Recommended: Kingston Roller Ball (precise).
- **QWERTY Keyboard** (global + mode-independent):
  - Global: **Esc** (cancel), **Enter** (complete), **?** (query layer of segment at cursor), **:** (segment pick-and-drag), **;** (primitive pick-and-drag), **M** (mend engaged primitive with rubber-banding), **Del** / **<Shift>Del** (delete segment/primitive), **Insert** (undelete), **<Ctrl>Home** (move segment to current layer), **N** / **<Alt>N** (new/extract primitive attribute), **#** / **$** (get number/string), **P**/**L**/**T**/**K** (pen color/line type/text/fill options), **/** (File menu), **=**/**+**/**-** (zoom), **O** (options menu).
  - **Diamond Keys** (ESDX ortho + RWZC skew + F/V altitude): **.** sets TAB dimension; **>** sets TAB angle. Used for precise cursor jumps while rubber-banding.
  - **Function Keys (F1–F12)**: **F1** (context help), **F2** (stroke vs. TrueType text toggle), **F3** (toolbar/status on/off), **F4** (odometer), **F5** (refresh), **F6** (colors by layer), **F7** (pen widths), **F8** (plot manager / print preview / restore colors), **F9** (background image).
- **Numeric Keypad** (Num Lock on): The **primary GIN command engine**. Every mode exposes exactly **10 commands** (0–9 + Enter) displayed on the Tool Bar. **Enter** breaks sequences or completes; **Esc** cancels.
- **Snaps & Constraints** (Setup > Options > Constraints / User Axis):
  - Grid snap (default 1/8" 3D, toggle on/off).
  - Axis snap (influence angle, e.g., 45° forces ortho).
  - Object snaps (command-only): **Q** (engage nearest segment — blinks; sequential Q cycles ends), **A** (engage node/point), **B** (jump to center/midpoint/centroid of engaged). **Space** disengages. Text ignored; polygons treated as contiguous vectors. Engaged length/angle reported on Caption Bar.

### 3. Graphic Input (GIN) Mode Architecture — The Heart of the System
**<Alt>M** or MenuBar → Mode switches modes. Hotkeys (some shifted):  
**[** / **Home** = Draw, **\** / **End** = Annotate, **-** / **PageUp** = Trap, **]** / **PageDn** = Edit, **{** / **^Home** = 2Line/Double Line, **|** / **^End** = Dimension, **_** / **^PageUp** = Cut, **}** / **^PageDn** = Nodal, ***** = Fixup, **'** = new text, **"** = edit text, **:** = segment edit, **;** = primitive edit.  
**Tasks** shortcuts: **<Alt>1** (Rectangular Duct), **<Alt>2** (Power), **<Alt>3** (Pipe), **<Alt>4** (Round Duct).

Each mode has a **dedicated 10-command numeric keypad layout** (Tool Bar mirrors it). Trap buffer (sepia/orange highlighting) is **persistent** across modes and used for batch edits. Complex segments built by trapping primitives/segments then compressing (**Trap 7**).

**Key Mode Details** (numeric commands):

- **Draw** (**[**, default mode, + cross cursor): Core drafting.  
  **0** Options (pen, hatch pattern, arc/circle/curve types, scale, constraints).  
  **1** Marker (point style).  
  **2** Lines (rubber-band; **22** precision via TAB dim + diamond keys ESDX/RWZC).  
  **3** Polygons (close with Enter; hatch/solid/fill via Options).  
  **4** Quadrilaterals (auto-closed 4-line compressed).  
  **5** Arcs (3-point tangent).  
  **6** Curve/Spline (more points = tighter; coincident = corner; fitted option).  
  **7** Circle (center + radius).  
  **8** Ellipse (center + minor/major axes).  
  **9** Block insert (from block menu; create via Trap 8).  
  Rubber-banding + precision TAB everywhere.

- **Trap** (**-**, trap cursor variants for add/remove): Selection buffer.  
  **0** IN/OUT toggle, **1** Single, **2** Stitch (line + crossing), **4** Field, **5** Last (reverse order), **6** Engaged, **7** Compress, **8** TrapMenu (operate), **9** Modify.  
  Over-trap + filter (e.g., by pen). Status Bar shows trap count.

- **Edit** (**]**): Modify trapped elements. **0** Options, **1** Set Pivot, **2/3** Rotate CW/CCW, **4** Move, **5** Copy, **6** Mirror, **7** Reduce, **8** Enlarge.

- **Annotate** (**\**): Decoration. **0** Options, **1** Tag, **2** Leader, **3** Arrow, **4** Bubble, **5** Hook, **6** Underline, **7** Box, **8** Cut-in, **9** Construction line.

- **Cut** (**_**): Surgical removal. **0** Options, **1** Torch, **2** Slice, **4** Field, **7** Clip, **9** Divide.

- **Nodal** (**}**): Stretching/deformation. **0** IN/OUT, **1** Point, **2** Line select, **3** Area, **4** Move, **5** Copy, **6** Extrude, **9** Engaged.

- **Dimension** (**|**): Annotation dims. **0** Options, **1** Tic mark, **2** Leader, **3** Plain, **4** Arrowhead, **5** Witness lines, **6** Radius, **7** Diameter, **8** Arc, **9** Convert.

- **Double Line / 2Line** (**{**): Parallel drafting. **0** Options, **1** Join, **2** Wall, **3** Flup (turn), etc.

- **Fixup** (*****): Cleanup. **0** Options, **1** Reference, **2** Mend, **3** Chamfer, **4** Fillet, **5** Square, **6** Parallel (Level/Adjust/Trim).

- **Tasks** (specialized): Low-pressure duct, power symbols/circuits, pipe (plain/line/fittings/drop/rise/symbol), round duct, pick-and-drag (shrink/swell/duplicate/flip/turn).

**Text** (any mode): **'** new (start point + lines + Esc), **"** edit existing. Attributes via **T** or **N**/**<Alt>N**. Stroke (default Simplex.psf) or TrueType (**F2**). Line editor; clipboard supported. Vectorize text in segments before export (**Trap 8**).

**Pick & Drag**: **:** segments, **;** primitives (M for rubber-band mend).

### 4. Supporting Architecture
- **Blocks & Inserts**: Trap 8 creates; Draw 9 inserts. Persistent across sessions.
- **Home Points**: **H** set numbered; **G** go to (menu).
- **Programming/Integration**: DDE (Visual Basic / Foxpro) for external control; Read for legacy.
- **File Ops**: PEG (native, V1 backward-compatible), DWG import/export, recover, shadow generations for undo.
- **3D/Projection**: Diamond F/V for Z; cylindrical coords; Project a view; odometer auto-3D.
- **Customization & Persistence**: Profiles, constraints (snaps, axis influence), user axis, appearance, tasks. Registry for options; transparent overlays respect current scheme.

### 5. Overall Design Strengths (Roadmap Takeaways)
- **Modal + Numeric** — Every operation is one or two keystrokes in the right mode; Tool Bar always shows the map.
- **Transparent, Non-Intrusive Overlays** — Status/Tool/Odometer never obscure drawing.
- **Persistent Context** — Trap, engaged items, home points, render state carry forward.
- **Precision-First** — TAB dimension/angle + diamond keys + snaps for “distance then direction” input (no command-line typing).
- **Construction-Oriented Tasks** — Built-in duct/pipe/power/round-duct workflows with symbols.
- **Extensibility** — DDE, block/attribute support, bitmap backgrounds, TrueType fallback, import/export.

This architecture makes AeSys exceptionally efficient for repetitive precision drafting (HVAC, electrical, piping, architectural) while remaining lightweight and keyboard-centric. The mode + numeric-keypad + transparent-status pattern is the central organizing principle — everything else (snaps, text, blocks, menus) feeds into it.

The help file itself follows this roadmap: TOC → Welcome/Keystroke Index → Text/Constraints/Definitions/FAQ → Display elements → Keyboards → Detailed per-mode numeric command pages (with diagrams) → Menu/Setup details. All topics are hyperlinked internally.