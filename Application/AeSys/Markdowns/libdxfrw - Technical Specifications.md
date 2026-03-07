# libdxfrw - Technical Specifications

## Project Overview

**Project Name**: libdxfrw
**Version**: 0.6.3
**License**: GNU General Public License v2 (or later)
**Original Author**: José F. Soriano (Rallaz)
**Maintainers**: codelibs, Nicu Tofan, Miguel E. Hernández Cuervo

### Purpose

libdxfrw is a free C++ library for reading and writing DXF (Drawing eXchange Format) files in both ASCII and binary formats. It also supports reading DWG (AutoCAD drawing) files from versions R14 through V2015.

### Main Features

1. **DXF File Read/Write**
   - Read and write DXF files in ASCII format
   - Read and write DXF files in binary format
   - Support for multiple DXF versions: R12, 2000, 2004, 2007, 2010

2. **Supported CAD Drawing Entities**
   - Basic shapes: Points, Lines, Rays, XLines
   - Curves: Circles, Arcs, Ellipses, Splines
   - Polylines: 2D/3D Lightweight Polylines
   - 3D shapes: 3D Faces, Traces, Solids
   - Text: Single-line text, Multi-line text
   - Annotations: Hatches, Dimensions, Leaders
   - Others: Block references, Images, Viewports

3. **Supported CAD Objects**
   - Layers
   - Line Types
   - Text Styles
   - Dimension Styles
   - Viewports
   - Application IDs
   - Block Definitions

---

## Architecture

### Core Class Structure

```
dxfRW (Main API Class)
├── EoDxfInterface (Abstract Interface)
├── EoDxfReader/EoDxfWriter (DXF I/O Handlers)
├── EoDxfHeader (Header Variables)
├── EoDxfEntity (Geometric Entities)
│   ├── EoDxfPoint
│   ├── EoDxfLine
│   ├── EoDxfArc
│   ├── EoDxfCircle
│   ├── EoDxfEllipse
│   ├── EoDxfLwPolyline
│   ├── EoDxfPolyline
│   ├── EoDxfSpline
│   ├── EoDxfText
│   ├── EoDxfMText
│   ├── EoDxfHatch
│   ├── EoDxfDimension (various dimension types)
│   └── Other entities
└── DRW_Object (Non-geometric Objects)
    ├── EoDxfLayer
    ├── EoDxfLinetype
    ├── EoDxfTextStyle
    ├── EoDxfDimensionStyle
    └── Other objects
```

### Directory Structure

```
libdxfrw/
├── src/                      # Library source code
│   ├── EoDxfLib.h/cpp        # Main API class
│   ├── EoDxfInterface.h      # Abstract interface
│   ├── EoDxfBase.h           # Basic data structures
│   ├── EoDxfHeader.h/cpp     # Header variables
│   ├── EoDxfEntities.h/cpp   # Entity definitions
│   ├── EoDxfObjects.h/cpp    # Object definitions
│   └── intern/              # Internal implementation
│       ├── EoDxfReader.h/cpp     # DXF reader
│       ├── EoDxfWriter.h/cpp     # DXF writer
│       ├── EoDxfTextCodec.h/cpp # Character encoding
│       └── Other utilities
├── bin/                      # Executable scripts
```

---

## API Specifications

### Main Class: dxfRW

#### Constructor

```cpp
dxfRW(const char* name);
```

- **Parameters**: `name` - File name to process
- **Description**: Creates an instance for processing DXF/DWG files

#### Main Methods

##### Reading

```cpp
bool read(EoDxfInterface* interface_, bool ext);
```

- **Parameters**:
  - `interface_`: Callback interface implementation
  - `ext`: Apply extrusion to convert to 2D
- **Returns**: true on success, false on failure
- **Description**: Reads the file and calls interface methods for each entity

##### Writing

```cpp
bool write(EoDxfInterface* interface_, EoDxf::Version ver, bool bin);
```

- **Parameters**:
  - `interface_`: Interface implementation providing data
  - `ver`: DXF version (R14, 2000, 2004, 2007, 2010)
  - `bin`: Output in binary format
- **Returns**: true on success, false on failure

##### Entity Writing Methods

```cpp
bool writePoint(EoDxfPoint* point);
bool writeLine(EoDxfLine* line);
bool writeCircle(EoDxfCircle* circle);
bool writeArc(EoDxfArc* arc);
bool writeEllipse(EoDxfEllipse *ellipse);
bool writeLwPolyline(EoDxfLwPolyline* polyline);
bool writeSpline(EoDxfSpline* spline);
bool writeText(EoDxfText* text);
bool writeMText(EoDxfMText* mText);
bool writeHatch(EoDxfHatch* hatch);
// ... many more
```

##### Table Object Writing Methods

```cpp
bool writeLayer(EoDxfLayer* layer);
bool writeLineType(EoDxfLinetype* linetype);
bool writeTextstyle(EoDxfTextStyle* textStyle);
bool writeDimstyle(EoDxfDimensionStyle* dimensionStyle);
bool writeVport(EoDxfViewport* viewport);
bool writeAppId(EoDxfAppId* appId);
```

### Interface: EoDxfInterface

Applications must inherit from this interface and implement callback methods.

#### Required Methods

##### Header

```cpp
virtual void addHeader(const EoDxfHeader* data) = 0;
```

##### Table Objects

```cpp
virtual void addLayer(const EoDxfLayer& layer) = 0;
virtual void addLinetype(const EoDxfLinetype& linetype) = 0;
virtual void addDimStyle(const EoDxfDimensionStyle& dimensionStyle) = 0;
virtual void addTextStyle(const EoDxfTextStyle& textStyle) = 0;
virtual void addVport(const EoDxfViewport& viewport) = 0;
virtual void addAppId(const EoDxfAppId& appId) = 0;
```

##### Blocks

```cpp
virtual void addBlock(const EoDxfBlock& block) = 0;
virtual void setBlock(const int handle) = 0;
virtual void endBlock() = 0;
```

##### Entities

```cpp
virtual void addPoint(const EoDxfPoint& point) = 0;
virtual void addLine(const EoDxfLine& line) = 0;
virtual void addRay(const EoDxfRay& ray) = 0;
virtual void addXline(const EoDxfXline& xline) = 0;
virtual void addCircle(const EoDxfCircle& circle) = 0;
virtual void addArc(const EoDxfArc& arc) = 0;
virtual void addEllipse(const EoDxfEllipse& ellipse) = 0;
virtual void addLWPolyline(const EoDxfLwPolyline& polyline) = 0;
virtual void addPolyline(const EoDxfPolyline& polyline) = 0;
virtual void addSpline(const EoDxfSpline* spline) = 0;
virtual void addInsert(const EoDxfInsert& blockReference) = 0;
virtual void addTrace(const EoDxfTrace& trace) = 0;
virtual void add3dFace(const EoDxf3dFace& face) = 0;
virtual void addSolid(const EoDxfSolid& solid) = 0;
virtual void addMText(const EoDxfMText& mText) = 0;
virtual void addText(const EoDxfText& text) = 0;
virtual void addHatch(const EoDxfHatch* hatch) = 0;
virtual void addViewport(const EoDxfViewPort& data) = 0;

virtual void addImage(const EoDxfImage* image) = 0;
```

##### Dimensions

```cpp
virtual void addDimAlign(const EoDxfAlignedDimension *data) = 0;
virtual void addDimLinear(const EoDxfDimLinear *data) = 0;
virtual void addDimRadial(const EoDxfRadialDimension *data) = 0;
virtual void addDimDiametric(const EoDxfDiametricDimension *data) = 0;
virtual void addDimAngular(const EoDxf2LineAngularDimension *data) = 0;
virtual void addDimAngular3P(const EoDxf3PointAngularDimension *data) = 0;
virtual void addDimOrdinate(const EoDxfOrdinateDimension *data) = 0;
```

##### Others

```cpp
virtual void addLeader(const EoDxfLeader* leader) = 0;
virtual void linkImage(const EoDxfImageDefinition* imageDef) = 0;
virtual void addComment(const char* comment) = 0;
```

##### Writing Methods

```cpp
virtual void writeHeader(EoDxfHeader& data) = 0;
virtual void writeBlocks() = 0;
virtual void writeBlockRecords() = 0;
virtual void writeEntities() = 0;
virtual void writeLTypes() = 0;
virtual void writeLayers() = 0;
virtual void writeTextstyles() = 0;
virtual void writeVports() = 0;
virtual void writeDimstyles() = 0;
virtual void writeAppId() = 0;
```

---

## Build Systems

### 1. Autotools (Recommended)

```bash
autoreconf -vfi
./configure
make
sudo make install
```

#### Configuration Files
- `configure.ac`: Autoconf configuration
- `Makefile.am`: Automake templates
- Library version: 0.6.3 (CURRENT=6, REVISION=3, AGE=0)

#### Dependencies
- **Required**: libiconv (character conversion)
- **Standard Library**: stdlib.h, string.h
- **Math Functions**: sqrt()

### 2. CMake

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
sudo cmake --build . --config Release --target install
```

#### Key CMake Settings
- Minimum CMake version: 3.10
- Build target: Static library `dxfrw`
- Installation paths:
  - Headers: `include/`
  - Library: `lib/` (Linux/macOS), `Debug/lib/` or `Release/lib/` (Windows)

### 3. Visual Studio 2013

- Solution file: `vs2013/libdxfrw.sln`
- Project file: `vs2013/libdxfrw.vcxproj`
- NuGet package: libiconv 1.14.0.11

### 4. Docker

```bash
# Build image
docker build --rm -t codelibs/libdxfrw .

# Build library
docker run -t --rm -v `pwd`:/work codelibs/libdxfrw:latest /work/build.sh
```

- Base image: CentOS 7
- Output: `dxfrw.tar.gz` (install location: `/opt`)

---

## Utility Tools

### 3. dxf2txt.py

Python script to dump DXF files in text format.

```bash
python bin/dxf2txt.py input.dxf
```

---

## Data Structures

### Basic Types

```cpp
namespace DRW {
    enum Version {
        UNKNOWNV,    // Unknown
        R12,         // R12 DXF
        R14,         // R14 DXF
        R2000,       // R2000 DXF
        R2004,       // R2004 DXF
        R2007,       // R2007 DXF
        R2010,       // R2010 DXF
        R2013,       // R2013 DXF
        R2015        // R2015 DXF
    };
}
```

### Entity Base Class

```cpp
class EoDxfEntity {
public:
    enum DRW_EntityType {
        POINT, LINE, CIRCLE, ARC, ELLIPSE,
        LWPOLYLINE, POLYLINE, SPLINE,
        INSERT, TEXT, MTEXT, HATCH,
        DIMENSION, LEADER, VIEWPORT, IMAGE,
        // ... others
    };

    EoDxfGeometryBase3d basePoint;      // Base point
    std::string layer;        // Layer name
    std::string lineType;     // Line type
    int color;                // Color number
    double thickness;         // Thickness
    double ltypeScale;        // Line type scale
    int handle;               // Handle
    // ... other properties
};
```

### Coordinate Type

```cpp
class EoDxfGeometryBase3d {
public:
    double x;
    double y;
    double z;
};
```

---

## Character Encoding Support

libdxfrw supports multiple character code pages:

### Implementation Files

- `src/intern/EoDxfTextCodec.h/cpp`: Character encoding conversion engine
- `src/intern/EoDxfCodePageTables.h`: Code page table integration

---

## Performance Considerations

### Memory Management

- Entities and objects are passed by pointer or reference
- Memory management is handled by the interface implementation side

### Ellipse to Polyline Conversion

```cpp
void setEllipseParts(int parts);
```

Set the number of segments when converting ellipses to polylines. Increasing the number of segments improves accuracy.

---

## Test Data

External test data repository:
- [fess-testdata/autocad](https://github.com/codelibs/fess-testdata/tree/master/autocad)

Contains sample files of various DXF/DWG file versions, useful for integration testing.

---

## Known Limitations

1. **DWG File Writing Not Supported**: Only DWG file reading is supported
2. **Limited Entity Support**: May not support all entities from the latest AutoCAD versions
3. **Reed-Solomon Decoding**: Error correction functionality for DWG files is limited

---

## Version History

See the `ChangeLog` file for details.

Major development milestones:
- **2011**: Project started
- **2013**: Added DWG R2004 support
- **2014**: Added DWG R2010/R2013 support
- **2015**: Added DWG R2015 support
- **2020s**: Ongoing community maintenance

---

## License

GNU General Public License v2.0 (or later)

See the `COPYING` file for details.

---

## Contributors

- **José F. Soriano (Rallaz)**: Original author
- **Nicu Tofan**: Contributor
- **Miguel E. Hernández Cuervo**: Contributor
- **codelibs**: Maintainer

---

## References

- Official website: http://sourceforge.net/projects/libdxfrw
- GitHub: https://github.com/codelibs/libdxfrw
- DXF Reference: AutoCAD DXF Specification (Autodesk official documentation)

---

## Support and Community

Report issues and submit pull requests through the GitHub repository:
https://github.com/codelibs/libdxfrw/issues

---

Last Updated: 2025-11-08