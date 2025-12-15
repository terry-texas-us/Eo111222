#include "stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbDimension.h"
#include "EoDbEllipse.h"
#include "EoDbLine.h"
#include "EoDbPegFile.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbSpline.h"
#include "EoDbText.h"
#include "EoGeReferenceSystem.h"

void EoDbPegFile::Load(AeSysDoc* document) {
  ReadHeaderSection(document);
  ReadTablesSection(document);
  ReadBlocksSection(document);
  ReadEntitiesSection(document);
}
void EoDbPegFile::ReadHeaderSection(AeSysDoc*) {
  if (EoDb::ReadUInt16(*this) != EoDb::kHeaderSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kHeaderSection.";

  // 	with addition of info here will loop key-value pairs till EoDb::kEndOfSection sentinel

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.";
}
void EoDbPegFile::ReadTablesSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kTablesSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kTablesSection.";

  ReadViewportTable(document);
  ReadLinetypesTable(document);
  ReadLayerTable(document);

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.";
}
void EoDbPegFile::ReadViewportTable(AeSysDoc*) {
  if (EoDb::ReadUInt16(*this) != EoDb::kViewPortTable)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kViewPortTable.";

  EoDb::ReadUInt16(*this);

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.";
}
void EoDbPegFile::ReadLinetypesTable(AeSysDoc* document) {
  EoDbLineTypeTable* LineTypeTable = document->LineTypeTable();

  if (EoDb::ReadUInt16(*this) != EoDb::kLinetypeTable)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kLinetypeTable.";

  double* DashLength = new double[32];

  EoUInt16 NumberOfLinetypes = EoDb::ReadUInt16(*this);

  for (EoUInt16 n = 0; n < NumberOfLinetypes; n++) {
    CString Name;
    CString Description;
    EoUInt16 DefinitionLength;
    ReadLinetypeDefinition(DashLength, Name, Description, DefinitionLength);

    EoDbLineType* LineType;
    if (!LineTypeTable->Lookup(Name, LineType)) {
      EoUInt16 Index = LineTypeTable->LegacyLineTypeIndex(Name);
      LineTypeTable->SetAt(Name, new EoDbLineType(Index, Name, Description, DefinitionLength, DashLength));
    }
  }
  delete[] DashLength;

  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.";
}
void EoDbPegFile::ReadLinetypeDefinition(double* dashLength, CString& name, CString& description,
                                         EoUInt16& definitionLength) {
  EoDb::Read(*this, name);
  /* EoUInt16 Flags = */ EoDb::ReadUInt16(*this);
  EoDb::Read(*this, description);
  definitionLength = EoDb::ReadUInt16(*this);
  double PatternLength;
  EoDb::Read(*this, PatternLength);

  for (EoUInt16 n = 0; n < definitionLength; n++) { EoDb::Read(*this, dashLength[n]); }
}
void EoDbPegFile::ReadLayerTable(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kLayerTable)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kLayerTable.";

  CString Name;
  CString LineTypeName;
  EoUInt16 NumberOfLayers = EoDb::ReadUInt16(*this);
  for (EoUInt16 n = 0; n < NumberOfLayers; n++) {
    EoDb::Read(*this, Name);
    EoUInt16 TracingFlags = EoDb::ReadUInt16(*this);
    EoUInt16 StateFlags = EoDb::ReadUInt16(*this);

    StateFlags |= EoDbLayer::kIsResident;

    if ((StateFlags & EoDbLayer::kIsInternal) != EoDbLayer::kIsInternal) {
      if (Name.Find('.') == -1) Name += L".jb1";
    }
    EoInt16 ColorIndex = EoDb::ReadInt16(*this);
    EoDb::Read(*this, LineTypeName);

    if (document->FindLayerTableLayer(Name) < 0) {
      EoDbLayer* Layer = new EoDbLayer(Name, StateFlags);

      Layer->SetTracingFlg(TracingFlags);
      Layer->SetColorIndex(ColorIndex);

      EoDbLineType* LineType;
      document->LineTypeTable()->Lookup(LineTypeName, LineType);
      Layer->SetLineType(LineType);

      document->AddLayerTableLayer(Layer);

      if (Layer->IsWork()) { document->SetWorkLayer(Layer); }
#if defined(USING_ODA)
      document->ConvertLayerTable();
#endif  // USING_ODA
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfTable)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfTable.";
}
void EoDbPegFile::ReadBlocksSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kBlocksSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kBlocksSection.";

  EoDbPrimitive* Primitive;
  CString Name;
  CString XRefPathName;

  EoUInt16 NumberOfBlocks = EoDb::ReadUInt16(*this);

  for (EoUInt16 n = 0; n < NumberOfBlocks; n++) {
    EoUInt16 NumberOfPrimitives = EoDb::ReadUInt16(*this);

    EoDb::Read(*this, Name);
    EoUInt16 BlockTypeFlags = EoDb::ReadUInt16(*this);
    EoGePoint3d BasePoint = EoDb::ReadPoint3d(*this);
    EoDbBlock* Block = new EoDbBlock(BlockTypeFlags, BasePoint, XRefPathName);
    document->InsertBlock(Name, Block);

    for (EoUInt16 PrimitiveIndex = 0; PrimitiveIndex < NumberOfPrimitives; PrimitiveIndex++) {
      EoDb::Read(*this, Primitive);
      Block->AddTail(Primitive);
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.";
}
void EoDbPegFile::ReadEntitiesSection(AeSysDoc* document) {
  if (EoDb::ReadUInt16(*this) != EoDb::kGroupsSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kGroupsSection.";

  EoDbPrimitive* Primitive;

  EoUInt16 NumberOfLayers = EoDb::ReadUInt16(*this);

  for (EoUInt16 n = 0; n < NumberOfLayers; n++) {
    EoDbLayer* Layer = document->GetLayerTableLayerAt(n);

    if (Layer == 0) return;

    EoUInt16 NumberOfGroups = EoDb::ReadUInt16(*this);

    if (Layer->IsInternal()) {
      for (EoUInt16 GroupIndex = 0; GroupIndex < NumberOfGroups; GroupIndex++) {
        EoDbGroup* Group = new EoDbGroup;
        EoUInt16 NumberOfPrimitives = EoDb::ReadUInt16(*this);
        for (EoUInt16 PrimitiveIndex = 0; PrimitiveIndex < NumberOfPrimitives; PrimitiveIndex++) {
          EoDb::Read(*this, Primitive);
          Group->AddTail(Primitive);
        }
        Layer->AddTail(Group);
      }
    } else {
      document->TracingLoadLayer(Layer->Name(), Layer);
    }
  }
  if (EoDb::ReadUInt16(*this) != EoDb::kEndOfSection)
    throw L"Exception EoDbPegFile: Expecting sentinel EoDb::kEndOfSection.";
}
void EoDbPegFile::Unload(AeSysDoc* document) {
  CFile::SetLength(0);
  CFile::SeekToBegin();

  WriteHeaderSection(document);
  WriteTablesSection(document);
  WriteBlocksSection(document);
  WriteEntitiesSection(document);
  EoDb::Write(*this, L"EOF");

  CFile::Flush();
}
void EoDbPegFile::WriteHeaderSection(AeSysDoc*) {
  EoDb::Write(*this, EoUInt16(EoDb::kHeaderSection));

  // header variable items go here

  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void EoDbPegFile::WriteTablesSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kTablesSection));

  WriteVPortTable(document);
  WriteLinetypeTable(document);
  WriteLayerTable(document);
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void EoDbPegFile::WriteVPortTable(AeSysDoc*) {
  EoDb::Write(*this, EoUInt16(EoDb::kViewPortTable));
  EoDb::Write(*this, EoUInt16(0));
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfTable));
}
void EoDbPegFile::WriteLinetypeTable(AeSysDoc* document) {
  EoDbLineTypeTable* LineTypeTable = document->LineTypeTable();

  EoUInt16 NumberOfLinetypes = EoUInt16(LineTypeTable->Size());

  EoDb::Write(*this, EoUInt16(EoDb::kLinetypeTable));
  EoDb::Write(*this, NumberOfLinetypes);

  CString Name;
  EoDbLineType* Linetype;

  POSITION Position = LineTypeTable->GetStartPosition();
  while (Position) {
    LineTypeTable->GetNextAssoc(Position, Name, Linetype);

    EoDb::Write(*this, (LPCWSTR)Name);
    EoDb::Write(*this, EoUInt16(0));
    EoDb::Write(*this, (LPCWSTR)Linetype->Description());

    EoUInt16 DefinitionLength = Linetype->GetNumberOfDashes();
    EoDb::Write(*this, DefinitionLength);
    double PatternLength = Linetype->GetPatternLen();
    EoDb::Write(*this, PatternLength);

    if (DefinitionLength > 0) {
      double* DashLength = new double[DefinitionLength];
      Linetype->GetDashLen(DashLength);
      for (EoUInt16 w = 0; w < DefinitionLength; w++) EoDb::Write(*this, DashLength[w]);

      delete[] DashLength;
    }
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfTable));
}
void EoDbPegFile::WriteLayerTable(AeSysDoc* document) {
  int NumberOfLayers = document->GetLayerTableSize();

  EoDb::Write(*this, EoUInt16(EoDb::kLayerTable));

  ULONGLONG SavedFilePosition = CFile::GetPosition();
  EoDb::Write(*this, EoUInt16(NumberOfLayers));

  for (int n = 0; n < document->GetLayerTableSize(); n++) {
    EoDbLayer* Layer = document->GetLayerTableLayerAt(n);
    if (Layer->IsResident()) {
      EoDb::Write(*this, Layer->Name());
      EoDb::Write(*this, Layer->GetTracingFlgs());
      EoDb::Write(*this, Layer->LayerStateFlags());
      EoDb::Write(*this, Layer->ColorIndex());
      EoDb::Write(*this, Layer->LineTypeName());
    } else
      NumberOfLayers--;
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfTable));

  if (NumberOfLayers != document->GetLayerTableSize()) {
    ULONGLONG CurrentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(SavedFilePosition), CFile::begin);
    EoDb::Write(*this, EoUInt16(NumberOfLayers));
    CFile::Seek(static_cast<LONGLONG>(CurrentFilePosition), CFile::begin);
  }
}
void EoDbPegFile::WriteBlocksSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kBlocksSection));

  EoUInt16 NumberOfBlocks = document->BlockTableSize();
  EoDb::Write(*this, NumberOfBlocks);

  CString Name;
  EoDbBlock* Block;

  POSITION Position = document->GetFirstBlockPosition();
  while (Position != 0) {
    document->GetNextBlock(Position, Name, Block);

    ULONGLONG SavedFilePosition = CFile::GetPosition();
    EoDb::Write(*this, EoUInt16(0));
    EoUInt16 NumberOfPrimitives = 0;

    EoDb::Write(*this, Name);
    EoDb::Write(*this, Block->GetBlkTypFlgs());
    Block->GetBasePt().Write(*this);

    POSITION PrimitivePosition = Block->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Block->GetNext(PrimitivePosition);
      if (Primitive->Write(*this)) NumberOfPrimitives++;
    }
    ULONGLONG CurrentFilePosition = CFile::GetPosition();
    CFile::Seek(static_cast<LONGLONG>(SavedFilePosition), CFile::begin);
    EoDb::Write(*this, NumberOfPrimitives);
    CFile::Seek(static_cast<LONGLONG>(CurrentFilePosition), CFile::begin);
  }

  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}
void EoDbPegFile::WriteEntitiesSection(AeSysDoc* document) {
  EoDb::Write(*this, EoUInt16(EoDb::kGroupsSection));

  int NumberOfLayers = document->GetLayerTableSize();
  EoDb::Write(*this, EoUInt16(NumberOfLayers));

  for (int n = 0; n < NumberOfLayers; n++) {
    EoDbLayer* Layer = document->GetLayerTableLayerAt(n);
    if (Layer->IsInternal()) {
      EoDb::Write(*this, EoUInt16(Layer->GetCount()));

      POSITION Position = Layer->GetHeadPosition();
      while (Position != 0) {
        EoDbGroup* Group = Layer->GetNext(Position);
        Group->Write(*this);
      }
    } else
      EoDb::Write(*this, EoUInt16(0));
  }
  EoDb::Write(*this, EoUInt16(EoDb::kEndOfSection));
}

bool EoDb::Read(CFile& file, EoDbPrimitive*& primitive) {
  switch (EoDb::ReadUInt16(file)) {
    case EoDb::kPointPrimitive:
      ConstructPointPrimitive(file, primitive);
      break;
    case EoDb::kInsertPrimitive:
      ConstructBlockReferencePrimitiveFromInsertPrimitive(file, primitive);
      break;
    case EoDb::kGroupReferencePrimitive:
      ConstructBlockReferencePrimitive(file, primitive);
      break;
    case EoDb::kLinePrimitive:
      ConstructLinePrimitive(file, primitive);
      break;
    case EoDb::kPolygonPrimitive:
      ConstructPolygonPrimitive(file, primitive);
      break;
    case EoDb::kEllipsePrimitive:
      ConstructEllipsePrimitive(file, primitive);
      break;
    case EoDb::kSplinePrimitive:
      ConstructSplinePrimitive(file, primitive);
      break;
    case EoDb::kCSplinePrimitive:
      ConstructPolylinePrimitiveFromCSplinePrimitive(file, primitive);
      break;
    case EoDb::kPolylinePrimitive:
      ConstructPolylinePrimitive(file, primitive);
      break;
    case EoDb::kTextPrimitive:
      ConstructTextPrimitive(file, primitive);
      break;
    case EoDb::kDimensionPrimitive:
      ConstructDimensionPrimitive(file, primitive);
      break;
    case EoDb::kTagPrimitive:
      ConstructPointPrimitiveFromTagPrimitive(file, primitive);
      break;

    default:
      app.WarningMessageBox(IDS_MSG_BAD_PRIM_TYPE);
      return false;
  }
  return true;
}
void EoDb::Read(CFile& file, CString& string) {
  string.Empty();
  char c;
  while (file.Read(&c, 1) == 1) {
    if (c == '\t') return;
    string += c;
  }
}
void EoDb::Read(CFile& file, double& number) { file.Read(&number, sizeof(double)); }
void EoDb::Read(CFile& file, EoInt16& number) { file.Read(&number, sizeof(EoInt16)); }
EoGePoint3d EoDb::ReadPoint3d(CFile& file) {
  EoGePoint3d Point;
  Point.Read(file);
  return Point;
}
EoGeVector3d EoDb::ReadVector3d(CFile& file) {
  EoGeVector3d Vector;
  Vector.Read(file);
  return Vector;
}
void EoDb::Read(CFile& file, EoUInt16& number) { file.Read(&number, sizeof(EoUInt16)); }
double EoDb::ReadDouble(CFile& file) {
  double number;
  file.Read(&number, sizeof(double));
  return number;
}
EoInt16 EoDb::ReadInt16(CFile& file) {
  EoInt16 number;
  file.Read(&number, sizeof(EoInt16));
  return number;
}
EoUInt16 EoDb::ReadUInt16(CFile& file) {
  EoUInt16 number;
  file.Read(&number, sizeof(EoUInt16));
  return number;
}
void EoDb::Write(CFile& file, const CString& string) {
  int NumberOfCharacters = string.GetLength();
  for (int n = 0; n < NumberOfCharacters; n++) {
    char c = EoSbyte(string.GetAt(n));
    file.Write(&c, 1);
  }
  file.Write("\t", 1);
}
void EoDb::Write(CFile& file, double number) { file.Write(&number, sizeof(double)); }
void EoDb::Write(CFile& file, EoInt16 number) { file.Write(&number, sizeof(EoInt16)); }
void EoDb::Write(CFile& file, EoUInt16 number) { file.Write(&number, sizeof(EoUInt16)); }
void EoDb::ConstructBlockReferencePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  CString Name;
  EoDb::Read(file, Name);
  EoGePoint3d Point(EoDb::ReadPoint3d(file));
  EoGeVector3d Normal(EoDb::ReadVector3d(file));
  EoGeVector3d ScaleFactors(EoDb::ReadVector3d(file));
  double Rotation = EoDb::ReadDouble(file);
  /* EoUInt16 NumberOfColumns = */ EoDb::ReadUInt16(file);
  /* EoUInt16 NumberOfRows = */ EoDb::ReadUInt16(file);
  /* double ColumnSpacing = */ EoDb::ReadDouble(file);
  /* double RowSpacing = */ EoDb::ReadDouble(file);

  primitive = new EoDbBlockReference(static_cast<EoUInt16>(PenColor), static_cast<EoUInt16>(LineType), Name, Point,
                                     Normal, ScaleFactors, Rotation);
}
void EoDb::ConstructBlockReferencePrimitiveFromInsertPrimitive(CFile& /* file */, EoDbPrimitive*& /* primitive */) {}
void EoDb::ConstructDimensionPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  EoGePoint3d BeginPoint = EoDb::ReadPoint3d(file);
  EoGePoint3d EndPoint = EoDb::ReadPoint3d(file);
  EoInt16 TextPenColor = EoDb::ReadInt16(file);
  EoDbFontDefinition FontDefinition;
  FontDefinition.Read(file);
  EoGeReferenceSystem ReferenceSystem;
  ReferenceSystem.Read(file);
  CString Text;
  EoDb::Read(file, Text);

  primitive = new EoDbDimension(PenColor, LineType, EoGeLine(BeginPoint, EndPoint), TextPenColor, FontDefinition,
                                ReferenceSystem, Text);
}
void EoDb::ConstructEllipsePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  EoGePoint3d CenterPoint(ReadPoint3d(file));
  EoGeVector3d MajorAxis(ReadVector3d(file));
  EoGeVector3d MinorAxis(ReadVector3d(file));
  double SweepAngle;
  EoDb::Read(file, SweepAngle);
  primitive = new EoDbEllipse(PenColor, LineType, CenterPoint, MajorAxis, MinorAxis, SweepAngle);
}
void EoDb::ConstructLinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  EoGePoint3d BeginPoint(EoDb::ReadPoint3d(file));
  EoGePoint3d EndPoint(EoDb::ReadPoint3d(file));
  primitive = new EoDbLine(PenColor, LineType, BeginPoint, EndPoint);
}
void EoDb::ConstructPointPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 PointStyle = EoDb::ReadInt16(file);

  EoGePoint3d Point(EoDb::ReadPoint3d(file));
  EoUInt16 NumberOfDatums = EoDb::ReadUInt16(file);

  double* Data = (NumberOfDatums == 0) ? 0 : new double[NumberOfDatums];

  for (EoUInt16 n = 0; n < NumberOfDatums; n++) { EoDb::Read(file, Data[n]); }
  primitive = new EoDbPoint(PenColor, PointStyle, Point, NumberOfDatums, Data);
}
void EoDb::ConstructPointPrimitiveFromTagPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 PointStyle = EoDb::ReadInt16(file);
  EoGePoint3d Point(EoDb::ReadPoint3d(file));
  primitive = new EoDbPoint(PenColor, PointStyle, Point);
}
void EoDb::ConstructPolygonPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 InteriorStyle = EoDb::ReadInt16(file);
  EoInt16 InteriorStyleIndex = EoDb::ReadInt16(file);
  EoUInt16 NumberOfPoints = EoDb::ReadUInt16(file);
  EoGePoint3d HatchOrigin(EoDb::ReadPoint3d(file));
  EoGeVector3d HatchXAxis(EoDb::ReadVector3d(file));
  EoGeVector3d HatchYAxis(EoDb::ReadVector3d(file));

  EoGePoint3dArray Points;
  Points.SetSize(NumberOfPoints);
  for (EoUInt16 n = 0; n < NumberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbPolygon(PenColor, InteriorStyle, InteriorStyleIndex, HatchOrigin, HatchXAxis, HatchYAxis, Points);
}
void EoDb::ConstructPolylinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);
  EoUInt16 NumberOfPoints = EoDb::ReadUInt16(file);

  EoGePoint3dArray Points;
  Points.SetSize(NumberOfPoints);

  for (EoUInt16 n = 0; n < NumberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbPolyline(PenColor, LineType, Points);
}
void EoDb::ConstructPolylinePrimitiveFromCSplinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);

  file.Seek(sizeof(EoUInt16), CFile::current);
  EoUInt16 NumberOfPoints = EoDb::ReadUInt16(file);
  file.Seek(sizeof(EoUInt16), CFile::current);
  file.Seek(sizeof(EoGeVector3d), CFile::current);
  file.Seek(sizeof(EoGeVector3d), CFile::current);
  EoGePoint3dArray Points;
  Points.SetSize(NumberOfPoints);
  for (EoUInt16 n = 0; n < NumberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbPolyline(PenColor, LineType, Points);
}
void EoDb::ConstructSplinePrimitive(CFile& file, EoDbPrimitive*& primitive) {
  EoInt16 PenColor = EoDb::ReadInt16(file);
  EoInt16 LineType = EoDb::ReadInt16(file);

  EoUInt16 NumberOfPoints = EoDb::ReadUInt16(file);

  EoGePoint3dArray Points;
  Points.SetSize(NumberOfPoints);

  for (EoUInt16 n = 0; n < NumberOfPoints; n++) { Points[n] = EoDb::ReadPoint3d(file); }
  primitive = new EoDbSpline(PenColor, LineType, Points);
}
void EoDb::ConstructTextPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  /* EoInt16 PenColor = */ EoDb::ReadInt16(file);
  /* EoInt16 LineType = */ EoDb::ReadInt16(file);
  EoDbFontDefinition FontDefinition;
  FontDefinition.Read(file);
  EoGeReferenceSystem ReferenceSystem;
  ReferenceSystem.Read(file);
  CString Text;
  EoDb::Read(file, Text);

  primitive = new EoDbText(FontDefinition, ReferenceSystem, Text);
  static_cast<EoDbText*>(primitive)->ConvertFormattingCharacters();
}
