#pragma once

#include <string_view>

namespace dxfrw {
namespace dxf {

// ==================== Structural / Section keywords ====================
constexpr std::string_view SECTION = "SECTION";
constexpr std::string_view ENDSEC = "ENDSEC";
constexpr std::string_view HEADER = "HEADER";
constexpr std::string_view CLASSES = "CLASSES";
constexpr std::string_view TABLES = "TABLES";
constexpr std::string_view BLOCKS = "BLOCKS";
constexpr std::string_view ENTITIES = "ENTITIES";
constexpr std::string_view OBJECTS = "OBJECTS";
constexpr std::string_view EOF_STR = "EOF";
constexpr std::string_view TABLE = "TABLE";
constexpr std::string_view ENDTAB = "ENDTAB";
constexpr std::string_view BLOCK = "BLOCK";
constexpr std::string_view ENDBLK = "ENDBLK";

// ==================== Entity types (for group code 0) ====================
namespace entity {
constexpr std::string_view POINT = "POINT";
constexpr std::string_view LINE = "LINE";
constexpr std::string_view RAY = "RAY";
constexpr std::string_view XLINE = "XLINE";
constexpr std::string_view CIRCLE = "CIRCLE";
constexpr std::string_view ARC = "ARC";
constexpr std::string_view ELLIPSE = "ELLIPSE";
constexpr std::string_view TRACE = "TRACE";
constexpr std::string_view SOLID = "SOLID";
constexpr std::string_view _3DFACE = "3DFACE";
constexpr std::string_view LWPOLYLINE = "LWPOLYLINE";
constexpr std::string_view POLYLINE = "POLYLINE";
constexpr std::string_view VERTEX = "VERTEX";
constexpr std::string_view SEQEND = "SEQEND";
constexpr std::string_view TEXT = "TEXT";
constexpr std::string_view MTEXT = "MTEXT";
constexpr std::string_view HATCH = "HATCH";
constexpr std::string_view SPLINE = "SPLINE";
constexpr std::string_view INSERT = "INSERT";
constexpr std::string_view DIMENSION = "DIMENSION";
constexpr std::string_view LEADER = "LEADER";
constexpr std::string_view VIEWPORT = "VIEWPORT";
constexpr std::string_view IMAGE = "IMAGE";
// ... add the remaining ~10 entities
}  // namespace entity

// ==================== Table types ====================
namespace table {
constexpr std::string_view VPORT = "VPORT";
constexpr std::string_view LTYPE = "LTYPE";
constexpr std::string_view LAYER = "LAYER";
constexpr std::string_view STYLE = "STYLE";
constexpr std::string_view VIEW = "VIEW";
constexpr std::string_view UCS = "UCS";
constexpr std::string_view APPID = "APPID";
constexpr std::string_view DIMSTYLE = "DIMSTYLE";
constexpr std::string_view BLOCK_RECORD = "BLOCK_RECORD";
}  // namespace table

// ==================== Subclass markers (AcDb*) ====================
namespace acdb {
constexpr std::string_view Entity = "AcDbEntity";
constexpr std::string_view Point = "AcDbPoint";
constexpr std::string_view Line = "AcDbLine";
constexpr std::string_view Circle = "AcDbCircle";
constexpr std::string_view Arc = "AcDbArc";
constexpr std::string_view Ellipse = "AcDbEllipse";
constexpr std::string_view Polyline2d = "AcDb2dPolyline";
constexpr std::string_view Polyline3d = "AcDb3dPolyline";
constexpr std::string_view LwPolyline = "AcDbPolyline";
constexpr std::string_view Hatch = "AcDbHatch";
constexpr std::string_view Spline = "AcDbSpline";
constexpr std::string_view Text = "AcDbText";
constexpr std::string_view MText = "AcDbMText";
constexpr std::string_view BlockBegin = "AcDbBlockBegin";
constexpr std::string_view BlockEnd = "AcDbBlockEnd";
constexpr std::string_view BlockReference = "AcDbBlockReference";
// ... continue for every AcDb* string in the file
}  // namespace acdb

// ==================== Special / reserved names ====================
namespace special {
constexpr std::string_view MODEL_SPACE = "*Model_Space";
constexpr std::string_view PAPER_SPACE = "*Paper_Space";
constexpr std::string_view ACTIVE = "*ACTIVE";
constexpr std::string_view STANDARD = "Standard";
constexpr std::string_view BYLAYER = "BYLAYER";
constexpr std::string_view BYBLOCK = "BYBLOCK";
constexpr std::string_view CONTINUOUS = "CONTINUOUS";
constexpr std::string_view ACAD = "ACAD";
constexpr std::string_view ACAD_GROUP = "ACAD_GROUP";
constexpr std::string_view ACAD_IMAGE_DICT = "ACAD_IMAGE_DICT";
}  // namespace special

// ==================== Binary DXF sentinel (22 bytes) ====================
constexpr char BINARY_SENTINEL[22] = {
    'A', 'u', 't', 'o', 'C', 'A', 'D', ' ', 'B', 'i', 'n', 'a', 'r', 'y', ' ', 'D', 'X', 'F', '\r', '\n', 26, '\0'};

// ==================== Common hardcoded handles (as string_view) ====================
namespace handle {
constexpr std::string_view ZERO = "0";
constexpr std::string_view ONE = "1";
constexpr std::string_view TWO = "2";
constexpr std::string_view FIVE = "5";
constexpr std::string_view NINE = "9";
constexpr std::string_view A = "A";
constexpr std::string_view C = "C";
constexpr std::string_view D = "D";
constexpr std::string_view MODEL = "1F";
constexpr std::string_view PAPER = "1E";
// ... add the rest
}  // namespace handle

}  // namespace dxf
}  // namespace dxfrw