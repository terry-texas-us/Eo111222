#pragma once

// File if detailed on repo Markdowns folder.`Job File Format.md` https://github.com/terry-texas-us/Eo111222/blob/6f68dc865e9cc90c9c42ce9194740f39b7adbc57/AeSys/Markdowns/Job%20File%20Format.md?plain=1in

#include <afx.h>
#include <string.h>

#include "EoDbGroup.h"
#include "EoDbLayer.h"
#include "EoDbPrimitive.h"
#include "EoGeVector3d.h"

class EoDbJobFile {
 private:
  int m_Version;
  EoUInt8* m_PrimBuf;

 public:
  EoDbJobFile() {
    m_Version = 3;
    m_PrimBuf = new EoUInt8[EoDbPrimitive::BUFFER_SIZE];
  }
  virtual ~EoDbJobFile() { delete[] m_PrimBuf; }
  /// <summary>Reads document data from a memory file and adds all groups to the trap with a translation. This is a data stream retrieved from the clipboard.</summary>
  void ReadMemFile(CFile& file, EoGeVector3d translateVector);
  void ReadHeader(CFile& file);
  void ReadLayer(CFile& file, EoDbLayer* layer);

  bool GetNextVisibleGroup(CFile& file, EoDbGroup*& group);
  bool GetNextPrimitive(CFile& file, EoDbPrimitive*& primitve);
  bool ReadNextPrimitive(CFile& file, EoUInt8* buffer, EoInt16& primitiveType) const;

  int Version();
  static bool IsValidPrimitive(EoInt16 primitiveType);
  static bool IsValidVersion1Primitive(EoInt16 primitiveType);

  void WriteHeader(CFile& file);
  void WriteLayer(CFile& file, EoDbLayer* layer);
  void WriteGroup(CFile& file, EoDbGroup* group);
  void ConstructPrimitive(EoDbPrimitive*& primitive, EoInt16 PrimitiveType);
  void ConstructPrimitiveFromVersion1(EoDbPrimitive*& primitive);

  EoDbPrimitive* ConvertEllipsePrimitive();
  EoDbPrimitive* ConvertLinePrimitive();
  EoDbPrimitive* ConvertPointPrimitive();

  EoDbPrimitive* ConvertVersion1EllipsePrimitive();
  EoDbPrimitive* ConvertVersion1LinePrimitive();
  EoDbPrimitive* ConvertVersion1PointPrimitive();

  /// <summary> Converts a deprecated version 1 CSpline to a BSpline</summary>
  void ConvertCSplineToBSpline();
  void ConvertTagToPoint();
};
