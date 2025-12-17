#include "stdafx.h"

#include "AeSysDoc.h"
#include "EoDbCharacterCellDefinition.h"

#if defined(USING_ODA)
#include "RxObjectImpl.h"
#include "Db2LineAngularDimension.h"
#include "Db2dPolyline.h"
#include "Db3PointAngularDimension.h"
#include "Db3dPolyline.h"
#include "Db3dPolylineVertex.h"
#include "Db3dSolid.h"
#include "DbAlignedDimension.h"
#include "DbArc.h"
#include "DbArcAlignedText.h"
#include "DbArcDimension.h"
#include "DbAttribute.h"
#include "DbAttributeDefinition.h"
#include "DbBlockReference.h"
#include "DbBlockTableRecord.h"
#include "DbBody.h"
#include "DbCircle.h"
#include "DbDiametricDimension.h"
#include "DbEllipse.h"
#include "DbFace.h"
#include "DbFaceRecord.h"
#include "DbFcf.h"
#include "DbHatch.h"
#include "DbIndex.h"
#include "DbLeader.h"
#include "DbLine.h"
#include "DbMInsertBlock.h"
#include "DbMText.h"
#include "DbMline.h"
#include "DbOle2Frame.h"
#include "DbOrdinateDimension.h"
#include "DbPoint.h"
#include "DbPolyFaceMesh.h"
#include "DbPolyFaceMeshVertex.h"
#include "DbPolygonMesh.h"
#include "DbPolygonMeshVertex.h"
#include "DbPolyline.h"
#include "DbProxyEntity.h"
#include "DbRadialDimension.h"
#include "DbRasterImage.h"
#include "DbRay.h"
#include "DbRegion.h"
#include "DbRotatedDimension.h"
#include "DbShape.h"
#include "DbSolid.h"
#include "DbSpatialFilter.h"
#include "DbSpline.h"
#include "DbTable.h"
#include "DbTextStyleTableRecord.h"
#include "DbTrace.h"
#include "DbViewport.h"
#include "DbWipeout.h"
#include "DbXline.h"
#include "Ge/GeCircArc2d.h"
#include "Ge/GeCircArc3d.h"
#include "Ge/GeCurve2d.h"
#include "Ge/GeEllipArc2d.h"
#include "Ge/GeKnotVector.h"
#include "Ge/GeNurbCurve2d.h"
#include "GeometryFromProxy.h"
#include "Gs/Gs.h"

#include "D:/Teigha/TD_vc10mtdbg/TD/Extensions/ExServices/OdFileBuf.h"
#include "StaticRxObject.h"

#include "EntityToPrimitiveProtocolExtension.h"

extern CTraceCategory traceOdDb;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// <summary>Construction/Destruction</summary>

ODRX_NO_CONS_DEFINE_MEMBERS(EoDbConvertEntityToPrimitive, OdRxObject)

void ConvertEntityData(OdDbEntity* entity, EoDbPrimitive* primitive) {
  OdDbDatabasePtr DatabasePtr = entity->database();

  OdCmColor Color = entity->color();

  if (Color.isByBlock()) {
    primitive->PenColor(7);
  } else if (Color.isByLayer()) {
    primitive->PenColor(EoDbPrimitive::PENCOLOR_BYLAYER);
  } else {
    primitive->PenColor(Color.colorIndex());
  }
  OdDbObjectId Linetype = entity->linetypeId();

  if (Linetype == DatabasePtr->getLinetypeByBlockId()) {
    primitive->LineType(EoDbPrimitive::LINETYPE_BYBLOCK);
  } else if (Linetype == DatabasePtr->getLinetypeByLayerId()) {
    primitive->LineType(EoDbPrimitive::LINETYPE_BYLAYER);
  } else {
    AeSysDoc* Document = ProtocolExtension_ConvertEntityToPegPrimitive::m_Document;
    EoDbLineTypeTable* LineTypeTable = Document->LineTypeTable();

    CString Name = (PCTSTR)entity->linetype();
    EoDbLineType* LineType;
    LineTypeTable->Lookup(Name, LineType);

    primitive->LineType(LineType->Index());
  }
  OdGeExtents3d extents;
  if (eOk == entity->getGeomExtents(extents)) {
    ATLTRACE2(traceOdDb, 2, L"Min Extents: %f, %f, %f\n", extents.minPoint());
    ATLTRACE2(traceOdDb, 2, L"Max Extents: %f, %f, %f\n", extents.maxPoint());
  }
  ATLTRACE2(traceOdDb, 2, L"Layer: %s\n", (PCTSTR)entity->layer());
  ATLTRACE2(traceOdDb, 2, L"Color Index: %i\n", entity->colorIndex());
  ATLTRACE2(traceOdDb, 2, L"Color: %i\n", entity->color());
  ATLTRACE2(traceOdDb, 2, L"Linetype: %i\n", entity->linetype());
  ATLTRACE2(traceOdDb, 2, L"LTscale: %f\n", entity->linetypeScale());
  ATLTRACE2(traceOdDb, 2, L"Lineweight: %i\n", entity->lineWeight());
  ATLTRACE2(traceOdDb, 2, L"Plot Style: %i\n", entity->plotStyleName());
  ATLTRACE2(traceOdDb, 2, L"Transparency Method: %i\n", entity->transparency().method());
  ATLTRACE2(traceOdDb, 2, L"Visibility: %i\n", entity->visibility());
  ATLTRACE2(traceOdDb, 2, L"Planar: %i\n", entity->isPlanar());

  OdGePlane plane;
  OdDb::Planarity planarity = OdDb::kNonPlanar;
  entity->getPlane(plane, planarity);
  ATLTRACE2(traceOdDb, 2, L"Planarity: %i\n", planarity);
  if (entity->isPlanar()) {
    OdGePoint3d origin;
    OdGeVector3d uAxis;
    OdGeVector3d vAxis;
    plane.get(origin, uAxis, vAxis);
    ATLTRACE2(traceOdDb, 2, L"Origin: %f, %f, %f\n", origin);
    ATLTRACE2(traceOdDb, 2, L"u-Axis: %f, %f, %f\n", uAxis);
    ATLTRACE2(traceOdDb, 2, L"v-Axis: %f, %f, %f\n", vAxis);
  }
}
void ConvertTextData(OdDbText* text, EoDbGroup* group) {
  ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbText ...\n", (PCTSTR)text->desc()->name());

  ATLTRACE2(traceOdDb, 2, L"Text Style: %i\n", text->textStyle());
  OdDbObjectId TextStyleObjectId = text->textStyle();
  OdDbTextStyleTableRecordPtr TextStyleTableRecordPtr = TextStyleObjectId.safeOpenObject();

  OdString FileName;
  if (TextStyleTableRecordPtr->isShapeFile()) {
    FileName = L"Standard";
    ATLTRACE2(traceOdDb, 2, L"TextStyle references shape library %s.\n",
              (PCTSTR)TextStyleTableRecordPtr->desc()->name());
  } else {
    FileName = TextStyleTableRecordPtr->fileName();
    int nExt = FileName.reverseFind('.');
    if (nExt != -1) {
      if (FileName.mid(nExt).compare(L".shx") == 0) {
        FileName = FileName.left(nExt);
        for (int n = nExt; n < 8; n++) { FileName += '_'; }
        FileName += L".ttf";
      }
    }
  }
  ATLTRACE2(traceOdDb, 2, L"Vertical Mode: %i\n", text->verticalMode());
  EoUInt16 VerticalAlignment;
  switch (text->verticalMode()) {
    case OdDb::kTextVertMid:
      VerticalAlignment = EoDb::kAlignMiddle;
      break;

    case OdDb::kTextTop:
      VerticalAlignment = EoDb::kAlignTop;
      break;

    default:  // OdDb::kTextBottom & OdDb::kTextBase
      VerticalAlignment = EoDb::kAlignBottom;
  }
  ATLTRACE2(traceOdDb, 2, L"Horizontal Mode: %i\n", text->horizontalMode());
  EoUInt16 HorizontalAlignment;
  switch (text->horizontalMode()) {
    case OdDb::kTextMid:
    case OdDb::kTextCenter:
      HorizontalAlignment = EoDb::kAlignCenter;
      break;

    case OdDb::kTextRight:
    case OdDb::kTextAlign:
    case OdDb::kTextFit:
      HorizontalAlignment = EoDb::kAlignRight;
      break;

    default:  // OdDb::kTextLeft
      HorizontalAlignment = EoDb::kAlignLeft;
  }
  ATLTRACE2(traceOdDb, 2, L"Text Position: %f, %f, %f\n", text->position());
  ATLTRACE2(traceOdDb, 2, L"Alignment Point: %f, %f, %f\n", text->alignmentPoint());
  OdGePoint3d AlignmentPoint = text->position();
  if (HorizontalAlignment != EoDb::kAlignLeft || VerticalAlignment != EoDb::kAlignBottom)
    AlignmentPoint = text->alignmentPoint();

  EoDbFontDefinition FontDefinition(EoDb::kEoTrueType, (PCTSTR)FileName, EoDb::kPathRight, HorizontalAlignment,
                                    VerticalAlignment, 0.0);

  ATLTRACE2(traceOdDb, 2, L"Rotation: %f\n", text->rotation());
  ATLTRACE2(traceOdDb, 2, L"Oblique: %f\n", text->oblique());
  ATLTRACE2(traceOdDb, 2, L"Width Factor: %i\n", text->widthFactor());
  ATLTRACE2(traceOdDb, 2, L"Height: %f\n", text->height());
  EoDbCharacterCellDefinition ccd(text->rotation(), text->oblique(), text->widthFactor(), text->height());

  EoGeVector3d XDirection;
  EoGeVector3d YDirection;
  CharCellDef_EncdRefSys(text->normal(), ccd, XDirection, YDirection);

  EoGeReferenceSystem ReferenceSystem(AlignmentPoint, XDirection, YDirection);
  ATLTRACE2(traceOdDb, 0, L"Text String: %s\n", (PCTSTR)text->textString());
  EoDbText* TextPrimitive = new EoDbText(FontDefinition, ReferenceSystem, (PCTSTR)text->textString());

  ConvertEntityData(text, TextPrimitive);
  group->AddTail(TextPrimitive);

  ATLTRACE2(traceOdDb, 2, L"Default Alignment: %i\n", text->isDefaultAlignment());
  ATLTRACE2(traceOdDb, 2, L"Mirrored in X: %i\n", text->isMirroredInX());
  ATLTRACE2(traceOdDb, 2, L"Mirrored in Y: %i\n", text->isMirroredInY());
  OdGePoint3dArray points;
  text->getBoundingPoints(points);
  ATLTRACE2(traceOdDb, 2, L"TL Bounding Point: %f, %f, %f\n", points[0]);
  ATLTRACE2(traceOdDb, 2, L"TR Bounding Point: %f, %f, %f\n", points[1]);
  ATLTRACE2(traceOdDb, 2, L"BL Bounding Point: %f, %f, %f\n", points[2]);
  ATLTRACE2(traceOdDb, 2, L"BR Bounding Point: %f, %f, %f\n", points[3]);
  ATLTRACE2(traceOdDb, 2, L"Normal: %f, %f, %f\n", text->normal());
  ATLTRACE2(traceOdDb, 2, L"Thickness: %f\n", text->thickness());
};
void ConvertAttributeData(OdDbAttribute* attribute) {
  ATLTRACE2(traceOdDb, 2, L"Tag: %s\n", (PCTSTR)attribute->tag());
  ATLTRACE2(traceOdDb, 2, L"Field Length: %s\n", (PCTSTR)attribute->fieldLength());
  ATLTRACE2(traceOdDb, 2, L"Invisible: %i\n", (PCTSTR)attribute->isInvisible());
  ATLTRACE2(traceOdDb, 2, L"Preset: %i\n", (PCTSTR)attribute->isPreset());
  ATLTRACE2(traceOdDb, 2, L"Verifiable: %i\n", (PCTSTR)attribute->isVerifiable());
  ATLTRACE2(traceOdDb, 2, L"Locked in Position: %i\n", (PCTSTR)attribute->lockPositionInBlock());
  ATLTRACE2(traceOdDb, 2, L"Constant: %i\n", (PCTSTR)attribute->isConstant());
};

void ConvertDimensionData(OdDbDimension* dimension) {
  OdDbBlockTableRecordPtr Block = dimension->dimBlockId().safeOpenObject();
  ATLTRACE2(traceOdDb, 2, L"Measurement: %f\n", dimension->getMeasurement());
  ATLTRACE2(traceOdDb, 2, L"Dimension Text: %s\n", (PCTSTR)dimension->dimensionText());

  if (dimension->getMeasurement() >= 0.0) {
    OdString formattedMeasurement;
    dimension->formatMeasurement(formattedMeasurement, dimension->getMeasurement(), dimension->dimensionText());
    ATLTRACE2(traceOdDb, 2, L"Formatted Measurement: %s\n", (PCTSTR)formattedMeasurement);
  }
  ATLTRACE2(traceOdDb, 2, L"Dimension Block Name: %s\n", (PCTSTR)Block->getName());
  ATLTRACE2(traceOdDb, 2, L"Position: %f, %f, %f\n", dimension->dimBlockPosition()[0], dimension->dimBlockPosition()[1],
            dimension->dimBlockPosition()[2]);
  ATLTRACE2(traceOdDb, 2, L"Rotation: %f\n", dimension->dimBlockRotation());
  ATLTRACE2(traceOdDb, 2, L"Scale: %f, %f, %f\n", dimension->dimBlockScale()[0], dimension->dimBlockScale()[1],
            dimension->dimBlockScale()[2]);
  ATLTRACE2(traceOdDb, 2, L"Text Position: %f, %f, %f\n", dimension->textPosition()[0], dimension->textPosition()[1],
            dimension->textPosition()[2]);
  ATLTRACE2(traceOdDb, 2, L"Text Rotation: %f\n", dimension->textRotation());
  ATLTRACE2(traceOdDb, 2, L"Dimension Style: %i\n", dimension->dimensionStyle());
  //OdCmColor bgrndTxtColor;
  //OdUInt16 bgrndTxtFlags = dimension->getBgrndTxtColor(bgrndTxtColor));
  //ATLTRACE2(traceOdDb, 2, L"Background Text Color: %i\n", bgrndTxtColor);
  //ATLTRACE2(traceOdDb, 2, L"BackgroundText Flags: %i\n", bgrndTxtFlags);
  ATLTRACE2(traceOdDb, 2, L"Extension Line 1 Linetype: %i\n", dimension->getDimExt1Linetype());
  ATLTRACE2(traceOdDb, 2, L"Extension Line 2 Linetype: %i\n", dimension->getDimExt2Linetype());
  ATLTRACE2(traceOdDb, 2, L"Dim Line Linetype: %i\n", dimension->getDimExt2Linetype());
  ATLTRACE2(traceOdDb, 2, L"Horizontal Rotation: %f\n", dimension->horizontalRotation());
  ATLTRACE2(traceOdDb, 2, L"Elevation: %f\n", dimension->elevation());
  ATLTRACE2(traceOdDb, 2, L"Normal: %f, %f, %f\n", dimension->normal()[0], dimension->normal()[1],
            dimension->normal()[2]);
};

void ConvertCurveData(OdDbEntity* entity, EoDbPrimitive* primitive) {
  OdDbCurvePtr Curve = entity;
  OdGePoint3d StartPoint;
  if (eOk == Curve->getStartPoint(StartPoint)) { ATLTRACE2(traceOdDb, 2, L"Start Point: %f, %f, %f\n", StartPoint); }
  OdGePoint3d EndPoint;
  if (eOk == Curve->getEndPoint(EndPoint)) { ATLTRACE2(traceOdDb, 2, L"End Point: %f, %f, %f\n", StartPoint); }
  if (Curve->isClosed()) static_cast<EoDbPolyline*>(primitive)->SetFlag(EoDbPolyline::sm_Closed);

  ATLTRACE2(traceOdDb, 2, L"Periodic: %i\n", Curve->isPeriodic());

  double Area;
  if (eOk == Curve->getArea(Area)) { ATLTRACE2(traceOdDb, 2, L"Area: %f\n", Area); }
  ConvertEntityData(entity, primitive);
}
void EoDbConvertEntityToPrimitive::Convert(OdDbEntity* entity, EoDbGroup*) {
  OdDbEntityPtr UnknownEntity = entity;
  ATLTRACE2(traceOdDb, 0, L"%s is unknown entity ...\n", (PCTSTR)UnknownEntity->desc()->name());
}
/// <summary>2 Line Angular Dimension Converter</summary>
class EoDb2LineAngularDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDb2LineAngularDimensionPtr AngularDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)AngularDimensionEntity->desc()->name());
  }
};
/// <summary>2D Polyline Converter</summary>
class EoDb2dPolyline_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDb2dPolylinePtr PolylineEntity = entity;

    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbPolyline ...\n", (PCTSTR)PolylineEntity->desc()->name());

    ATLTRACE2(traceOdDb, 2, L"Elevation: %f\n", PolylineEntity->elevation());
    ATLTRACE2(traceOdDb, 2, L"Normal: %f, %f, %f\n", PolylineEntity->normal());
    ATLTRACE2(traceOdDb, 2, L"Thickness: %f\n", PolylineEntity->thickness());

    EoGePoint3dArray pts;

    OdDbObjectIteratorPtr Iterator = PolylineEntity->vertexIterator();
    for (int i = 0; !Iterator->done(); i++, Iterator->step()) {
      OdDb2dVertexPtr Vertex = Iterator->entity();
      if (Vertex.get()) {
        OdGePoint3d Point(Vertex->position());
        Point.z = PolylineEntity->elevation();
        pts.Add(Point);
      }
    }
    EoDbPolyline* PolylinePrimitive = new EoDbPolyline(pts);
    if (PolylineEntity->isClosed()) PolylinePrimitive->SetFlag(EoDbPolyline::sm_Closed);

    ConvertCurveData(PolylineEntity, PolylinePrimitive);

    if (PolylineEntity->polyType() == OdDb::k2dCubicSplinePoly) {
      ATLTRACE2(traceOdDb, 2, L"Cubic spline polyline converted to simple polyline\n");
    } else if (PolylineEntity->polyType() == OdDb::k2dQuadSplinePoly) {
      ATLTRACE2(traceOdDb, 2, L"Quad spline polyline converted to simple polyline\n");
    }
    group->AddTail(PolylinePrimitive);
  }
};
/// <summary>3D Polyline Converter</summary>
class EoDb3dPolyline_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDb3dPolylinePtr PolylineEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbPolyline ...\n", (PCTSTR)PolylineEntity->desc()->name());

    EoGePoint3dArray pts;

    OdDbObjectIteratorPtr Iterator = PolylineEntity->vertexIterator();
    for (int i = 0; !Iterator->done(); i++, Iterator->step()) {
      OdDb3dPolylineVertexPtr Vertex = Iterator->entity();
      if (Vertex.get()) { pts.Add(Vertex->position()); }
    }
    EoDbPolyline* PolylinePrimitive = new EoDbPolyline(pts);
    if (PolylineEntity->isClosed()) PolylinePrimitive->SetFlag(EoDbPolyline::sm_Closed);

    if (PolylineEntity->polyType() == OdDb::k3dCubicSplinePoly) {
      ATLTRACE2(traceOdDb, 2, L"Cubic spline polyline converted to simple polyline\n");
    } else if (PolylineEntity->polyType() == OdDb::k3dQuadSplinePoly) {
      ATLTRACE2(traceOdDb, 2, L"Quad spline polyline converted to simple polyline\n");
    }
    ConvertCurveData(PolylineEntity, PolylinePrimitive);

    group->AddTail(PolylinePrimitive);
  }
};
/// <summary>3DSolid Converter</summary>
class EoDb3dSolid_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDb3dSolidPtr SolidEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)SolidEntity->desc()->name());
  }
};
/// <summary>3 Point Angular Dimension Converter</summary>
class EoDb3PointAngularDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDb3PointAngularDimensionPtr AngularDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)AngularDimensionEntity->desc()->name());
  }
};
/// <summary>Aligned Dimension Converter</summary>
class EoDbAlignedDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbAlignedDimensionPtr AlignedDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)AlignedDimensionEntity->desc()->name());
  }
};
/// <summary>Arc Converter</summary>
class EoDbArc_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbArcPtr ArcEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbEllipse ...\n", (PCTSTR)ArcEntity->desc()->name());

    EoGeVector3d Normal(ArcEntity->normal());
    EoGePoint3d Center(ArcEntity->center());

    double StartAngle = ArcEntity->startAngle();
    double EndAngle = ArcEntity->endAngle();

    if (StartAngle >= Eo::TwoPi) {  // need to rationalize angs to first period angles in range on (0 to twopi)
      StartAngle -= Eo::TwoPi;
      EndAngle -= Eo::TwoPi;
    }
    double SweepAngle = EndAngle - StartAngle;

    if (SweepAngle <= FLT_EPSILON) SweepAngle += Eo::TwoPi;

    OdGePoint3d StartPoint;
    ArcEntity->getStartPoint(StartPoint);

    EoGeVector3d MajorAxis = EoGeVector3d(Center, StartPoint);
    EoGeVector3d MinorAxis = EoGeCrossProduct(Normal, MajorAxis);
    EoDbEllipse* ArcPrimitive = new EoDbEllipse(Center, MajorAxis, MinorAxis, SweepAngle);
    ConvertEntityData(ArcEntity, ArcPrimitive);

    ConvertEntityData(ArcEntity, ArcPrimitive);
    group->AddTail(ArcPrimitive);
  }
};
/// <summary>Arc Aligned Text</summary>
class EoDbArcAlignedText_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbArcAlignedTextPtr ArcAlignedTextEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)ArcAlignedTextEntity->desc()->name());
  }
};
/// <summary>Arc Dimension Converter</summary>
class EoDbArcDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbArcDimensionPtr ArcDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)ArcDimensionEntity->desc()->name());
  }
};
/// <summary>Attribute Definition Converter</summary>
class EoDbAttributeDefinition_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbAttributeDefinitionPtr AttributeDefinitionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s with constant text converted to EoDbText ...\n",
              (PCTSTR)AttributeDefinitionEntity->desc()->name());
    ATLTRACE2(traceOdDb, 2, L"Tag: %s\n", (PCTSTR)AttributeDefinitionEntity->tag());

    if (AttributeDefinitionEntity->isConstant() && !AttributeDefinitionEntity->isInvisible()) {
      ConvertTextData(static_cast<OdDbText*>(entity), group);
    }
    //ATLTRACE2(traceOdDb, 2, L"Field Length: %s\n", (PCTSTR) attribute->fieldLength());
    //ATLTRACE2(traceOdDb, 2, L"Preset: %i\n", (PCTSTR) attribute->isPreset());
    //ATLTRACE2(traceOdDb, 2, L"Verifiable: %i\n", (PCTSTR) attribute->isVerifiable());
    //ATLTRACE2(traceOdDb, 2, L"Locked in Position: %i\n", (PCTSTR) attribute->lockPositionInBlock());
  }
};
/// <summary>Block Reference Converter</summary>
class EoDbBlockReference_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbBlockReferencePtr BlockReferenceEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbBlockReference ...\n", (PCTSTR)BlockReferenceEntity->desc()->name());

    OdDbBlockTableRecordPtr BlockTableRecordPtr = BlockReferenceEntity->blockTableRecord().safeOpenObject();

    EoDbBlockReference* SegRefPrimitive =
        new EoDbBlockReference((PCTSTR)BlockTableRecordPtr->getName(), BlockReferenceEntity->position());
    SegRefPrimitive->SetNormal(BlockReferenceEntity->normal());
    SegRefPrimitive->SetScaleFactors(BlockReferenceEntity->scaleFactors());
    SegRefPrimitive->SetRotation(BlockReferenceEntity->rotation());

    ConvertEntityData(BlockReferenceEntity, SegRefPrimitive);
    group->AddTail(SegRefPrimitive);

    // attributes
    OdDbObjectIteratorPtr ObjectIterator = BlockReferenceEntity->attributeIterator();
    for (int i = 0; !ObjectIterator->done(); i++, ObjectIterator->step()) {
      OdDbAttributePtr AttributePtr = ObjectIterator->entity();
      if (!AttributePtr.isNull()) {
        if (!AttributePtr->isConstant() && !AttributePtr->isInvisible()) {
          ConvertTextData(static_cast<OdDbText*>(AttributePtr), group);
        }
      }
    }
  }
};
/// <summary>Body Converter</summary>
class EoDbBody_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbBodyPtr BodyEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)BodyEntity->desc()->name());
  }
};
/// <summary>Circle Converter</summary>
class EoDbCircle_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbCirclePtr CircleEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbEllipse ...\n", (PCTSTR)CircleEntity->desc()->name());

    EoGeVector3d Normal(CircleEntity->normal());
    EoGeVector3d MajorAxis = ComputeArbitraryAxis(Normal);
    EoGeVector3d MinorAxis = EoGeCrossProduct(Normal, MajorAxis);
    MajorAxis *= CircleEntity->radius();
    MinorAxis *= CircleEntity->radius();

    EoDbEllipse* CirclePrimitive = new EoDbEllipse(CircleEntity->center(), MajorAxis, MinorAxis, Eo::TwoPi);

    ConvertEntityData(CircleEntity, CirclePrimitive);
    group->AddTail(CirclePrimitive);
  }
};
/// <summary>Diametric Dimension Converter</summary>
class EoDbDiametricDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbDiametricDimensionPtr DiametricDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)DiametricDimensionEntity->desc()->name());
  }
};
/// <summary>Ellipse Converter</summary>
class EoDbEllipse_Converter : public EoDbConvertEntityToPrimitive {
 public:
  /// <remarks>
  /// Can only properly convert ellipse which is radial (trival) or non radials which have a start parameter of 0.
  /// </remarks>
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbEllipsePtr EllipseEntity = entity;
    ATLTRACE2(traceOdDb, 1, L"Converting %s to EoDbEllipse ...\n", (PCTSTR)EllipseEntity->desc()->name());

    EoGeVector3d MajorAxis(EllipseEntity->majorAxis());
    EoGeVector3d MinorAxis(EllipseEntity->minorAxis());

    double StartAngle = EllipseEntity->startAngle();
    double EndAngle = EllipseEntity->endAngle();

    if (StartAngle >= Eo::TwoPi) {  // need to rationalize angs to first period angles in range on (0 to twopi)
      StartAngle -= Eo::TwoPi;
      EndAngle -= Eo::TwoPi;
    }
    double SweepAngle = EndAngle - StartAngle;
    if (SweepAngle <= FLT_EPSILON) SweepAngle += Eo::TwoPi;

    if (StartAngle != 0.0) {
      MajorAxis.RotAboutArbAx(EllipseEntity->normal(), StartAngle);
      MinorAxis.RotAboutArbAx(EllipseEntity->normal(), StartAngle);
      if (EllipseEntity->radiusRatio() != 1.0) {
        ATLTRACE2(traceOdDb, 2, L"Ellipse: Non radial with start parameter not 0.\n");
      }
    }
    EoDbEllipse* EllipsePrimitive = new EoDbEllipse(EllipseEntity->center(), MajorAxis, MinorAxis, SweepAngle);
    ConvertEntityData(EllipseEntity, EllipsePrimitive);
    group->AddTail(EllipsePrimitive);
  }
};
/// <summary>Face Converter</summary>
/// <remarks>
/// Four sided, not necessarily planar, surface. It hides other objects and fills with solid color.
/// No support for individual edge visibilty
/// </remarks>
class EoDbFace_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbFacePtr FaceEntity = entity;
    ATLTRACE2(traceOdDb, 1, L"Converting %s to EoDbPolygon ...\n", (PCTSTR)FaceEntity->desc()->name());

    OdGePoint3d Vertex;
    EoGePoint3dArray pts;
    pts.SetSize(4);
    for (OdUInt16 VertexIndex = 0; VertexIndex < 4; VertexIndex++) {
      FaceEntity->getVertexAt(VertexIndex, Vertex);
      pts[VertexIndex] = Vertex;
    }
    EoDbPolygon* PolygonPrimitive = new EoDbPolygon(pts);
    PolygonPrimitive->SetIntStyle(EoDb::kHollow);
    PolygonPrimitive->SetIntStyleId(0);

    ConvertEntityData(FaceEntity, PolygonPrimitive);
    group->AddTail(PolygonPrimitive);
  }
};
/// <summary>FCF Converter</summary>
class EoDbFcf_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbFcfPtr FcfEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)FcfEntity->desc()->name());
  }
};
/// <summary>Hatch Converter</summary>
class EoDbHatch_Converter : public EoDbConvertEntityToPrimitive {
 private:
  static void ConvertPolylineType(int loopIndex, OdDbHatchPtr& HatchEntity) {
    OdGePoint2dArray vertices;
    OdGeDoubleArray bulges;
    HatchEntity->getLoopAt(loopIndex, vertices, bulges);
    bool hasBulges = (bulges.size() > 0);
    for (int i = 0; i < (int)vertices.size(); i++) {
      ATLTRACE2(traceOdDb, 2, L"Vertex %f, %f\n", vertices[i]);
      if (hasBulges) {
        ATLTRACE2(traceOdDb, 2, L"Bulge %f\n", bulges[i]);
        ATLTRACE2(traceOdDb, 2, L"Bulge angle %f\n", (4 * atan(bulges[i])));
      }
    }
  }
  static void ConvertCircularArcEdge(OdGeCurve2d* pEdge) {
    OdGeCircArc2d* pCircArc = (OdGeCircArc2d*)pEdge;
    ATLTRACE2(traceOdDb, 2, L"Center: %f, %f\n", pCircArc->center());
    ATLTRACE2(traceOdDb, 2, L"Radius: %f\n", pCircArc->radius());
    ATLTRACE2(traceOdDb, 2, L"Start Angle %f\n", pCircArc->startAng());
    ATLTRACE2(traceOdDb, 2, L"End Angle: %f\n", pCircArc->endAng());
    ATLTRACE2(traceOdDb, 2, L"Clockwise: %i\n", pCircArc->isClockWise());
  }
  static void ConvertEllipticalArcEdge(OdGeCurve2d* pEdge) {
    OdGeEllipArc2d* pEllipArc = (OdGeEllipArc2d*)pEdge;
    ATLTRACE2(traceOdDb, 2, L"Center: %f, %f\n", pEllipArc->center()[0], pEllipArc->center()[1]);
    ATLTRACE2(traceOdDb, 2, L"Major Radius: %f\n", pEllipArc->majorRadius());
    ATLTRACE2(traceOdDb, 2, L"Minor Radius: %f\n", pEllipArc->minorRadius());
    ATLTRACE2(traceOdDb, 2, L"Major Axis: %f\n", pEllipArc->majorAxis());
    ATLTRACE2(traceOdDb, 2, L"Minor Axis: %f, %f\n", pEllipArc->minorAxis());
    ATLTRACE2(traceOdDb, 2, L"Start Angle: %f\n", pEllipArc->startAng());
    ATLTRACE2(traceOdDb, 2, L"End Angle: %f\n", pEllipArc->endAng());
    ATLTRACE2(traceOdDb, 2, L"Clockwise:%i\n", pEllipArc->isClockWise());
  }
  static void ConvertNurbCurveEdge(OdGeCurve2d* pEdge) {
    OdGeNurbCurve2d* pNurbCurve = (OdGeNurbCurve2d*)pEdge;
    int Degree;
    bool Rational, Periodic;
    OdGePoint2dArray ControlPoints;
    OdGeDoubleArray Weights;
    OdGeKnotVector Knots;

    pNurbCurve->getDefinitionData(Degree, Rational, Periodic, Knots, ControlPoints, Weights);
    ATLTRACE2(traceOdDb, 2, L"Degree: %i\n", Degree);
    ATLTRACE2(traceOdDb, 2, L"Rational: %i\n", Rational);
    ATLTRACE2(traceOdDb, 2, L"Periodic: %i\n", Periodic);

    ATLTRACE2(traceOdDb, 2, L"Number of Control Points: %i\n", (int)ControlPoints.size());
    int i;
    for (i = 0; i < (int)ControlPoints.size(); i++) {
      ATLTRACE2(traceOdDb, 2, L"Control Point: %f, %f", ControlPoints[i][0], ControlPoints[i][1]);
    }
    ATLTRACE2(traceOdDb, 2, L"Number of Knots: %i\n", Knots.length());
    for (i = 0; i < Knots.length(); i++) { ATLTRACE2(traceOdDb, 2, L"Knot: %f\n", Knots[i]); }
    if (Rational) {
      ATLTRACE2(traceOdDb, 2, L"Number of Weights: %i\n", (int)Weights.size());
      for (i = 0; i < (int)Weights.size(); i++) { ATLTRACE2(traceOdDb, 2, L"Weight: %f\n", Weights[i]); }
    }
  }
  static void ConvertEdgesType(int loopIndex, OdDbHatchPtr& HatchEntity) {
    EdgeArray edges;
    HatchEntity->getLoopAt(loopIndex, edges);
    for (int i = 0; i < (int)edges.size(); i++) {
      OdGeCurve2d* pEdge = edges[i];
      ATLTRACE2(traceOdDb, 2, L"Edge %i\n", pEdge->type());
      switch (pEdge->type()) {
        case OdGe::kLineSeg2d:
          break;
        case OdGe::kCircArc2d:
          ConvertCircularArcEdge(pEdge);
          break;
        case OdGe::kEllipArc2d:
          ConvertEllipticalArcEdge(pEdge);
          break;
          // case OdGe::kNurbCurve2d : ConvertNurbCurveEdge(pEdge);
          // break;
      }

      // Common Edge Properties
      OdGeInterval interval;
      pEdge->getInterval(interval);
      double lower;
      double upper;
      interval.getBounds(lower, upper);
      ATLTRACE2(traceOdDb, 2, L"Start Point: %f, %f\n", pEdge->evalPoint(lower));
      ATLTRACE2(traceOdDb, 2, L"End Point: %f, %f\n", pEdge->evalPoint(upper));
      ATLTRACE2(traceOdDb, 2, L"Closed: %i\n", pEdge->isClosed());
    }
  }

 public:
  void Convert(OdDbEntity* entity, EoDbGroup* /* group */) {
    OdDbHatchPtr HatchEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)HatchEntity->desc()->name());

    ATLTRACE2(traceOdDb, 2, L"Hatch Style: %i\n", HatchEntity->hatchStyle());
    ATLTRACE2(traceOdDb, 2, L"Hatch Object Type: %i\n", HatchEntity->hatchObjectType());
    if (HatchEntity->isHatch()) {
      ATLTRACE2(traceOdDb, 2, L"Pattern Type: %i\n", HatchEntity->patternType());
      switch (HatchEntity->patternType()) {
        case OdDbHatch::kPreDefined:
        case OdDbHatch::kCustomDefined:
          ATLTRACE2(traceOdDb, 2, L"Pattern Name: %s\n", (PCTSTR)HatchEntity->patternName());
          ATLTRACE2(traceOdDb, 2, L"Solid Fill: %i", HatchEntity->isSolidFill());
          if (!HatchEntity->isSolidFill()) {
            ATLTRACE2(traceOdDb, 2, L"Pattern Angle: %f\n", HatchEntity->patternAngle());
            ATLTRACE2(traceOdDb, 2, L"Pattern Scale: %f\n", HatchEntity->patternScale());
          }
          break;
        case OdDbHatch::kUserDefined:
          ATLTRACE2(traceOdDb, 2, L"Pattern Angle: %f\n", HatchEntity->patternAngle());
          ATLTRACE2(traceOdDb, 2, L"Pattern Double: %f\n", HatchEntity->patternDouble());
          ATLTRACE2(traceOdDb, 2, L"Pattern Space: %f\n", HatchEntity->patternSpace());
          break;
      }
    }
    if (HatchEntity->isGradient()) {
      // Dump Gradient Parameters
      ATLTRACE2(traceOdDb, 2, L"Gradient Type: %i\n", HatchEntity->gradientType());
      ATLTRACE2(traceOdDb, 2, L"Gradient Name: %s\n", HatchEntity->gradientName());
      ATLTRACE2(traceOdDb, 2, L"Gradient Angle: %f\n", HatchEntity->gradientAngle());
      ATLTRACE2(traceOdDb, 2, L"Gradient Shift: %f\n", HatchEntity->gradientShift());
      ATLTRACE2(traceOdDb, 2, L"Gradient One-Color Mode: %i\n", HatchEntity->getGradientOneColorMode());
      if (HatchEntity->getGradientOneColorMode()) {
        ATLTRACE2(traceOdDb, 2, L"ShadeTintValue: %f\n", HatchEntity->getShadeTintValue());
      }
      OdCmColorArray colors;
      OdGeDoubleArray values;
      HatchEntity->getGradientColors(colors, values);
      for (int i = 0; i < (int)colors.size(); i++) {
        ATLTRACE2(traceOdDb, 2, L"Color:         %f", colors[i]);
        ATLTRACE2(traceOdDb, 2, L"Interpolation: %f", values[i]);
      }
    }
    // Dump Associated Objects
    ATLTRACE2(traceOdDb, 2, L"Associated objects: %i\n", HatchEntity->associative());
    OdDbObjectIdArray assocIds;
    HatchEntity->getAssocObjIds(assocIds);
    int i;
    for (i = 0; i < (int)assocIds.size(); i++) {
      OdDbEntityPtr pAssoc = assocIds[i].safeOpenObject();
      // pAssoc->isA(),      pAssoc->getDbHandle();
    }
    // Dump Seed Points
    ATLTRACE2(traceOdDb, 2, L"Seed points: %i\n", HatchEntity->numSeedPoints());
    for (i = 0; i < HatchEntity->numSeedPoints(); i++) {
      ATLTRACE2(traceOdDb, 2, L"Seed point %f, %f\n", HatchEntity->getSeedPointAt(i)[0],
                HatchEntity->getSeedPointAt(i)[1]);
    }
    // Dump Loops
    ATLTRACE2(traceOdDb, 2, L"Loops: %i\n", HatchEntity->numLoops());
    for (i = 0; i < HatchEntity->numLoops(); i++) {
      //ATLTRACE2(traceOdDb, 2, L"Loop %f", toLooptypeString((int) HatchEntity->loopTypeAt(i)));

      // Dump Loop
      if (HatchEntity->loopTypeAt(i) & OdDbHatch::kPolyline) {
        ConvertPolylineType(i, HatchEntity);
      } else {
        ConvertEdgesType(i, HatchEntity);
      }
      // Dump Associated Objects
      if (HatchEntity->associative()) {
        assocIds.clear();
        HatchEntity->getAssocObjIdsAt(i, assocIds);
        for (int j = 0; j < (int)assocIds.size(); j++) {
          OdDbEntityPtr pAssoc = assocIds[j].safeOpenObject();
          // pAssoc->isA(),      pAssoc->getDbHandle();
        }
      }
    }
    ATLTRACE2(traceOdDb, 2, L"Elevation: %f\n", HatchEntity->elevation());
    ATLTRACE2(traceOdDb, 2, L"Normal: %f, %f, %f\n", HatchEntity->normal()[0], HatchEntity->normal()[1],
              HatchEntity->normal()[2]);
  }
};
/// <summary>Leader Converter</summary>
class EoDbLeader_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbLeaderPtr LeaderEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to primitive set ...\n", (PCTSTR)LeaderEntity->desc()->name());

    OdRxObjectPtrArray EntitySet;
    LeaderEntity->explode(EntitySet);
    int NumberOfEntities = EntitySet.size();
    for (int i = 0; i < NumberOfEntities; i++) {
      OdDbEntityPtr Entity = static_cast<OdDbEntityPtr>(EntitySet[i]);
      OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
      EntityConverter->Convert(Entity, group);
    }
  }
};
/// <summary>Line Converter</summary>
class EoDbLine_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbLinePtr LineEntity = entity;
    ATLTRACE2(traceOdDb, 1, L"Converting %s to EoDbLine ...\n", (PCTSTR)LineEntity->desc()->name());

    EoDbLine* LinePrimitive = new EoDbLine(LineEntity->startPoint(), LineEntity->endPoint());

    ConvertEntityData(LineEntity, LinePrimitive);
    group->AddTail(LinePrimitive);
  }
};
/// <summary>MInsertBlock Converter</summary>
class EoDbMInsertBlock_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbMInsertBlockPtr MInsertBlockEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbBlockReference ...\n", (PCTSTR)MInsertBlockEntity->desc()->name());

    OdDbBlockTableRecordPtr BlockTableRecordPtr = MInsertBlockEntity->blockTableRecord().safeOpenObject();

    EoDbBlockReference* SegRefPrimitive =
        new EoDbBlockReference((PCTSTR)BlockTableRecordPtr->getName(), MInsertBlockEntity->position());
    SegRefPrimitive->SetNormal(MInsertBlockEntity->normal());
    SegRefPrimitive->SetScaleFactors(MInsertBlockEntity->scaleFactors());
    SegRefPrimitive->SetRotation(MInsertBlockEntity->rotation());

    SegRefPrimitive->SetRows(MInsertBlockEntity->rows());
    SegRefPrimitive->SetRowSpacing(MInsertBlockEntity->rowSpacing());
    SegRefPrimitive->SetColumns(MInsertBlockEntity->columns());
    SegRefPrimitive->SetColumnSpacing(MInsertBlockEntity->columnSpacing());

    ConvertEntityData(MInsertBlockEntity, SegRefPrimitive);
    group->AddTail(SegRefPrimitive);
  }
};
/// <summary>Mline Converter</summary>
class EoDbMline_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbMlinePtr MlineEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)MlineEntity->desc()->name());
  }
};
/// <summary>MText Converter</summary>
class EoDbMText_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbMTextPtr MTextEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbText ...\n", (PCTSTR)MTextEntity->desc()->name());

    OdDbObjectId TextStyleObjectId = MTextEntity->textStyle();
    OdDbTextStyleTableRecordPtr TextStyleTableRecordPtr = TextStyleObjectId.safeOpenObject();
    OdString FileName("Standard");
    if (TextStyleTableRecordPtr->isShapeFile()) {
      ATLTRACE2(traceOdDb, 2, L"TextStyle references shape library.\n",
                (PCTSTR)TextStyleTableRecordPtr->desc()->name());
    } else {
      FileName = TextStyleTableRecordPtr->fileName();
      int nExt = FileName.reverseFind('.');
      if (nExt != -1) {
        if (FileName.mid(nExt).compare(".shx") == 0) {
          FileName = FileName.left(nExt);
          for (int n = nExt; n < 8; n++) { FileName += '_'; }
          FileName += L".ttf";
        }
      }
    }
    OdGePoint3d Location = MTextEntity->location();
    //double Width = MTextEntity->width();

    EoUInt16 HorizontalAlignment;
    EoUInt16 VerticalAlignment;
    switch (MTextEntity->attachment()) {
      case OdDb::kTopLeft:
        HorizontalAlignment = EoDb::kAlignLeft;
        VerticalAlignment = EoDb::kAlignTop;
        break;
      case OdDb::kTopCenter:
        HorizontalAlignment = EoDb::kAlignCenter;
        VerticalAlignment = EoDb::kAlignTop;
        break;
      case OdDb::kTopRight:
        HorizontalAlignment = EoDb::kAlignRight;
        VerticalAlignment = EoDb::kAlignTop;
        break;
      case OdDb::kMiddleLeft:
        HorizontalAlignment = EoDb::kAlignLeft;
        VerticalAlignment = EoDb::kAlignMiddle;
        break;
      case OdDb::kMiddleCenter:
        HorizontalAlignment = EoDb::kAlignCenter;
        VerticalAlignment = EoDb::kAlignMiddle;
        break;
      case OdDb::kMiddleRight:
        HorizontalAlignment = EoDb::kAlignRight;
        VerticalAlignment = EoDb::kAlignMiddle;
        break;
      case OdDb::kBottomCenter:
        HorizontalAlignment = EoDb::kAlignCenter;
        VerticalAlignment = EoDb::kAlignBottom;
        break;
      case OdDb::kBottomRight:
        HorizontalAlignment = EoDb::kAlignRight;
        VerticalAlignment = EoDb::kAlignBottom;
        break;
      default:
        HorizontalAlignment = EoDb::kAlignLeft;
        VerticalAlignment = EoDb::kAlignBottom;
    }
    OdString Contents = MTextEntity->contents();

    EoDbFontDefinition FontDefinition(EoDb::kEoTrueType, (PCTSTR)FileName, EoDb::kPathRight, HorizontalAlignment,
                                      VerticalAlignment, 0.0);

    EoDbCharacterCellDefinition ccd(MTextEntity->rotation(), 0.0, 1.0, MTextEntity->textHeight());

    EoGeVector3d XDirection;
    EoGeVector3d YDirection;
    CharCellDef_EncdRefSys(MTextEntity->normal(), ccd, XDirection, YDirection);

    EoGeReferenceSystem ReferenceSystem(MTextEntity->location(), XDirection, YDirection);
    EoDbText* TextPrimitive = new EoDbText(FontDefinition, ReferenceSystem, (PCTSTR)Contents);

    ConvertEntityData(MTextEntity, TextPrimitive);
    group->AddTail(TextPrimitive);
  }
};
/// <summary>Ordinate Dimension Converter</summary>
class EoDbOrdinateDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbOrdinateDimensionPtr OrdinateDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)OrdinateDimensionEntity->desc()->name());
  }
};
/// <summary>PolyFaceMesh Converter</summary>
class EoDbPolyFaceMesh_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbPolyFaceMeshPtr PolyFaceMeshEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)PolyFaceMeshEntity->desc()->name());
  }
};
/// <summary>Ole2Frame</summary>
class EoDbOle2Frame_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbOle2FramePtr Ole2FrameEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)Ole2FrameEntity->desc()->name());
  }
};
/// <summary>Point Converter</summary>
class EoDbPoint_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbPointPtr PointEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbPoint ...\n", (PCTSTR)PointEntity->desc()->name());

    EoDbPoint* PointPrimitive = new EoDbPoint(PointEntity->position());
    PointPrimitive->PointStyle() = PointEntity->database()->getPDMODE();

    ConvertEntityData(PointEntity, PointPrimitive);
    group->AddTail(PointPrimitive);
  }
};
/// <summary>Polygon Mesh Converter</summary>
class EoDbPolygonMesh_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbPolygonMeshPtr PolygonMeshEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)PolygonMeshEntity->desc()->name());
  }
};
/// <summary>Polyline Converter</summary>
/// <remarks>
///The polyline verticies are not properly transformed from ECS to WCS. Arcs and wide polylines are
/// note realized at all.
/// </remarks>
class EoDbPolyline_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbPolylinePtr PolylineEntity = entity;

    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbPolyline ...\n", (PCTSTR)PolylineEntity->desc()->name());

    EoGeVector3d Normal(PolylineEntity->normal());
    //double Elevation = PolylineEntity->elevation();
    int NumberOfVerticies = PolylineEntity->numVerts();

    EoGePoint3dArray pts;
    pts.SetSize(NumberOfVerticies);

    for (int n = 0; n < NumberOfVerticies; n++) {
      OdGePoint3d Point;
      PolylineEntity->getPointAt(n, Point);
      pts[n] = Point;
    }
    EoDbPolyline* PolylinePrimitive = new EoDbPolyline(pts);
    if (PolylineEntity->isClosed()) { PolylinePrimitive->SetFlag(EoDbPolyline::sm_Closed); }
    if (PolylineEntity->hasBulges()) {
      ATLTRACE2(traceOdDb, 2, L"Polyline: At least one of the groups has a non zero bulge\n");
    }
    if (PolylineEntity->hasWidth()) {
      if (PolylineEntity->getConstantWidth()) {
        ATLTRACE2(traceOdDb, 2, L"Polyline: At least one of the groups has a constant start and end width\n");
      } else {
        ATLTRACE2(traceOdDb, 2, L"Polyline: At least one of the groups has a different start and end width\n");
      }
    }

    ConvertEntityData(PolylineEntity, PolylinePrimitive);
    group->AddTail(PolylinePrimitive);
  }
};
/// <summary>Proxy Entity Converter</summary>
class EoDbProxyEntity_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbProxyEntityPtr ProxyEntityEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to ", (PCTSTR)ProxyEntityEntity->desc()->name());

    ATLTRACE2(traceOdDb, 0, L"Graphics Metafile type: ");
    if (ProxyEntityEntity->graphicsMetafileType() == ProxyEntityEntity->kNoMetafile) {
      ATLTRACE2(traceOdDb, 0, L"No Metafile\n");
    } else {
      if (ProxyEntityEntity->graphicsMetafileType() == ProxyEntityEntity->kBoundingBox) {
        ATLTRACE2(traceOdDb, 0, L"Bounding Box\n");
      } else if (ProxyEntityEntity->graphicsMetafileType() == ProxyEntityEntity->kFullGraphics) {
        ATLTRACE2(traceOdDb, 0, L"Full Graphics\n");
      }
      OdRxObjectPtrArray EntitySet;
      ProxyEntityEntity->explodeGeometry(EntitySet);
      int NumberOfEntities = EntitySet.size();
      for (int n = 0; n < NumberOfEntities; n++) {
        OdDbEntityPtr Entity = static_cast<OdDbEntityPtr>(EntitySet[n]);
        OdSmartPtr<EoDbConvertEntityToPrimitive> EntityConverter = Entity;
        EntityConverter->Convert(Entity, group);
      }
    }
    ATLTRACE2(traceOdDb, 2, L"Application Description: %s\n", (PCTSTR)ProxyEntityEntity->applicationDescription());
    ATLTRACE2(traceOdDb, 2, L"Original class name: %s\n", (PCTSTR)ProxyEntityEntity->originalClassName());

    OdAnsiString satString;
    ATLTRACE2(traceOdDb, 2, L"Proxy Sat: %s\n", (PCTSTR)odGetSatFromProxy(ProxyEntityEntity, satString));

    ATLTRACE2(traceOdDb, 2, L"Proxy Flags: %i\n", ProxyEntityEntity->proxyFlags());
    ATLTRACE2(traceOdDb, 2, L"Erase Allowed: %i\n", ProxyEntityEntity->eraseAllowed());
    ATLTRACE2(traceOdDb, 2, L"Transform Allowed: %i\n", ProxyEntityEntity->transformAllowed());
    ATLTRACE2(traceOdDb, 2, L"Color Change Allowed: %i\n", ProxyEntityEntity->colorChangeAllowed());
    ATLTRACE2(traceOdDb, 2, L"Layer Change Allowed: %i\n", ProxyEntityEntity->layerChangeAllowed());
    ATLTRACE2(traceOdDb, 2, L"Linetype Change Allowed: %i\n", ProxyEntityEntity->linetypeChangeAllowed());
    ATLTRACE2(traceOdDb, 2, L"Linetype Scale Change Allowed: %i\n", ProxyEntityEntity->linetypeScaleChangeAllowed());
    ATLTRACE2(traceOdDb, 2, L"Visibility Change Allowed: %i\n", ProxyEntityEntity->visibilityChangeAllowed());
    ATLTRACE2(traceOdDb, 2, L"Cloning Allowed: %i\n", ProxyEntityEntity->cloningAllowed());
    ATLTRACE2(traceOdDb, 2, L"Line Weight Change Allowed: %i\n", ProxyEntityEntity->lineWeightChangeAllowed());
    ATLTRACE2(traceOdDb, 2, L"Plot Style Name Change Allowed: %i\n", ProxyEntityEntity->plotStyleNameChangeAllowed());
  }
};
/// <summary>Radial Dimension Converter</summary>
class EoDbRadialDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbRadialDimensionPtr RadialDimensionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)RadialDimensionEntity->desc()->name());
  }
};
/// <summary>Raster Image Converter</summary>
class EoDbRasterImage_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbRasterImagePtr RasterImageEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)RasterImageEntity->desc()->name());
  }
};
/// <summary>Ray Converter</summary>
class EoDbRay_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbRayPtr RayEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)RayEntity->desc()->name());
  }
};
/// <summary>Region Converter</summary>
class EoDbRegion_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbRegionPtr RegionEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)RegionEntity->desc()->name());
  }
};
/// <summary>Rotated Dimension Converter</summary>
class EoDbRotatedDimension_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbRotatedDimensionPtr RotatedDimensionEntity = entity;
    OdDbBlockTableRecordPtr Block = RotatedDimensionEntity->dimBlockId().safeOpenObject();

    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbBlockReference of block %s  ...\n",
              (PCTSTR)RotatedDimensionEntity->desc()->name(), (PCTSTR)Block->getName());

    ATLTRACE2(traceOdDb, 2, L"Dimension Line Point: %f, %f, %f\n", RotatedDimensionEntity->dimLinePoint());
    ATLTRACE2(traceOdDb, 2, L"Oblique: %f\n", RotatedDimensionEntity->oblique());
    ATLTRACE2(traceOdDb, 2, L"Rotation: %f\n", RotatedDimensionEntity->rotation());
    ATLTRACE2(traceOdDb, 2, L"Extension Line 1 Point: %f, %f, %f\n", RotatedDimensionEntity->xLine1Point());
    ATLTRACE2(traceOdDb, 2, L"Extension Line 2 Point: %f, %f, %f\n", RotatedDimensionEntity->xLine2Point());
    //ConvertDimensionData(RotatedDimensionEntity);

    ATLTRACE2(traceOdDb, 2, L"Dimension Block Name: %s\n", (PCTSTR)Block->getName());

    EoDbBlockReference* SegRefPrimitive = new EoDbBlockReference((PCTSTR)Block->getName(), OdGePoint3d::kOrigin);
    SegRefPrimitive->SetNormal(OdGeVector3d::kZAxis);
    SegRefPrimitive->SetScaleFactors(OdGeScale3d::kIdentity);
    SegRefPrimitive->SetRotation(0.0);

    ConvertEntityData(RotatedDimensionEntity, SegRefPrimitive);
    group->AddTail(SegRefPrimitive);
  }
};
/// <summary>Shape Converter</summary>
class EoDbShape_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbShapePtr ShapeEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)ShapeEntity->desc()->name());
  }
};
/// <summary>Solid Converter</summary>
/// <remarks>
/// The first two points define one edge of the polygon.
/// The third point is diagonally opposite the second
/// If the fourth point coincides with third result is a filled triangle.
/// else fourth point creates a quadrilateral area.
/// </remarks>
class EoDbSolid_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbSolidPtr SolidEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbPolygon ...\n", (PCTSTR)SolidEntity->desc()->name());

    OdGePoint3d Point;
    EoGePoint3dArray pts;
    pts.SetSize(4);
    for (OdUInt16 PointIndex = 0; PointIndex < 4; PointIndex++) {
      SolidEntity->getPointAt(PointIndex, Point);
      pts[PointIndex] = Point;
    }
    EoDbPolygon* PolygonPrimitive = new EoDbPolygon(pts);
    PolygonPrimitive->SetIntStyle(EoDb::kSolid);
    PolygonPrimitive->SetIntStyleId(0);

    ConvertEntityData(SolidEntity, PolygonPrimitive);
    group->AddTail(PolygonPrimitive);
  }
};
/// <summary>Spline Converter</summary>
class EoDbSpline_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbSplinePtr SplineEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbSpline ...\n", (PCTSTR)SplineEntity->desc()->name());

    int Degree;
    bool Rational;
    bool Closed;
    bool Periodic;
    OdGePoint3dArray ControlPoints;
    OdGeDoubleArray Weights;
    OdGeDoubleArray Knots;
    double Tolerance;
    double KTolerance;

    SplineEntity->getNurbsData(Degree, Rational, Closed, Periodic, ControlPoints, Knots, Weights, Tolerance,
                               KTolerance);

    EoGePoint3dArray pts;

    ATLTRACE2(traceOdDb, 2, L"Degree: %i\n", Degree);
    ATLTRACE2(traceOdDb, 2, L"Rational: %i\n", Rational);
    ATLTRACE2(traceOdDb, 2, L"Periodic: %i\n", Periodic);
    ATLTRACE2(traceOdDb, 2, L"Control Point Tolerance: %f\n", Tolerance);
    ATLTRACE2(traceOdDb, 2, L"Knot Tolerance: %f\n", KTolerance);

    ATLTRACE2(traceOdDb, 2, L"Number of control points: %i\n", ControlPoints.size());
    for (EoUInt16 n = 0; n < ControlPoints.size(); n++) {
      ATLTRACE2(traceOdDb, 2, L"Control Point: %f, %f, %f\n", ControlPoints[n]);
      pts.Add(ControlPoints[n]);
    }
    ATLTRACE2(traceOdDb, 2, L"Number of Knots: %i\n", Knots.length());
    for (EoUInt16 n = 0; n < Knots.length(); n++) { ATLTRACE2(traceOdDb, 2, L"Knot: %f\n", Knots[n]); }
    if (Rational) {
      ATLTRACE2(traceOdDb, 2, L"Number of Weights: %i\n", Weights.size());
      for (EoUInt16 n = 0; n < Weights.size(); n++) { ATLTRACE2(traceOdDb, 2, L"Weight: %f\n", Weights[n]); }
    }
    EoDbSpline* BSplinePrimitive = new EoDbSpline(pts);
    ConvertCurveData(entity, BSplinePrimitive);
    group->AddTail(BSplinePrimitive);
  }
};
/// <summary>Table Converter</summary>
class EoDbTable_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbTablePtr TableEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)TableEntity->desc()->name());
  }
};
/// <summary>Text Converter</summary>
class EoDbText_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbTextPtr TextEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbText ...\n", (PCTSTR)TextEntity->desc()->name());

    OdDbObjectId TextStyleObjectId = TextEntity->textStyle();
    OdDbTextStyleTableRecordPtr TextStyleTableRecordPtr = TextStyleObjectId.safeOpenObject();
    OdString FileName("Standard");
    if (TextStyleTableRecordPtr->isShapeFile()) {
      ATLTRACE2(traceOdDb, 2, L"TextStyle references shape library %s.\n",
                (PCTSTR)TextStyleTableRecordPtr->desc()->name());
    } else {
      FileName = TextStyleTableRecordPtr->fileName();
      int nExt = FileName.reverseFind('.');
      if (nExt != -1) {
        if (FileName.mid(nExt).compare(L".shx") == 0) {
          FileName = FileName.left(nExt);
          for (int n = nExt; n < 8; n++) { FileName += '_'; }
          FileName += L".ttf";
        }
      }
    }
    EoUInt16 HorizontalAlignment;
    EoUInt16 VerticalAlignment;
    switch (TextEntity->verticalMode()) {
      case OdDb::kTextVertMid:
        VerticalAlignment = EoDb::kAlignMiddle;
        break;

      case OdDb::kTextTop:
        VerticalAlignment = EoDb::kAlignTop;
        break;

      default:  // OdDb::kTextBottom & OdDb::kTextBase
        VerticalAlignment = EoDb::kAlignBottom;
    }
    switch (TextEntity->horizontalMode()) {
      case OdDb::kTextMid:
      case OdDb::kTextCenter:
        HorizontalAlignment = EoDb::kAlignCenter;
        break;

      case OdDb::kTextRight:
      case OdDb::kTextAlign:
      case OdDb::kTextFit:
        HorizontalAlignment = EoDb::kAlignRight;
        break;

      default:  // OdDb::kTextLeft
        HorizontalAlignment = EoDb::kAlignLeft;
    }
    OdGePoint3d AlignmentPoint = TextEntity->position();
    if (HorizontalAlignment != EoDb::kAlignLeft || VerticalAlignment != EoDb::kAlignBottom)
      AlignmentPoint = TextEntity->alignmentPoint();

    EoDbFontDefinition FontDefinition(EoDb::kEoTrueType, (PCTSTR)FileName, EoDb::kPathRight, HorizontalAlignment,
                                      VerticalAlignment, 0.0);

    EoDbCharacterCellDefinition ccd(TextEntity->rotation(), TextEntity->oblique(), TextEntity->widthFactor(),
                                    TextEntity->height());

    EoGeVector3d XDirection;
    EoGeVector3d YDirection;
    CharCellDef_EncdRefSys(TextEntity->normal(), ccd, XDirection, YDirection);

    EoGeReferenceSystem ReferenceSystem(AlignmentPoint, XDirection, YDirection);
    EoDbText* TextPrimitive = new EoDbText(FontDefinition, ReferenceSystem, (PCTSTR)TextEntity->textString());

    ConvertEntityData(TextEntity, TextPrimitive);
    group->AddTail(TextPrimitive);
  }
};
/// <summary>Trace Converter</summary>
/// <remarks>
/// A Trace entity is the exactsame thing as a Solid entity
/// </remarks>
class EoDbTrace_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* group) {
    OdDbTracePtr TraceEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"Converting %s to EoDbPolygon ...\n", (PCTSTR)TraceEntity->desc()->name());

    OdGePoint3d Point;
    EoGePoint3dArray pts;
    pts.SetSize(4);
    for (OdUInt16 PointIndex = 0; PointIndex < 4; PointIndex++) {
      TraceEntity->getPointAt(PointIndex, Point);
      pts[PointIndex] = Point;
    }
    EoDbPolygon* PolygonPrimitive = new EoDbPolygon(pts);
    PolygonPrimitive->SetIntStyle(EoDb::kSolid);
    PolygonPrimitive->SetIntStyleId(0);

    ConvertEntityData(TraceEntity, PolygonPrimitive);
    group->AddTail(PolygonPrimitive);
  }
};
/// <summary>Viewport Converter</summary>
class EoDbViewport_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* /* group */) {
    OdDbViewportPtr ViewportEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)ViewportEntity->desc()->name());

    ATLTRACE2(traceOdDb, 2, L"Back Clip Distance: %f\n", ViewportEntity->backClipDistance());
    ATLTRACE2(traceOdDb, 2, L"Back Clip On: %i\n", ViewportEntity->isBackClipOn());
    ATLTRACE2(traceOdDb, 2, L"Center Point: %f, %f, %f\n", ViewportEntity->centerPoint());
    ATLTRACE2(traceOdDb, 2, L"Circle sides: %i\n", ViewportEntity->circleSides());
    ATLTRACE2(traceOdDb, 2, L"Custom Scale: %f\n", ViewportEntity->customScale());
    ATLTRACE2(traceOdDb, 2, L"Elevation: %f\n", ViewportEntity->elevation());
    ATLTRACE2(traceOdDb, 2, L"Front Clip at Eye: %i\n", ViewportEntity->isFrontClipAtEyeOn());
    ATLTRACE2(traceOdDb, 2, L"Front Clip Distance: %f\n", ViewportEntity->frontClipDistance());
    ATLTRACE2(traceOdDb, 2, L"Front Clip On: %i\n", ViewportEntity->isFrontClipOn());
    ATLTRACE2(traceOdDb, 2, L"Plot style sheet: %i\n", ViewportEntity->effectivePlotStyleSheet());

    OdDbObjectIdArray layerIds;
    ViewportEntity->getFrozenLayerList(layerIds);
    if (layerIds.length()) {
      ATLTRACE2(traceOdDb, 2, L"Frozen Layers:");
      for (int i = 0; i < (int)layerIds.length(); i++) { ATLTRACE2(traceOdDb, 2, L"%i  ", layerIds[i]); }
    } else {
      ATLTRACE2(traceOdDb, 2, L"Frozen Layers: None\n");
    }

    OdGePoint3d origin;
    OdGeVector3d xAxis;
    OdGeVector3d yAxis;
    ViewportEntity->getUcs(origin, xAxis, yAxis);
    ATLTRACE2(traceOdDb, 2, L"UCS origin: %f, %f, %f\n", origin);
    ATLTRACE2(traceOdDb, 2, L"UCS x-Axis: %f, %f, %f\n", xAxis);
    ATLTRACE2(traceOdDb, 2, L"UCS y-Axis: %f, %f, %f\n", yAxis);
    ATLTRACE2(traceOdDb, 2, L"Grid Increment: %f\n", ViewportEntity->gridIncrement());
    ATLTRACE2(traceOdDb, 2, L"Grid On: %i\n", ViewportEntity->isGridOn());
    ATLTRACE2(traceOdDb, 2, L"Height: %f\n", ViewportEntity->height());
    ATLTRACE2(traceOdDb, 2, L"Lens Length: %f\n", ViewportEntity->lensLength());
    ATLTRACE2(traceOdDb, 2, L"Locked: %i\n", ViewportEntity->isLocked());
    ATLTRACE2(traceOdDb, 2, L"Non-Rectangular Clip: %i\n", ViewportEntity->isNonRectClipOn());

    if (!ViewportEntity->nonRectClipEntityId().isNull()) {
      ATLTRACE2(traceOdDb, 2, L"Non-rectangular Clipper: \n", ViewportEntity->nonRectClipEntityId().getHandle());
    }
    ATLTRACE2(traceOdDb, 2, L"Render Mode: %i\n", ViewportEntity->renderMode());
    ATLTRACE2(traceOdDb, 2, L"Remove Hidden Lines: %i\n", ViewportEntity->hiddenLinesRemoved());
    ATLTRACE2(traceOdDb, 2, L"Shade Plot: \n", ViewportEntity->shadePlot());
    ATLTRACE2(traceOdDb, 2, L"Snap Isometric: %i\n", ViewportEntity->isSnapIsometric());
    ATLTRACE2(traceOdDb, 2, L"Snap On: %i\n", ViewportEntity->isSnapOn());
    ATLTRACE2(traceOdDb, 2, L"Transparent: %i\n", ViewportEntity->isTransparent());
    ATLTRACE2(traceOdDb, 2, L"UCS Follow: %i\n", ViewportEntity->isUcsFollowModeOn());
    ATLTRACE2(traceOdDb, 2, L"UCS Icon at Origin: %i\n", ViewportEntity->isUcsIconAtOrigin());

    OdDb::OrthographicView orthoUCS;
    ATLTRACE2(traceOdDb, 2, L"UCS Orthographic: %i\n", ViewportEntity->isUcsOrthographic(orthoUCS));
    ATLTRACE2(traceOdDb, 2, L"Orthographic UCS: %i\n", orthoUCS);
    ATLTRACE2(traceOdDb, 2, L"UCS Saved with VP: %i\n", ViewportEntity->isUcsSavedWithViewport());

    if (!ViewportEntity->ucsName().isNull()) {
      //OdDbUCSTableRecordPtr pUCS = ViewportEntity->ucsName().safeOpenObject();
      //ATLTRACE2(traceOdDb, 2, L"UCS Name: \n", pUCS->getName());
    } else {
      ATLTRACE2(traceOdDb, 2, L"UCS Name: Null");
    }
    ATLTRACE2(traceOdDb, 2, L"View Center: %f, %f\n", ViewportEntity->viewCenter());
    ATLTRACE2(traceOdDb, 2, L"View Height: %f\n", ViewportEntity->viewHeight());
    ATLTRACE2(traceOdDb, 2, L"View Target: %f, %f, %f\n", ViewportEntity->viewTarget());
    ATLTRACE2(traceOdDb, 2, L"Width: %f\n", ViewportEntity->width());
    //ConvertEntityData(ViewportEntity, );
  }
};
/// <summary>Wipeout Converter</summary>
class EoDbWipeout_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup*) {
    OdDbWipeoutPtr WipeoutEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)WipeoutEntity->desc()->name());
  }
};
/// <summary>Xline Converter</summary>
class EoDbXline_Converter : public EoDbConvertEntityToPrimitive {
 public:
  void Convert(OdDbEntity* entity, EoDbGroup* /* group */) {
    OdDbXlinePtr XlineEntity = entity;
    ATLTRACE2(traceOdDb, 0, L"%s was not converted ...\n", (PCTSTR)XlineEntity->desc()->name());
  }
};
class Converters {
  OdStaticRxObject<EoDb2LineAngularDimension_Converter> m_2LineAngularDimensionConverter;
  OdStaticRxObject<EoDb2dPolyline_Converter> m_2dPolylineConverter;
  OdStaticRxObject<EoDb3PointAngularDimension_Converter> m_3PointAngularDimensionConverter;
  OdStaticRxObject<EoDb3dPolyline_Converter> m_3dPolylineConverter;
  OdStaticRxObject<EoDb3dSolid_Converter> m_3dSolidConverter;
  OdStaticRxObject<EoDbAlignedDimension_Converter> m_alignedDimensionConverter;
  OdStaticRxObject<EoDbArcAlignedText_Converter> m_arcAlignedTextConverter;
  OdStaticRxObject<EoDbArcDimension_Converter> m_arcDimensionConverter;
  OdStaticRxObject<EoDbArc_Converter> m_arcConverter;
  OdStaticRxObject<EoDbAttributeDefinition_Converter> m_attributeDefinitionConverter;
  OdStaticRxObject<EoDbBlockReference_Converter> m_blockReference;
  OdStaticRxObject<EoDbBody_Converter> m_bodyConverter;
  OdStaticRxObject<EoDbCircle_Converter> m_circleConverter;
  OdStaticRxObject<EoDbDiametricDimension_Converter> m_diametricDimensionConverter;
  OdStaticRxObject<EoDbEllipse_Converter> m_ellipseConverter;
  OdStaticRxObject<EoDbConvertEntityToPrimitive> m_entityConverter;
  OdStaticRxObject<EoDbFace_Converter> m_faceConverter;
  OdStaticRxObject<EoDbFcf_Converter> m_fcfConverter;
  OdStaticRxObject<EoDbHatch_Converter> m_hatchConverter;
  OdStaticRxObject<EoDbLeader_Converter> m_leaderConverter;
  OdStaticRxObject<EoDbLine_Converter> m_lineConverter;
  OdStaticRxObject<EoDbMInsertBlock_Converter> m_mInsertBlock;
  OdStaticRxObject<EoDbMText_Converter> m_mTextConverter;
  OdStaticRxObject<EoDbMline_Converter> m_mlineConverter;
  OdStaticRxObject<EoDbOle2Frame_Converter> m_ole2FrameConverter;
  OdStaticRxObject<EoDbOrdinateDimension_Converter> m_ordinateDimensionConverter;
  OdStaticRxObject<EoDbPoint_Converter> m_pointConverter;
  OdStaticRxObject<EoDbPolyFaceMesh_Converter> m_polyFaceMeshConverter;
  OdStaticRxObject<EoDbPolygonMesh_Converter> m_polygonMesh;
  OdStaticRxObject<EoDbPolyline_Converter> m_polylineConverter;
  OdStaticRxObject<EoDbProxyEntity_Converter> m_proxyEntityConverter;
  OdStaticRxObject<EoDbRadialDimension_Converter> m_radialDimensionConverter;
  OdStaticRxObject<EoDbRasterImage_Converter> m_imageConverter;
  OdStaticRxObject<EoDbRay_Converter> m_rayConverter;
  OdStaticRxObject<EoDbRegion_Converter> m_regionConverter;
  OdStaticRxObject<EoDbRotatedDimension_Converter> m_rotatedDimensionConverter;
  OdStaticRxObject<EoDbShape_Converter> m_shapeConverter;
  OdStaticRxObject<EoDbSolid_Converter> m_solidConverter;
  OdStaticRxObject<EoDbSpline_Converter> m_splineConverter;
  OdStaticRxObject<EoDbTable_Converter> m_tableConverter;
  OdStaticRxObject<EoDbText_Converter> m_textConverter;
  OdStaticRxObject<EoDbTrace_Converter> m_traceConverter;
  OdStaticRxObject<EoDbViewport_Converter> m_viewportConverter;
  OdStaticRxObject<EoDbWipeout_Converter> m_wipeoutConverter;
  OdStaticRxObject<EoDbXline_Converter> m_xlineConverter;

 public:
  /// Add Protocol Extensions ///
  void addXs() {
    OdDb2LineAngularDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_2LineAngularDimensionConverter);
    OdDb2dPolyline::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_2dPolylineConverter);
    OdDb3PointAngularDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_3PointAngularDimensionConverter);
    OdDb3dPolyline::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_3dPolylineConverter);
    OdDb3dSolid::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_3dSolidConverter);
    OdDbAlignedDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_alignedDimensionConverter);
    OdDbArc::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_arcConverter);
    OdDbArcAlignedText::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_arcAlignedTextConverter);
    OdDbArcDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_arcDimensionConverter);
    OdDbAttributeDefinition::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_attributeDefinitionConverter);
    OdDbBlockReference::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_blockReference);
    OdDbBody::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_bodyConverter);
    OdDbCircle::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_circleConverter);
    OdDbDiametricDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_diametricDimensionConverter);
    OdDbEllipse::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_ellipseConverter);
    OdDbEntity::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_entityConverter);
    OdDbFace::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_faceConverter);
    OdDbFcf::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_fcfConverter);
    OdDbHatch::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_hatchConverter);
    OdDbLeader::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_leaderConverter);
    OdDbLine::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_lineConverter);
    OdDbMInsertBlock::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_mInsertBlock);
    OdDbMText::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_mTextConverter);
    OdDbMline::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_mlineConverter);
    OdDbOle2Frame::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_ole2FrameConverter);
    OdDbOrdinateDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_ordinateDimensionConverter);
    OdDbPoint::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_pointConverter);
    OdDbPolyFaceMesh::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_polyFaceMeshConverter);
    OdDbPolygonMesh::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_polygonMesh);
    OdDbPolyline::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_polylineConverter);
    OdDbProxyEntity::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_proxyEntityConverter);
    OdDbRadialDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_radialDimensionConverter);
    OdDbRasterImage::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_imageConverter);
    OdDbRay::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_rayConverter);
    OdDbRegion::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_regionConverter);
    OdDbRotatedDimension::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_rotatedDimensionConverter);
    OdDbShape::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_shapeConverter);
    OdDbSolid::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_solidConverter);
    OdDbSpline::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_splineConverter);
    OdDbTable::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_tableConverter);
    OdDbText::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_textConverter);
    OdDbTrace::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_traceConverter);
    OdDbViewport::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_viewportConverter);
    OdDbWipeout::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_wipeoutConverter);
    OdDbXline::desc()->addX(EoDbConvertEntityToPrimitive::desc(), &m_xlineConverter);
  }
  /// Delete Protocol Extensions ///
  void delXs() {
    OdDb2LineAngularDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDb2dPolyline::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDb3PointAngularDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDb3dPolyline::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDb3dSolid::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbAlignedDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbArc::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbArcAlignedText::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbArcDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbAttributeDefinition ::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbBlockReference::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbBody::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbCircle::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbDiametricDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbEllipse::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbEntity::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbFace::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbFcf::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbHatch::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbLeader::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbLine::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbMInsertBlock::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbMText::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbMline::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbOle2Frame::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbOrdinateDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbPoint::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbPolyFaceMesh::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbPolygonMesh::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbPolyline::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbProxyEntity::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbRadialDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbRasterImage::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbRay::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbRegion::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbRotatedDimension::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbShape::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbSolid::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbSpline::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbTable::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbText::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbTrace::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbViewport::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbWipeout::desc()->delX(EoDbConvertEntityToPrimitive::desc());
    OdDbXline::desc()->delX(EoDbConvertEntityToPrimitive::desc());
  }
};
AeSysDoc* ProtocolExtension_ConvertEntityToPegPrimitive::m_Document = nullptr;

ProtocolExtension_ConvertEntityToPegPrimitive::ProtocolExtension_ConvertEntityToPegPrimitive(AeSysDoc* document) {
  m_Document = document;
}
ProtocolExtension_ConvertEntityToPegPrimitive::~ProtocolExtension_ConvertEntityToPegPrimitive() {
  if (m_Converters) { Uninitialize(); }
}
void ProtocolExtension_ConvertEntityToPegPrimitive::Initialize() {
  // Register EoDbConvertEntityToPrimitive with DWGdirect
  EoDbConvertEntityToPrimitive::rxInit();
  m_Converters = new Converters;
  m_Converters->addXs();
}
void ProtocolExtension_ConvertEntityToPegPrimitive::Uninitialize() {
  m_Converters->delXs();
  EoDbConvertEntityToPrimitive::rxUninit();
  delete m_Converters;
  m_Converters = 0;
}
#endif  // USING_ODA
