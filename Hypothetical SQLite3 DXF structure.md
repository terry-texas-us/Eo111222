                     ┌──────────────────────────┐
                  │   ROOT DICTIONARY (0)     │
                  │   handle: '1'             │
                  └──────┬───────────────────┘
                         │ hard_owner (360)
                    ┌────┴────┐
                    │ACAD_GROUP│  ACAD_LAYOUT  ACAD_MLINESTYLE ...
                    │DICTIONARY│
                    └─────────┘
                         │
            ┌────────────┼────────────────┐
            ▼            ▼                ▼
     ┌──────────┐ ┌──────────┐    ┌──────────┐
     │  LAYOUT  │ │  LAYOUT  │    │ XRECORD  │
     │ "Model"  │ │ "Layout1"│    │  (XDATA) │
     └────┬─────┘ └────┬─────┘    └──────────┘
          │ soft_ptr     │ soft_ptr (340)
          ▼ (340)        ▼
  ┌───────────────┐ ┌───────────────┐
  │ BLOCK_RECORD  │ │ BLOCK_RECORD  │
  │*Model_Space   │ │*Paper_Space   │
  │  handle: 'A1' │ │  handle: 'A2' │
  └───┬───────────┘ └───────────────┘
      │ hard_owner (entities owned by block_record)
      ├──────────┬──────────┬──────────┐
      ▼          ▼          ▼          ▼
  ┌───────┐ ┌───────┐ ┌────────┐ ┌────────┐
  │ LINE  │ │ ARC   │ │ INSERT │ │ TEXT   │
  │handle │ │handle │ │handle  │ │handle  │
  │ 'B1'  │ │ 'B2'  │ │ 'B3'  │ │ 'B4'  │
  └───┬───┘ └───┬───┘ └──┬──┬─┘ └───┬───┘
      │         │        │  │        │
      │soft_ptr │s.p.    │  │hard_ptr│soft_ptr
      │(340)    │(340)   │  │(360)   │(340)
      ▼         ▼        │  ▼        ▼
  ┌───────┐ ┌───────┐   │ ┌────────┐┌───────────┐
  │ LAYER │ │ LAYER │   │ │ATTRIB  ││TEXT_STYLE  │
  │ "0"   │ │"Walls"│   │ │tag=NUM ││"Standard"  │
  └───────┘ └───────┘   │ └────────┘└───────────┘
                         │ soft_ptr (340)
                         ▼
                    ┌───────────────┐
                    │ BLOCK_RECORD  │
                    │ "RoomNumber"  │
                    └───┬───────────┘
                        │ owns
                   ┌────┴────┐
                   ▼         ▼
               ┌───────┐ ┌────────┐
               │ LINE  │ │ ATTDEF │
               │(rect) │ │tag=NUM │
               └───────┘ └────────┘


-- ═══════════════════════════════════════════════════════════
-- LAYER 1: CORE OBJECT REGISTRY
-- ═══════════════════════════════════════════════════════════

CREATE TABLE dxf_objects (
    handle          TEXT    PRIMARY KEY,           -- group 5 (hex string, e.g. 'A1')
    owner_handle    TEXT,                          -- group 330 (owner backpointer)
    entity_type     TEXT    NOT NULL,              -- 'LINE','LAYER','DICTIONARY', etc.
    section         TEXT    NOT NULL               -- 'HEADER','TABLES','BLOCKS','ENTITIES','OBJECTS'
                    CHECK (section IN ('HEADER','CLASSES','TABLES','BLOCKS','ENTITIES','OBJECTS')),
    subclass        TEXT,                          -- last AcDb subclass marker (code 100)
    is_graphical    INTEGER NOT NULL DEFAULT 0,    -- 1 = DXFGraphic, 0 = DXFObject
    FOREIGN KEY (owner_handle) REFERENCES dxf_objects(handle)
);

CREATE INDEX idx_obj_owner   ON dxf_objects(owner_handle);
CREATE INDEX idx_obj_type    ON dxf_objects(entity_type);
CREATE INDEX idx_obj_section ON dxf_objects(section);

-- ═══════════════════════════════════════════════════════════
-- LAYER 2: HANDLE CROSS-REFERENCE (main navigation index)
-- ═══════════════════════════════════════════════════════════

CREATE TABLE handle_references (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    source_handle   TEXT    NOT NULL,              -- object containing the reference
    target_handle   TEXT    NOT NULL,              -- referenced object
    group_code      INTEGER NOT NULL,              -- DXF group code (330,340..349,350..359,360..369,etc.)
    reference_type  TEXT    NOT NULL               -- classified type
                    CHECK (reference_type IN (
                        'soft_pointer',            -- 330(non-owner), 340-349
                        'hard_pointer',            -- 390-399, 480-481
                        'soft_owner',              -- 350-359
                        'hard_owner'               -- 360-369
                    )),
    context         TEXT,                          -- 'ACAD_REACTORS', 'ACAD_XDICTIONARY', NULL
    sequence        INTEGER NOT NULL DEFAULT 0,    -- ordering within source object
    FOREIGN KEY (source_handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (target_handle) REFERENCES dxf_objects(handle)
);

-- Primary navigation: "what does this object reference?"
CREATE INDEX idx_ref_source      ON handle_references(source_handle);
-- Reverse navigation: "what references this object?"
CREATE INDEX idx_ref_target      ON handle_references(target_handle);
-- Filter by relationship type
CREATE INDEX idx_ref_type        ON handle_references(reference_type);
-- Combined: "all hard owners of a target"
CREATE INDEX idx_ref_target_type ON handle_references(target_handle, reference_type);

-- ═══════════════════════════════════════════════════════════
-- LAYER 3: GROUP CODE EAV (round-trip fidelity)
-- ═══════════════════════════════════════════════════════════

CREATE TABLE group_codes (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_handle    TEXT    NOT NULL,
    group_code      INTEGER NOT NULL,
    value_text      TEXT,                          -- string value (all codes store text repr)
    value_real      REAL,                          -- populated for float codes (10-59, 110-149, etc.)
    value_int       INTEGER,                       -- populated for int codes (60-79, 90-99, 170-179, etc.)
    sequence        INTEGER NOT NULL,              -- preserves DXF file order within object
    FOREIGN KEY (owner_handle) REFERENCES dxf_objects(handle)
);

CREATE INDEX idx_gc_owner      ON group_codes(owner_handle);
CREATE INDEX idx_gc_code       ON group_codes(group_code);
CREATE INDEX idx_gc_owner_code ON group_codes(owner_handle, group_code);

-- ═══════════════════════════════════════════════════════════
-- LAYER 4: HEADER SECTION
-- ═══════════════════════════════════════════════════════════

CREATE TABLE header_variables (
    name            TEXT PRIMARY KEY,              -- '$ACADVER', '$HANDSEED', '$INSBASE', ...
    group_code      INTEGER NOT NULL,              -- primary value group code
    value_text      TEXT,
    value_real      REAL,
    value_int       INTEGER,
    -- Point-valued variables ($INSBASE, $EXTMIN, $EXTMAX, etc.)
    value_point_x   REAL,
    value_point_y   REAL,
    value_point_z   REAL
);

-- ═══════════════════════════════════════════════════════════
-- LAYER 5: CLASSES
-- ═══════════════════════════════════════════════════════════

CREATE TABLE classes (
    record_name     TEXT PRIMARY KEY,              -- DXF record name (code 1)
    cpp_class_name  TEXT NOT NULL,                 -- C++ class name (code 2)
    app_name        TEXT,                          -- application name (code 3)
    proxy_flags     INTEGER DEFAULT 0,             -- code 90
    instance_count  INTEGER DEFAULT 0,             -- code 91
    was_proxy       INTEGER DEFAULT 0,             -- code 280
    is_entity       INTEGER DEFAULT 0              -- code 281 (1=entity, 0=object)
);

-- ═══════════════════════════════════════════════════════════
-- LAYER 6: SYMBOL TABLES
-- ═══════════════════════════════════════════════════════════

-- The TABLE objects themselves are registered in dxf_objects with entity_type
-- = 'TABLE'. Each table entry references its parent table via owner_handle.

CREATE TABLE appids (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    flags           INTEGER DEFAULT 0,
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE block_records (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,           -- '*Model_Space', '*Paper_Space', user blocks
    flags           INTEGER DEFAULT 0,
    layout_handle   TEXT,                           -- hard pointer → layouts.handle
    insert_units    INTEGER DEFAULT 0,             -- code 70
    explodability   INTEGER DEFAULT 1,             -- code 280
    scalability     INTEGER DEFAULT 0,             -- code 281
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE dim_styles (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    flags           INTEGER DEFAULT 0,
    -- 70+ dimension variables — stored in group_codes EAV via owner_handle
    -- Common ones denormalized for query convenience:
    dimtxt_handle   TEXT,                          -- DIMTXSTY → text_styles.handle
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE layers (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    flags           INTEGER DEFAULT 0,             -- 1=frozen, 4=locked
    color           INTEGER DEFAULT 7,             -- negative = layer off
    linetype_handle TEXT,                          -- soft pointer → linetypes.handle
    lineweight      INTEGER DEFAULT -1,            -- -1=default, -3=BYLAYER
    plot_style_handle TEXT,                        -- → dxf_objects.handle
    material_handle TEXT,                          -- hard pointer → dxf_objects.handle
    is_plotted      INTEGER DEFAULT 1,             -- code 290
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (linetype_handle) REFERENCES linetypes(handle)
);

CREATE TABLE linetypes (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    flags           INTEGER DEFAULT 0,
    description     TEXT,
    alignment       INTEGER DEFAULT 65,            -- ASCII 'A', always
    pattern_length  REAL DEFAULT 0.0,              -- total pattern length
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE linetype_elements (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    linetype_handle TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    dash_length     REAL NOT NULL,                 -- positive=dash, negative=gap, 0=dot
    shape_number    INTEGER DEFAULT 0,             -- code 75
    style_handle    TEXT,                          -- → text_styles.handle (for shape/text elements)
    scale           REAL DEFAULT 1.0,
    rotation        REAL DEFAULT 0.0,              -- absolute or relative
    offset_x        REAL DEFAULT 0.0,
    offset_y        REAL DEFAULT 0.0,
    text            TEXT,                          -- embedded text string
    FOREIGN KEY (linetype_handle) REFERENCES linetypes(handle)
);

CREATE TABLE text_styles (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL UNIQUE,
    flags           INTEGER DEFAULT 0,             -- 4=vertical
    height          REAL DEFAULT 0.0,              -- 0=variable
    width_factor    REAL DEFAULT 1.0,
    oblique_angle   REAL DEFAULT 0.0,              -- degrees
    generation_flags INTEGER DEFAULT 0,            -- 2=backward, 4=upside-down
    last_height     REAL DEFAULT 0.2,
    font_file       TEXT,                          -- primary font (code 3)
    bigfont_file    TEXT,                          -- bigfont (code 4)
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE ucs_table (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL,
    flags           INTEGER DEFAULT 0,
    origin_x REAL, origin_y REAL, origin_z REAL,
    x_axis_x REAL, x_axis_y REAL, x_axis_z REAL,
    y_axis_x REAL, y_axis_y REAL, y_axis_z REAL,
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE views (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL,
    flags           INTEGER DEFAULT 0,
    height REAL, width REAL,
    center_x REAL, center_y REAL,
    direction_x REAL, direction_y REAL, direction_z REAL,
    target_x REAL, target_y REAL, target_z REAL,
    lens_length     REAL DEFAULT 50.0,
    front_clip REAL DEFAULT 0.0,
    back_clip  REAL DEFAULT 0.0,
    twist_angle     REAL DEFAULT 0.0,
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE vports (
    handle          TEXT PRIMARY KEY,
    name            TEXT NOT NULL,                 -- '*Active' for active viewport
    flags           INTEGER DEFAULT 0,
    lower_left_x REAL DEFAULT 0.0, lower_left_y REAL DEFAULT 0.0,
    upper_right_x REAL DEFAULT 1.0, upper_right_y REAL DEFAULT 1.0,
    center_x REAL DEFAULT 0.0, center_y REAL DEFAULT 0.0,
    snap_base_x REAL DEFAULT 0.0, snap_base_y REAL DEFAULT 0.0,
    snap_spacing_x REAL DEFAULT 0.5, snap_spacing_y REAL DEFAULT 0.5,
    grid_spacing_x REAL DEFAULT 10.0, grid_spacing_y REAL DEFAULT 10.0,
    view_dir_x REAL DEFAULT 0.0, view_dir_y REAL DEFAULT 0.0, view_dir_z REAL DEFAULT 1.0,
    view_target_x REAL DEFAULT 0.0, view_target_y REAL DEFAULT 0.0, view_target_z REAL DEFAULT 0.0,
    view_height     REAL DEFAULT 10.0,
    aspect_ratio    REAL DEFAULT 1.0,
    lens_length     REAL DEFAULT 50.0,
    front_clip REAL DEFAULT 0.0, back_clip REAL DEFAULT 0.0,
    twist_angle     REAL DEFAULT 0.0,
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

-- ═══════════════════════════════════════════════════════════
-- LAYER 7: BLOCKS SECTION
-- ═══════════════════════════════════════════════════════════

CREATE TABLE blocks (
    handle              TEXT PRIMARY KEY,
    block_record_handle TEXT NOT NULL,             -- hard owner → block_records.handle
    name                TEXT NOT NULL,
    flags               INTEGER DEFAULT 0,         -- 1=anonymous, 2=has_attdefs, 4=xref, etc.
    base_point_x REAL DEFAULT 0.0,
    base_point_y REAL DEFAULT 0.0,
    base_point_z REAL DEFAULT 0.0,
    xref_path           TEXT,                      -- code 1 (external reference path)
    description         TEXT,                      -- code 4
    end_block_handle    TEXT,                      -- handle of ENDBLK entity
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (block_record_handle) REFERENCES block_records(handle)
);

-- ═══════════════════════════════════════════════════════════
-- LAYER 8: ENTITY BASE (AcDbEntity common properties)
-- ═══════════════════════════════════════════════════════════

CREATE TABLE entities (
    handle              TEXT PRIMARY KEY,
    entity_type         TEXT NOT NULL,             -- 'LINE','ARC','TEXT','INSERT', etc.
    layer_handle        TEXT NOT NULL,             -- soft pointer → layers.handle
    linetype_handle     TEXT,                      -- soft pointer → linetypes.handle (NULL=BYLAYER)
    color               INTEGER DEFAULT 256,       -- 256=BYLAYER, 0=BYBLOCK, 1-255=ACI
    true_color          INTEGER,                   -- 24-bit RGB (code 420), NULL if ACI only
    transparency        INTEGER,                   -- code 440
    lineweight          INTEGER DEFAULT -1,        -- -1=BYLAYER, -2=BYBLOCK, -3=DEFAULT
    ltscale             REAL DEFAULT 1.0,          -- linetype scale
    paperspace          INTEGER DEFAULT 0,         -- 0=model, 1=paper
    invisible           INTEGER DEFAULT 0,         -- code 60
    shadow_mode         INTEGER,                   -- code 284
    -- OCS extrusion vector (default = WCS Z-axis)
    extrusion_x         REAL DEFAULT 0.0,
    extrusion_y         REAL DEFAULT 0.0,
    extrusion_z         REAL DEFAULT 1.0,
    -- Ownership within block structure
    block_record_handle TEXT NOT NULL,             -- which BLOCK_RECORD owns this entity
    sequence            INTEGER NOT NULL,          -- ordering within block (render order)
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (layer_handle) REFERENCES layers(handle),
    FOREIGN KEY (linetype_handle) REFERENCES linetypes(handle),
    FOREIGN KEY (block_record_handle) REFERENCES block_records(handle)
);

CREATE INDEX idx_ent_layer ON entities(layer_handle);
CREATE INDEX idx_ent_block ON entities(block_record_handle);
CREATE INDEX idx_ent_type  ON entities(entity_type);

-- ═══════════════════════════════════════════════════════════
-- LAYER 9: TYPED ENTITY EXTENSIONS
-- ═══════════════════════════════════════════════════════════

-- ── LINE ──
CREATE TABLE entity_line (
    handle    TEXT PRIMARY KEY,
    start_x REAL, start_y REAL, start_z REAL,      -- code 10/20/30 (OCS)
    end_x   REAL, end_y   REAL, end_z   REAL,      -- code 11/21/31 (OCS)
    thickness REAL DEFAULT 0.0,
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ── POINT ──
CREATE TABLE entity_point (
    handle    TEXT PRIMARY KEY,
    x REAL, y REAL, z REAL,
    thickness REAL DEFAULT 0.0,
    angle     REAL DEFAULT 0.0,                    -- code 50 (X-axis angle)
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ── CIRCLE ──
CREATE TABLE entity_circle (
    handle    TEXT PRIMARY KEY,
    center_x REAL, center_y REAL, center_z REAL,   -- OCS
    radius    REAL NOT NULL,
    thickness REAL DEFAULT 0.0,
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ── ARC ──
CREATE TABLE entity_arc (
    handle    TEXT PRIMARY KEY,
    center_x REAL, center_y REAL, center_z REAL,   -- OCS
    radius    REAL NOT NULL,
    start_angle REAL NOT NULL,                     -- degrees (DXF convention)
    end_angle   REAL NOT NULL,                     -- degrees
    thickness   REAL DEFAULT 0.0,
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ── ELLIPSE ──
CREATE TABLE entity_ellipse (
    handle    TEXT PRIMARY KEY,
    center_x REAL, center_y REAL, center_z REAL,   -- WCS (DXF spec!)
    major_x  REAL, major_y  REAL, major_z  REAL,   -- WCS endpoint of major axis
    ratio     REAL NOT NULL,                       -- minor/major (0 < ratio ≤ 1)
    start_param REAL DEFAULT 0.0,                  -- radians
    end_param   REAL DEFAULT 6.283185307179586,    -- 2π
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ── TEXT (single-line) ──
CREATE TABLE entity_text (
    handle            TEXT PRIMARY KEY,
    text_style_handle TEXT,                        -- soft pointer → text_styles.handle
    insert_x REAL, insert_y REAL, insert_z REAL,   -- first alignment point (10/20/30)
    align_x  REAL, align_y  REAL, align_z  REAL,   -- second alignment point (11/21/31)
    height            REAL NOT NULL,
    width_factor      REAL DEFAULT 1.0,
    rotation          REAL DEFAULT 0.0,            -- degrees (TEXT code 50 = degrees!)
    oblique_angle     REAL DEFAULT 0.0,            -- degrees
    generation_flags  INTEGER DEFAULT 0,           -- code 71 (2=backward, 4=upside-down)
    h_justify         INTEGER DEFAULT 0,           -- code 72 (0=left,1=center,2=right,3=aligned,4=middle,5=fit)
    v_justify         INTEGER DEFAULT 0,           -- code 73 (0=baseline,1=bottom,2=middle,3=top)
    content           TEXT NOT NULL,
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (text_style_handle) REFERENCES text_styles(handle)
);

-- ── MTEXT ──
CREATE TABLE entity_mtext (
    handle            TEXT PRIMARY KEY,
    text_style_handle TEXT,
    insert_x REAL, insert_y REAL, insert_z REAL,   -- insertion point
    height            REAL NOT NULL,               -- nominal text height
    reference_width   REAL,                        -- code 41 (reference rectangle width)
    attachment_point  INTEGER DEFAULT 1,           -- code 71 (1-9, TL/TC/TR/ML/MC/MR/BL/BC/BR)
    drawing_direction INTEGER DEFAULT 1,           -- code 72 (1=LR, 3=TB, 5=by_style)
    rotation          REAL DEFAULT 0.0,            -- RADIANS (MTEXT code 50 = radians!)
    line_spacing_style INTEGER DEFAULT 1,          -- code 73 (1=at_least, 2=exact)
    line_spacing_factor REAL DEFAULT 1.0,          -- code 44
    content           TEXT NOT NULL,               -- with \P, \S, etc. formatting codes
    -- Direction vector (code 11/21/31) — overrides rotation when present
    x_axis_x REAL, x_axis_y REAL, x_axis_z REAL,
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (text_style_handle) REFERENCES text_styles(handle)
);

-- ── INSERT ──
CREATE TABLE entity_insert (
    handle              TEXT PRIMARY KEY,
    block_record_handle TEXT NOT NULL,             -- which block definition to instantiate
    insert_x REAL, insert_y REAL, insert_z REAL,
    scale_x  REAL DEFAULT 1.0,
    scale_y  REAL DEFAULT 1.0,
    scale_z  REAL DEFAULT 1.0,
    rotation REAL DEFAULT 0.0,                     -- degrees
    column_count   INTEGER DEFAULT 1,
    row_count      INTEGER DEFAULT 1,
    column_spacing REAL DEFAULT 0.0,
    row_spacing    REAL DEFAULT 0.0,
    has_attribs    INTEGER DEFAULT 0,              -- code 66 (1=ATTRIB entities follow)
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (block_record_handle) REFERENCES block_records(handle)
);

-- ── ATTDEF (attribute template in block definition) ──
CREATE TABLE entity_attdef (
    handle            TEXT PRIMARY KEY,
    text_style_handle TEXT,
    insert_x REAL, insert_y REAL, insert_z REAL,
    align_x  REAL, align_y  REAL, align_z  REAL,
    height            REAL NOT NULL,
    tag               TEXT NOT NULL,               -- code 2 (attribute tag name)
    default_value     TEXT,                        -- code 1 (default text)
    prompt            TEXT,                        -- code 3 (prompt string)
    flags             INTEGER DEFAULT 0,           -- code 70 (1=invis,2=const,4=verify,8=preset)
    h_justify         INTEGER DEFAULT 0,
    v_justify         INTEGER DEFAULT 0,
    rotation          REAL DEFAULT 0.0,
    width_factor      REAL DEFAULT 1.0,
    oblique_angle     REAL DEFAULT 0.0,
    generation_flags  INTEGER DEFAULT 0,
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (text_style_handle) REFERENCES text_styles(handle)
);

-- ── ATTRIB (attribute instance on an INSERT) ──
CREATE TABLE entity_attrib (
    handle            TEXT PRIMARY KEY,
    text_style_handle TEXT,
    insert_handle     TEXT NOT NULL,               -- hard pointer → entity_insert.handle (PARENT)
    attdef_handle     TEXT,                        -- soft pointer → entity_attdef.handle (template)
    insert_x REAL, insert_y REAL, insert_z REAL,
    align_x  REAL, align_y  REAL, align_z  REAL,
    height            REAL NOT NULL,
    tag               TEXT NOT NULL,               -- code 2
    value             TEXT NOT NULL,               -- code 1 (the actual attribute value)
    flags             INTEGER DEFAULT 0,
    h_justify         INTEGER DEFAULT 0,
    v_justify         INTEGER DEFAULT 0,
    rotation          REAL DEFAULT 0.0,
    width_factor      REAL DEFAULT 1.0,
    oblique_angle     REAL DEFAULT 0.0,
    generation_flags  INTEGER DEFAULT 0,
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (insert_handle) REFERENCES entity_insert(handle),
    FOREIGN KEY (text_style_handle) REFERENCES text_styles(handle)
);

-- ── LWPOLYLINE ──
CREATE TABLE entity_lwpolyline (
    handle          TEXT PRIMARY KEY,
    flags           INTEGER DEFAULT 0,             -- 1=closed, 128=plinegen
    constant_width  REAL DEFAULT 0.0,
    elevation       REAL DEFAULT 0.0,
    thickness       REAL DEFAULT 0.0,
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

CREATE TABLE lwpolyline_vertices (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_handle    TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    x REAL NOT NULL, y REAL NOT NULL,              -- 2D vertices (OCS)
    bulge           REAL DEFAULT 0.0,
    start_width     REAL,                          -- NULL = use constant_width
    end_width       REAL,
    FOREIGN KEY (owner_handle) REFERENCES entity_lwpolyline(handle)
);

CREATE INDEX idx_lwpv_owner ON lwpolyline_vertices(owner_handle, sequence);

-- ── POLYLINE (2D/3D heavy polyline + VERTEX sub-entities) ──
CREATE TABLE entity_polyline (
    handle              TEXT PRIMARY KEY,
    flags               INTEGER DEFAULT 0,         -- code 70 (bitmask)
    default_start_width REAL DEFAULT 0.0,
    default_end_width   REAL DEFAULT 0.0,
    mesh_m INTEGER DEFAULT 0, mesh_n INTEGER DEFAULT 0,
    smooth_density_m INTEGER DEFAULT 0, smooth_density_n INTEGER DEFAULT 0,
    smooth_type         INTEGER DEFAULT 0,
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- VERTEX sub-entities are registered in dxf_objects + entities with
-- owner_handle pointing to the POLYLINE handle.
CREATE TABLE entity_vertex (
    handle    TEXT PRIMARY KEY,
    polyline_handle TEXT NOT NULL,
    sequence  INTEGER NOT NULL,
    x REAL, y REAL, z REAL,
    bulge     REAL DEFAULT 0.0,
    flags     INTEGER DEFAULT 0,                   -- code 70
    start_width REAL DEFAULT 0.0,
    end_width   REAL DEFAULT 0.0,
    curve_fit_tangent REAL,                        -- code 50
    -- Polygon mesh vertex indices (code 71-74)
    mesh_idx1 INTEGER, mesh_idx2 INTEGER,
    mesh_idx3 INTEGER, mesh_idx4 INTEGER,
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (polyline_handle) REFERENCES entity_polyline(handle)
);

-- ── SPLINE ──
CREATE TABLE entity_spline (
    handle          TEXT PRIMARY KEY,
    flags           INTEGER DEFAULT 0,             -- code 70 (1=closed,2=periodic,4=rational,8=planar,16=linear)
    degree          INTEGER NOT NULL DEFAULT 3,    -- code 71
    knot_tolerance  REAL DEFAULT 0.0000001,
    control_tolerance REAL DEFAULT 0.0000001,
    fit_tolerance   REAL DEFAULT 0.0000000001,
    start_tan_x REAL, start_tan_y REAL, start_tan_z REAL,  -- code 12/22/32
    end_tan_x   REAL, end_tan_y   REAL, end_tan_z   REAL,  -- code 13/23/33
    normal_x REAL, normal_y REAL, normal_z REAL,            -- code 210/220/230 (if planar)
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

CREATE TABLE spline_knots (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    spline_handle   TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    value           REAL NOT NULL,
    FOREIGN KEY (spline_handle) REFERENCES entity_spline(handle)
);

CREATE TABLE spline_control_points (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    spline_handle   TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    x REAL, y REAL, z REAL,
    weight          REAL DEFAULT 1.0,              -- NURBS weight (only meaningful if Rational flag)
    FOREIGN KEY (spline_handle) REFERENCES entity_spline(handle)
);

CREATE TABLE spline_fit_points (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    spline_handle   TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    x REAL, y REAL, z REAL,
    FOREIGN KEY (spline_handle) REFERENCES entity_spline(handle)
);

CREATE INDEX idx_spk_owner ON spline_knots(spline_handle, sequence);
CREATE INDEX idx_spcp_owner ON spline_control_points(spline_handle, sequence);
CREATE INDEX idx_spfp_owner ON spline_fit_points(spline_handle, sequence);

-- ── HATCH ──
CREATE TABLE entity_hatch (
    handle          TEXT PRIMARY KEY,
    pattern_name    TEXT,
    solid_fill      INTEGER DEFAULT 0,             -- code 70 (1=solid fill)
    associative     INTEGER DEFAULT 0,             -- code 71
    hatch_style     INTEGER DEFAULT 0,             -- code 75 (0=normal, 1=outer, 2=ignore)
    pattern_type    INTEGER DEFAULT 0,             -- code 76 (0=user, 1=predefined, 2=custom)
    pattern_angle   REAL DEFAULT 0.0,              -- code 52
    pattern_scale   REAL DEFAULT 1.0,              -- code 41
    pattern_double  INTEGER DEFAULT 0,             -- code 77
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

CREATE TABLE hatch_boundary_paths (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    hatch_handle    TEXT NOT NULL,
    path_type       INTEGER NOT NULL,              -- code 92 (bitmask: 1=external, 2=polyline, etc.)
    sequence        INTEGER NOT NULL,
    -- If polyline boundary (bit 1 of path_type):
    is_closed       INTEGER,
    has_bulge       INTEGER,
    FOREIGN KEY (hatch_handle) REFERENCES entity_hatch(handle)
);

CREATE TABLE hatch_boundary_edges (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    boundary_id     INTEGER NOT NULL,
    edge_type       INTEGER NOT NULL,              -- 1=line, 2=arc, 3=ellipse, 4=spline
    sequence        INTEGER NOT NULL,
    -- Line (type 1):
    start_x REAL, start_y REAL,
    end_x   REAL, end_y   REAL,
    -- Arc (type 2):
    center_x REAL, center_y REAL,
    radius   REAL,
    start_angle REAL, end_angle REAL,
    is_ccw   INTEGER,
    -- Ellipse (type 3):
    ecenter_x REAL, ecenter_y REAL,
    emajor_x  REAL, emajor_y  REAL,
    eratio     REAL,
    estart_angle REAL, eend_angle REAL,
    eis_ccw    INTEGER,
    -- Spline data lives in group_codes EAV (complex, variable-length)
    FOREIGN KEY (boundary_id) REFERENCES hatch_boundary_paths(id)
);

CREATE TABLE hatch_pattern_lines (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    hatch_handle    TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    angle           REAL,
    base_x REAL, base_y REAL,
    offset_x REAL, offset_y REAL,
    FOREIGN KEY (hatch_handle) REFERENCES entity_hatch(handle)
);

CREATE TABLE hatch_pattern_dashes (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    pattern_line_id INTEGER NOT NULL,
    sequence        INTEGER NOT NULL,
    length          REAL NOT NULL,
    FOREIGN KEY (pattern_line_id) REFERENCES hatch_pattern_lines(id)
);

-- ── DIMENSION (base) ──
CREATE TABLE entity_dimension (
    handle              TEXT PRIMARY KEY,
    dim_style_handle    TEXT,                      -- → dim_styles.handle
    block_record_handle TEXT,                      -- anonymous block for rendered geometry
    dim_type            INTEGER DEFAULT 0,         -- code 70 (bitmask)
    attachment_point    INTEGER DEFAULT 5,          -- code 71
    -- Definition point (code 10/20/30) — varies by dim type
    def_point_x REAL, def_point_y REAL, def_point_z REAL,
    -- Middle of text (code 11/21/31)
    text_mid_x REAL, text_mid_y REAL, text_mid_z REAL,
    text_override       TEXT,                      -- code 1 (empty = measured)
    text_rotation       REAL DEFAULT 0.0,          -- code 53
    -- Type-specific points stored in group_codes EAV (13/14/15/16 series)
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (dim_style_handle) REFERENCES dim_styles(handle),
    FOREIGN KEY (block_record_handle) REFERENCES block_records(handle)
);

-- ── SOLID / 3DFACE / TRACE ──
CREATE TABLE entity_solid (
    handle    TEXT PRIMARY KEY,
    corner1_x REAL, corner1_y REAL, corner1_z REAL,
    corner2_x REAL, corner2_y REAL, corner2_z REAL,
    corner3_x REAL, corner3_y REAL, corner3_z REAL,
    corner4_x REAL, corner4_y REAL, corner4_z REAL,
    thickness REAL DEFAULT 0.0,
    -- For 3DFACE: invisible edge flags (code 70)
    edge_flags INTEGER DEFAULT 0,
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ── LEADER ──
CREATE TABLE entity_leader (
    handle            TEXT PRIMARY KEY,
    dim_style_handle  TEXT,
    has_arrowhead     INTEGER DEFAULT 1,
    path_type         INTEGER DEFAULT 0,           -- 0=straight, 1=spline
    annotation_type   INTEGER DEFAULT 3,           -- 0=mtext, 1=tolerance, 2=insert, 3=none
    annotation_handle TEXT,                        -- → dxf_objects.handle (mtext/tolerance/insert)
    hook_line_direction INTEGER DEFAULT 1,
    FOREIGN KEY (handle) REFERENCES entities(handle),
    FOREIGN KEY (dim_style_handle) REFERENCES dim_styles(handle)
);

CREATE TABLE leader_vertices (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    leader_handle   TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    x REAL, y REAL, z REAL,
    FOREIGN KEY (leader_handle) REFERENCES entity_leader(handle)
);

-- ── VIEWPORT (paper space viewport entity, distinct from VPORT table) ──
CREATE TABLE entity_viewport (
    handle    TEXT PRIMARY KEY,
    center_x REAL, center_y REAL, center_z REAL,
    width     REAL, height REAL,
    viewport_id INTEGER,
    view_dir_x REAL, view_dir_y REAL, view_dir_z REAL,
    view_target_x REAL, view_target_y REAL, view_target_z REAL,
    view_height   REAL,
    lens_length   REAL DEFAULT 50.0,
    front_clip REAL, back_clip REAL,
    snap_angle REAL, twist_angle REAL,
    status     INTEGER DEFAULT 0,
    -- Frozen layer list stored in handle_references (code 331)
    FOREIGN KEY (handle) REFERENCES entities(handle)
);

-- ═══════════════════════════════════════════════════════════
-- LAYER 10: NON-GRAPHICAL OBJECTS
-- ═══════════════════════════════════════════════════════════

CREATE TABLE dictionaries (
    handle          TEXT PRIMARY KEY,
    hard_owned      INTEGER DEFAULT 0,             -- code 280 (0=soft, 1=hard)
    cloning_flag    INTEGER DEFAULT 0,             -- code 281
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE dictionary_entries (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    dictionary_handle   TEXT NOT NULL,
    key_name            TEXT NOT NULL,              -- code 3
    value_handle        TEXT NOT NULL,              -- code 350 (soft) or 360 (hard)
    sequence            INTEGER NOT NULL,
    FOREIGN KEY (dictionary_handle) REFERENCES dictionaries(handle),
    FOREIGN KEY (value_handle) REFERENCES dxf_objects(handle)
);

CREATE INDEX idx_dict_entry    ON dictionary_entries(dictionary_handle);
CREATE INDEX idx_dict_key      ON dictionary_entries(key_name);
CREATE INDEX idx_dict_value    ON dictionary_entries(value_handle);

CREATE TABLE xrecords (
    handle          TEXT PRIMARY KEY,
    cloning_flag    INTEGER DEFAULT 0,             -- code 280
    -- XRecord data (arbitrary group code/value pairs) stored in group_codes table
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE layouts (
    handle              TEXT PRIMARY KEY,
    name                TEXT NOT NULL,
    flags               INTEGER DEFAULT 0,
    tab_order           INTEGER DEFAULT 0,
    block_record_handle TEXT,                      -- → block_records.handle
    -- Extensive paper/plot settings stored in group_codes EAV
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (block_record_handle) REFERENCES block_records(handle)
);

CREATE TABLE materials (
    handle TEXT PRIMARY KEY,
    name   TEXT NOT NULL,
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

CREATE TABLE mline_styles (
    handle TEXT PRIMARY KEY,
    name   TEXT NOT NULL,
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

-- PLOTSETTINGS (base for LAYOUT)
CREATE TABLE plot_settings (
    handle          TEXT PRIMARY KEY,
    page_setup_name TEXT,
    plot_config     TEXT,                          -- printer/plotter name
    paper_size      TEXT,
    -- Margins, scale, offset — stored in group_codes EAV for full fidelity
    FOREIGN KEY (handle) REFERENCES dxf_objects(handle)
);

-- ═══════════════════════════════════════════════════════════
-- LAYER 11: EXTENSION DICTIONARIES, REACTORS, XDATA
-- ═══════════════════════════════════════════════════════════

-- Extension dictionary ownership
-- (Also tracked in handle_references with context='ACAD_XDICTIONARY')
CREATE TABLE extension_dictionaries (
    owner_handle        TEXT PRIMARY KEY,           -- object that owns the xdict
    dictionary_handle   TEXT NOT NULL,              -- hard owner (code 360)
    FOREIGN KEY (owner_handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (dictionary_handle) REFERENCES dictionaries(handle)
);

-- Reactor list per object
-- (Also tracked in handle_references with context='ACAD_REACTORS')
CREATE TABLE reactors (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_handle    TEXT NOT NULL,
    reactor_handle  TEXT NOT NULL,
    sequence        INTEGER NOT NULL,
    FOREIGN KEY (owner_handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (reactor_handle) REFERENCES dxf_objects(handle)
);

CREATE INDEX idx_reactor_owner ON reactors(owner_handle);

-- Extended data (XDATA) per object per registered application
CREATE TABLE xdata (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_handle    TEXT NOT NULL,
    appid_handle    TEXT NOT NULL,                  -- → appids.handle
    group_code      INTEGER NOT NULL,              -- 1000-1071 range
    value_text      TEXT,
    value_real      REAL,
    value_int       INTEGER,
    sequence        INTEGER NOT NULL,
    FOREIGN KEY (owner_handle) REFERENCES dxf_objects(handle),
    FOREIGN KEY (appid_handle) REFERENCES appids(handle)
);

CREATE INDEX idx_xdata_owner ON xdata(owner_handle);
CREATE INDEX idx_xdata_app   ON xdata(appid_handle);

-- ═══════════════════════════════════════════════════════════
-- LAYER 12: VIEWS (convenience queries)
-- ═══════════════════════════════════════════════════════════

-- Forward navigation: "what does this object reference?"
CREATE VIEW v_outgoing_references AS
SELECT
    r.source_handle,
    src.entity_type AS source_type,
    r.target_handle,
    tgt.entity_type AS target_type,
    r.group_code,
    r.reference_type,
    r.context
FROM handle_references r
JOIN dxf_objects src ON src.handle = r.source_handle
JOIN dxf_objects tgt ON tgt.handle = r.target_handle;

-- Reverse navigation: "what references this object?"
CREATE VIEW v_incoming_references AS
SELECT
    r.target_handle,
    tgt.entity_type AS target_type,
    r.source_handle,
    src.entity_type AS source_type,
    r.group_code,
    r.reference_type,
    r.context
FROM handle_references r
JOIN dxf_objects src ON src.handle = r.source_handle
JOIN dxf_objects tgt ON tgt.handle = r.target_handle;

-- Entity with resolved layer and linetype names
CREATE VIEW v_entity_properties AS
SELECT
    e.handle,
    e.entity_type,
    l.name  AS layer_name,
    lt.name AS linetype_name,
    e.color,
    e.true_color,
    e.lineweight,
    e.ltscale,
    br.name AS owner_block_name
FROM entities e
JOIN layers l ON l.handle = e.layer_handle
LEFT JOIN linetypes lt ON lt.handle = e.linetype_handle
JOIN block_records br ON br.handle = e.block_record_handle;

-- INSERT with its ATTRIBs resolved
CREATE VIEW v_inserts_with_attribs AS
SELECT
    i.handle        AS insert_handle,
    br.name         AS block_name,
    a.handle        AS attrib_handle,
    a.tag           AS attrib_tag,
    a.value         AS attrib_value,
    ad.prompt       AS attdef_prompt,
    ad.default_value AS attdef_default
FROM entity_insert i
JOIN block_records br ON br.handle = i.block_record_handle
LEFT JOIN entity_attrib a ON a.insert_handle = i.handle
LEFT JOIN entity_attdef ad ON ad.handle = a.attdef_handle;

-- Full ownership tree (recursive)
CREATE VIEW v_ownership_tree AS
WITH RECURSIVE tree AS (
    SELECT handle, owner_handle, entity_type, section, 0 AS depth
    FROM dxf_objects
    WHERE owner_handle IS NULL OR owner_handle = '0'
    UNION ALL
    SELECT o.handle, o.owner_handle, o.entity_type, o.section, t.depth + 1
    FROM dxf_objects o
    JOIN tree t ON o.owner_handle = t.handle
    WHERE t.depth < 20  -- safety limit
)
SELECT * FROM tree ORDER BY depth, handle;

-- Model space entities only
CREATE VIEW v_model_space_entities AS
SELECT e.*
FROM entities e
JOIN block_records br ON br.handle = e.block_record_handle
WHERE br.name = '*Model_Space'
ORDER BY e.sequence;

-- Paper space entities only
CREATE VIEW v_paper_space_entities AS
SELECT e.*
FROM entities e
JOIN block_records br ON br.handle = e.block_record_handle
WHERE br.name = '*Paper_Space'
ORDER BY e.sequence;

-- Dictionary tree (named object dictionary and children)
CREATE VIEW v_dictionary_tree AS
WITH RECURSIVE dtree AS (
    SELECT de.dictionary_handle, de.key_name, de.value_handle,
           o.entity_type AS value_type, 0 AS depth
    FROM dictionary_entries de
    JOIN dxf_objects o ON o.handle = de.value_handle
    WHERE de.dictionary_handle = (
        SELECT handle FROM dxf_objects
        WHERE entity_type = 'DICTIONARY' AND owner_handle = '0'
        LIMIT 1
    )
    UNION ALL
    SELECT de2.dictionary_handle, de2.key_name, de2.value_handle,
           o2.entity_type, dt.depth + 1
    FROM dictionary_entries de2
    JOIN dtree dt ON de2.dictionary_handle = dt.value_handle
    JOIN dxf_objects o2 ON o2.handle = de2.value_handle
    WHERE dt.value_type = 'DICTIONARY' AND dt.depth < 10
)
SELECT * FROM dtree ORDER BY depth, dictionary_handle, key_name;