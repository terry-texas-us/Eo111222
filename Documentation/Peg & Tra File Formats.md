### Tracing File Format (production)
    EoDb::kHeaderSection sentinel          uint16_t 0x0101
    {0 or more key-value pairs}
    EoDb::kEndOfSection sentinel           uint16_t 0x01ff
 
    EoDb::kGroupsSection sentinel          uint16_t 0x0104
    Number of groups definitions           uint16_t
    {0 or more groups definitions}
    EoDb::kEndOfSection sentinel           uint16_t 0x01ff

### Notes: 
- When a tracing (tra) file is loaded the required Linetype Table can no longer
  be assumed to contain the required definitions. More than likely the only
  linetype preloaded will be the default “CONTINUOUS” linetype (index = 1).
  The primitives must be reviewed before the first display to determine which (if any)
  linetype should be loaded from the external linetype file. The Legacy linetype file
  will always be maintained in the original indexed order. So, a lookup of the linetype
  by the linetype index value in the tracing file primitive definition is straight forward.
---
### Peg File Formats (production)
  EoDb::kHeaderSection sentinel            uint16_t 0x0101
      
      {0 or more key-value pairs}
  
  EoDb::kEndOfSection sentinel             uint16_t 0x01ff

  EoDb::kTablesSection sentinel            uint16_t 0x0102
      
      EoDb::kViewPortTable sentinel        uint16_t 0x0201
          {0 or more viewport definitions} uint16_t
      EoDb::kEndOfTable sentinel           uint16_t 0x02ff

      EoDb::kLinetypeTable sentinel        uint16_t 0x0202
          Number of linetype               uint16_t
          {0 or more linetype definitions}
      EoDb::kEndOfTable sentinel           uint16_t 0x02ff

      EoDb::kLayerTable sentinel           uint16_t 0x0203
          Number of layers (resident only) uint16_t
          {0 or more layer definitions}
      EoDb::kEndOfTable sentinel           uint16_t 0x02ff

      (AE2026 V2 only — optional, peek-ahead for backward compatibility)
      EoDb::kTextStyleTable sentinel       uint16_t 0x0204
          Number of text styles            uint16_t
          {0 or more text style definitions}
      EoDb::kEndOfTable sentinel           uint16_t 0x02ff

      (AE2026 V2 only — optional, peek-ahead for backward compatibility)
      EoDb::kLayoutTable sentinel          uint16_t 0x0205
          Number of layouts                uint16_t
          {0 or more layout definitions}
      EoDb::kEndOfTable sentinel           uint16_t 0x02ff

  EoDb::kEndOfSection sentinel             uint16_t 0x01ff

  EoDb::kBlocksSection sentinel            uint16_t 0x0103

      Number of block definitions          uint16_t
      {0 or more block definitions}
  
  EoDb::kEndOfSection sentinel             uint16_t 0x01ff

  EoDb::kGroupsSection sentinel            uint16_t 0x0104
      
      Number of groups definitions         uint16_t
      {0 or more groups definitions}
  
  EoDb::kEndOfSection sentinel             uint16_t 0x01ff

### Layer definition
  
    Layers
      Name                                  string
      Tracing flags                         uint16_t
      State                                 uint16_t
      Layer pen color                       uint16_t
      Layer line type                       string
   
    Tracings
      ?? nothing

### Line style definition
  
    Name                                    string
    Flags (always 0)                        uint16_t
    Description                             string
    Definition length                       uint16_t
    Pattern length                          double
    if (definition length > 0)
    1 or more dash length                   double...

### Block definition
  
    Number of primitives                    uint16_t
    Name                                    string
    Flags                                   uint16_t
    Base point                              point3d
    {0 or more primitive definitions}

### Segments definition
  
    Number of Segment definition            uint16_t
    {0 or more segment definitions}

### Segment definition
  
    Number of primitive definitions         uint16_t
    {0 or more primitive definitions}
 
### Primitive definition
#### Point primitive (The Pen style in base Prim is unused)
      Type code <0x0100>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Point style            uint16_t  [4-5] (Point style follows PDMODE in AutoCAD, 0 is dot, 1 is not visible, 2 is plus, 3 is cross, and 4 = vertical bar)
      Point                  point3d   [6-13][14-21][22-29]
      Number of data values  uint16_t  [30-31]
      {0 or more data values}double    []

#### Insert primitive
      Type code <0x0101>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Block name             string
      Insertion point        point3d
      Local reference x-axis vector3d
      Local reference y-axis vector3d
      Local reference z-axis vector3d
      Number of columns      uint16_t
      Number of rows         uint16_t
      Column spacing         double
      Row spacing            double

#### BlockReference primitive
      Type code <0x0102>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Block name             string
      Insertion point        point3d
      Local normal vector    vector3d
      Scale factors(x, y, z) vector3d
      Rotation               double
      Number of columns      uint16_t
      Number of rows         uint16_t
      Column spacing         double
      Row spacing            double

      AE2026 V2 per-primitive extension (written/read via WriteV2Extension/ReadV2Extension,
      after the generic V2 block of handle, ownerHandle, lineWeight, lineTypeScale):
      Attribute handle count uint16_t  (number of owned ATTRIB primitives)
      {attribute handles}    uint64_t[] (handles of EoDbAttrib primitives linked to this INSERT)

#### Line primitive
      Type code <0x0200>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Line begin             point3d   
      Line end               point3d   

#### Polygon primitive
      Type code <0x0400>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Polygon style          uint16_t  [4-5]
      Polygon Style Index    uint16_t  [6-7]
      Number of vertices     uint16_t  [8-9]
      Hatch origin           point3d   
      Hatch/pattern 
         reference x-axis    vector3d  
      Hatch/pattern 
         reference y-axis    vector3d  
      {0 or more points}     point3d   [46-]

#### Ellipse primitive
      Type code <0x1003>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Center point           point3d   
      Major axis             vector3d  
      Minor axis             vector3d  
      Sweep angle            double    

#### Spline primitive
      Type code <0x2000>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Number of control 
         points              uint16_t  [6-7]
      {0 or more control
         points}             point3d   

#### CSpline primitive (This primitive may still exist in some files and is readable but is converted on the read to a Polyline primitive and is never written to file.
      Type code <0x2001>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      m_wPtsS                uint16_t  [6-7]
      Number of control 
         points              uint16_t  [8-9]
      End condition Type     uint16_t  [10-11]
      Begin point tangent
         vector              vector3d  
      End point tangent
         vector              vector3d  
      {0 or more control 
      points}                point3d   

#### Polyline primitive
Generalized polyline supporting straight segments, bulge arcs, and per-vertex widths.
Handles both DXF LWPOLYLINE and 2D/3D POLYLINE entities.

      Type code <0x2002>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Flags                  uint16_t  [6-7]
         bit 0 (0x0001): Closed — last vertex connects back to first
         bit 1 (0x0002): HasBulge — bulge array follows vertices
         bit 2 (0x0004): HasWidth — start/end width arrays follow
         bit 3 (0x0008): Plinegen — generate linetype pattern across vertices
      Number of vertices     uint16_t  [8-9]
      {vertices}             point3d[]
      if (Flags & HasBulge):
        {bulge values}       double[]  (one per vertex, tan(θ/4))
      if (Flags & HasWidth):
        {start widths}       double[]  (one per vertex)
        {end widths}         double[]  (one per vertex)

#### Text primitive
      Type code <0x4000>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Font definition
      Text precision         uint16_t  [6-7]
      Text font name         string
      Text path              uint16_t
      Horizontal alignment   uint16_t
      Vertical alignment     uint16_t
      Character spacing      double
      Insertion point        point3d
      Local reference x-axis vector3d
      Local reference y-axis vector3d
      Text ('\t' terminated) string

#### Attrib primitive (AE2026 V2 only — written as kTextPrimitive in AE2011 V1 with attribute identity lost)
      Type code <0x4001>     uint16_t  [0-1]
      Pen color              int16_t
      Line type              int16_t
      Font definition        (same layout as Text primitive)
      Reference system       (same layout as Text primitive)
      Text ('\t' terminated) string    (attribute value)
      Tag  ('\t' terminated) string    (attribute tag name, DXF group code 2)
      Attribute flags        int16_t   (DXF group code 70: 1=Invisible, 2=Constant, 4=Verify, 8=Preset)
      Insert handle          uint64_t  (handle of parent INSERT / EoDbBlockReference)

      Note: The V2 extension block (handle, ownerHandle, lineWeight, lineTypeScale) written
      by EoDbGroup::Write() follows after this primitive data, same as all other V2 primitives.

#### Tag primitive (This primitive may still exist in some files and is readable but is converted on the read to a Point primitive and is never written to file.)
      Type code <0x4100>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Point style            uint16_t  [4-5]
      Point                  point3d   

#### Dim primitive
      Type code <0x4200>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Line begin             point3d   
      Line end               point3d   

      Text pen color         uint16_t  [30-31]
      Font definition
      Text precision         uint16_t  [32-33]
      Text font name         string    
      Text path              uint16_t
      Horizontal alignment   uint16_t
      Vertical alignment     uint16_t
      Character spacing      double
      Insertion point        point3d
      Local reference x-axis vector3d
      Local reference y-axis vector3d
      Text ('\t' terminated) string

### Layout definition (AE2026 V2 only)
Persists DXF LAYOUT objects from the OBJECTS section for lossless DXF → PEG V2 → DXF round-trip.

      Handle                   uint64_t
      Owner handle             uint64_t
      Extension dictionary handle uint64_t
      Reactor handle count     uint16_t
      {reactor handles}        uint64_t[]

      -- AcDbPlotSettings --
      Page setup name          string
      Plot config name         string
      Paper size name          string
      Plot view name           string
      Current style sheet      string
      Left margin              double    (mm)
      Bottom margin            double    (mm)
      Right margin             double    (mm)
      Top margin               double    (mm)
      Paper width              double    (mm)
      Paper height             double    (mm)
      Plot origin X            double    (mm)
      Plot origin Y            double    (mm)
      Plot window LL X         double
      Plot window LL Y         double
      Plot window UR X         double
      Plot window UR Y         double
      Custom scale numerator   double
      Custom scale denominator double
      Plot layout flags        int16_t   (bitfield)
      Plot paper units         int16_t   (0=in, 1=mm, 2=px)
      Plot rotation            int16_t   (0=none, 1=90°CCW, 2=180°, 3=90°CW)
      Plot type                int16_t   (0=last, 1=extents, 2=limits, 3=view, 4=window, 5=layout)
      Standard scale type      int16_t
      Shade plot mode          int16_t
      Shade plot res level     int16_t
      Shade plot custom DPI    int16_t
      Scale factor             double
      Paper image origin X     double
      Paper image origin Y     double

      -- AcDbLayout --
      Layout name              string    (e.g. "Model", "Layout1")
      Layout flags             int16_t
      Tab order                int16_t   (0 = Model)
      Limits min X             double
      Limits min Y             double
      Limits max X             double
      Limits max Y             double
      Insert base X            double
      Insert base Y            double
      Insert base Z            double
      Extents min X            double
      Extents min Y            double
      Extents min Z            double
      Extents max X            double
      Extents max Y            double
      Extents max Z            double
      Elevation                double
      UCS origin X             double
      UCS origin Y             double
      UCS origin Z             double
      UCS X-axis X             double
      UCS X-axis Y             double
      UCS X-axis Z             double
      UCS Y-axis X             double
      UCS Y-axis Y             double
      UCS Y-axis Z             double
      UCS ortho type           int16_t
      Block record handle      uint64_t
      Last active viewport handle uint64_t