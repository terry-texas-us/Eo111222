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
  linetype preloaded will be the default “Continuous” linetype (index = 1).
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
      Point                  point3d   [6-9][10-13][14-17]
      Number of data values  uint16_t  [18-19]
      {0 or more data values}double    [20-]

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

#### Line primitive
      Type code <0x0200>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Line begin             point3d   [6-9][10-13][14-17]
      Line end               point3d   [18-21][22-25][26-29]

#### Polygon primitive
      Type code <0x0400>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Polygon style          uint16_t  [4-5]
      Polygon Style Index    uint16_t  [6-7]
      Number of vertices     uint16_t  [8-9]
      Hatch origin           point3d   [10-13][14-17][18-21]
      Hatch/pattern 
         reference x-axis    vector3d  [22-25][26-29][30-33]
      Hatch/pattern 
         reference y-axis    vector3d  [34-37][38-41][42-45]
      {0 or more points}     point3d   [46-]

#### Ellipse primitive
      Type code <0x1003>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Center point           point3d   [6-9][10-13][14-17]
      Major axis             vector3d  [18-21][22-25][26-29]
      Minor axis             vector3d  [30-33][34-37][38-41]
      Sweep angle            double    [42-45]

#### Spline primitive
      Type code <0x2000>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Number of control 
         points              uint16_t  [6-7]
      {0 or more control
         points}             point3d   [8-]

#### CSpline primitive (This primitive may still exist in some files and is readable but is converted on the read to a Polyline primitive and is never written to file.
      Type code <0x2001>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      m_wPtsS                uint16_t  [6-7]
      Number of control 
         points              uint16_t  [8-9]
      End condition Type     uint16_t  [10-11]
      Begin point tangent
         vector              vector3d  [12-15][16-19][20-23]
      End point tangent
         vector              vector3d  [24-27][28-31][32-35]
      {0 or more control 
      points}                point3d   [36-]

#### Polyline primitive (never made it release: if already written flags not streamed)
      Type code <0x2002>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Flags                  uint16_t  [6-7]
      Number of points       uint16_t  [8-9]
      {0 or more points}     point3d   [10-]

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

#### Tag primitive (This primitive may still exist in some files and is readable but is converted on the read to a Point primitive and is never written to file.)
      Type code <0x4100>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Point style            uint16_t  [4-5]
      Point                  point3d   [6-9][10-13][14-17]

#### Dim primitive
      Type code <0x4200>     uint16_t  [0-1]
      Pen color              uint16_t  [2-3]
      Line type              uint16_t  [4-5]
      Line begin             point3d   [6-9][10-13][14-17]
      Line end               point3d   [18-21][22-25][26-29]

      Text pen color         uint16_t  [30-31]
      Font definition
      Text precision         uint16_t  [32-33]
      Text font name         string    [34
      Text path              uint16_t
      Horizontal alignment   uint16_t
      Vertical alignment     uint16_t
      Character spacing      double
      Insertion point        point3d
      Local reference x-axis vector3d
      Local reference y-axis vector3d
      Text ('\t' terminated) string