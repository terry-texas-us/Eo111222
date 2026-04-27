#include "Stdafx.h"

#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbConic.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbLabeledLine.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"
#include "Resource.h"

namespace {

constexpr double DimensionModePickTolerance{0.05};

EoGePoint3d PreviousDimensionCursorPosition{};
std::uint16_t PreviousDimensionCommand{};

/** @brief Produces the reference system vectors for a single character cell.
 *  @param characterCellDefinition The character cell definition containing the rotation and slant angles, expansion
 * factor, and height for the character cell.
 *  @param normal The normal vector of the text plane.
 *  @param[out] xAxisReference Receives the reference X axis vector for the character cell.
 *  @param[out] yAxisReference Receives the reference Y axis vector for the character cell.
 */
void GetReferenceAxesForCharacterCell(const EoDbCharacterCellDefinition& characterCellDefinition,
    const EoGeVector3d& normal,
    EoGeVector3d& xAxisReference,
    EoGeVector3d& yAxisReference) {
  xAxisReference = ComputeArbitraryAxis(normal);
  xAxisReference.RotateAboutArbitraryAxis(normal, characterCellDefinition.RotationAngle());

  yAxisReference = CrossProduct(normal, xAxisReference);

  xAxisReference *= Eo::defaultCharacterCellAspectRatio * characterCellDefinition.Height()
      * characterCellDefinition.ExpansionFactor();

  yAxisReference.RotateAboutArbitraryAxis(normal, characterCellDefinition.SlantAngle());
  yAxisReference *= characterCellDefinition.Height();
}

EoGePoint3d ProjPtToLn(EoGePoint3d pt) {
  auto* document = AeSysDoc::GetDoc();

  EoGeLine line{};
  EoGePoint3d ptProj{};

  double dRel[2]{};

  auto groupPosition = document->GetFirstWorkLayerGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = document->GetNextWorkLayerGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);

      if (primitive->Is(EoDb::kLinePrimitive)) {
        line = static_cast<EoDbLine*>(primitive)->Line();
      } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
        line = static_cast<EoDbLabeledLine*>(primitive)->Line();
      } else {
        continue;
      }

      if (line.IsSelectedByPointXY(pt, DimensionModePickTolerance, ptProj, dRel)) {
        return (*dRel <= 0.5) ? line.begin : line.end;
      }
    }
  }
  return pt;
}
}  // namespace

void AeSysView::OnDimensionModeOptions() {
  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
  PreviousDimensionCursorPosition = GetCursorPosition();
}

void AeSysView::OnDimensionModeArrow() {
  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();

  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
  EoGeLine testLine;
  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        auto* linePrimitive = static_cast<EoDbLine*>(primitive);
        testLine = linePrimitive->Line();
      } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
        auto* dimensionPrimitive = static_cast<EoDbLabeledLine*>(primitive);
        testLine = dimensionPrimitive->Line();
      } else {
        continue;
      }
      EoGePoint3d ptProj;
      double dRel[2]{};

      if (testLine.IsSelectedByPointXY(cursorPosition, DimensionModePickTolerance, ptProj, dRel)) {
        EoGePoint3d pt;

        auto* newGroup = new EoDbGroup;
        if (*dRel <= 0.5) {
          GenerateLineEndItem(1, 0.1, testLine.end, testLine.begin, newGroup);
          pt = testLine.begin;
        } else {
          GenerateLineEndItem(1, 0.1, testLine.begin, testLine.end, newGroup);
          pt = testLine.end;
        }
        document->AddWorkLayerGroup(newGroup);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newGroup);

        SetCursorPosition(pt);
        PreviousDimensionCursorPosition = pt;
        return;
      }
    }
  }
  PreviousDimensionCursorPosition = cursorPosition;
}

void AeSysView::OnDimensionModeLine() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  RubberBandingDisable();
  if (PreviousDimensionCommand != ID_OP2) {
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP2);
    PreviousDimensionCursorPosition = cursorPosition;
  } else {
    cursorPosition = SnapPointToAxis(PreviousDimensionCursorPosition, cursorPosition);
    if (PreviousDimensionCursorPosition != cursorPosition) {
      auto* group = new EoDbGroup(
          EoDbLine::CreateLine(PreviousDimensionCursorPosition, cursorPosition)->WithProperties(1, L"CONTINUOUS"));
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    PreviousDimensionCursorPosition = cursorPosition;
  }
  RubberBandingStartAtEnable(cursorPosition, Lines);
}

void AeSysView::OnDimensionModeDLine() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
    RubberBandingDisable();
    if (PreviousDimensionCursorPosition != cursorPosition) {
      auto* group = new EoDbGroup;

      if (PreviousDimensionCommand == ID_OP4) {
        GenerateLineEndItem(1, 0.1, cursorPosition, PreviousDimensionCursorPosition, group);
        ModeLineUnhighlightOp(PreviousDimensionCommand);
        PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
      }
      auto* linearDimension = new EoDbLabeledLine();

      linearDimension->SetColor(1);
      linearDimension->SetLineTypeName(L"CONTINUOUS");

      linearDimension->SetBeginPoint(PreviousDimensionCursorPosition);
      linearDimension->SetEndPoint(cursorPosition);

      linearDimension->SetDefaultNote();
      linearDimension->SetTextColor(5);
      linearDimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      group->AddTail(linearDimension);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      PreviousDimensionCursorPosition = cursorPosition;
    }
  } else {
    if (PreviousDimensionCommand != 0) {
      RubberBandingDisable();
      ModeLineUnhighlightOp(PreviousDimensionCommand);
    }
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
    PreviousDimensionCursorPosition = cursorPosition;
  }
  SetCursorPosition(cursorPosition);
  RubberBandingStartAtEnable(cursorPosition, Lines);
}

void AeSysView::OnDimensionModeDLine2() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (PreviousDimensionCommand == 0) {
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
    PreviousDimensionCursorPosition = cursorPosition;
  } else if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
    RubberBandingDisable();
    if (PreviousDimensionCursorPosition != cursorPosition) {
      auto* group = new EoDbGroup;
      if (PreviousDimensionCommand == ID_OP4) {
        GenerateLineEndItem(1, 0.1, cursorPosition, PreviousDimensionCursorPosition, group);
      } else {
        ModeLineUnhighlightOp(PreviousDimensionCommand);
        PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
      }
      auto* linearDimension = new EoDbLabeledLine();

      linearDimension->SetColor(1);
      linearDimension->SetLineTypeName(L"CONTINUOUS");

      linearDimension->SetBeginPoint(PreviousDimensionCursorPosition);
      linearDimension->SetEndPoint(cursorPosition);

      linearDimension->SetDefaultNote();
      linearDimension->SetTextColor(5);
      linearDimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      group->AddTail(linearDimension);
      GenerateLineEndItem(1, 0.1, PreviousDimensionCursorPosition, cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      PreviousDimensionCursorPosition = cursorPosition;
    } else {
      app.AddModeInformationToMessageList();
    }
  } else {
    // error finish prior op first
  }
  SetCursorPosition(cursorPosition);
  RubberBandingStartAtEnable(cursorPosition, Lines);
}

void AeSysView::OnDimensionModeExten() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (PreviousDimensionCommand != ID_OP5) {
    RubberBandingDisable();
    PreviousDimensionCursorPosition = ProjPtToLn(cursorPosition);
    ModeLineUnhighlightOp(PreviousDimensionCommand);
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP5);
  } else {
    cursorPosition = ProjPtToLn(cursorPosition);
    if (PreviousDimensionCursorPosition != cursorPosition) {
      cursorPosition = cursorPosition.ProjectToward(PreviousDimensionCursorPosition, -0.1875);
      PreviousDimensionCursorPosition = PreviousDimensionCursorPosition.ProjectToward(cursorPosition, 0.0625);

      auto* group = new EoDbGroup(
          EoDbLine::CreateLine(PreviousDimensionCursorPosition, cursorPosition)->WithProperties(1, L"CONTINUOUS"));
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    PreviousDimensionCursorPosition = cursorPosition;
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
}

void AeSysView::OnDimensionModeRadius() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  if (SelectGroupAndPrimitive(cursorPosition) != nullptr) {
    EoGePoint3d ptEnd = DetPt();

    if ((EngagedPrimitive())->Is(EoDb::kConicPrimitive)) {
      auto* conic = static_cast<EoDbConic*>(EngagedPrimitive());

      EoGePoint3d center = conic->Center();

      auto* group = new EoDbGroup;

      auto* radialDimension = new EoDbLabeledLine();

      radialDimension->SetColor(1);
      radialDimension->SetLineTypeName(L"CONTINUOUS");

      radialDimension->SetBeginPoint(center);
      radialDimension->SetEndPoint(ptEnd);

      radialDimension->SetText(L"R" + radialDimension->Text());
      radialDimension->SetDefaultNote();
      radialDimension->SetTextColor(5);
      radialDimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      group->AddTail(radialDimension);

      GenerateLineEndItem(1, 0.1, center, ptEnd, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      PreviousDimensionCursorPosition = ptEnd;
    }
  } else {  // error arc not identified
    PreviousDimensionCursorPosition = cursorPosition;
  }
}

void AeSysView::OnDimensionModeDiameter() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  if (SelectGroupAndPrimitive(cursorPosition) != nullptr) {
    EoGePoint3d end = DetPt();

    if ((EngagedPrimitive())->Is(EoDb::kConicPrimitive)) {
      auto* conic = static_cast<EoDbConic*>(EngagedPrimitive());

      auto begin = end.ProjectToward(conic->Center(), 2.0 * conic->Radius());

      auto* group = new EoDbGroup;

      GenerateLineEndItem(1, 0.1, end, begin, group);

      auto* diametricDimension = new EoDbLabeledLine();

      diametricDimension->SetColor(1);
      diametricDimension->SetLineTypeName(L"CONTINUOUS");

      diametricDimension->SetBeginPoint(begin);
      diametricDimension->SetEndPoint(end);

      diametricDimension->SetText(L"D" + diametricDimension->Text());
      diametricDimension->SetDefaultNote();
      diametricDimension->SetTextColor(5);
      diametricDimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      group->AddTail(diametricDimension);

      GenerateLineEndItem(1, 0.1, begin, end, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      PreviousDimensionCursorPosition = end;
    }
  } else {
    PreviousDimensionCursorPosition = cursorPosition;
  }
}

void AeSysView::OnDimensionModeAngle() {
  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();

  static EoGePoint3d rProjPt[2];
  static EoGePoint3d center;
  static int iLns;
  static EoGeLine line;

  if (PreviousDimensionCommand != ID_OP8) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);

    if (SelectLineUsingPoint(cursorPosition) != nullptr) {
      auto* engagedPrimitive = static_cast<EoDbLine*>(EngagedPrimitive());

      rProjPt[0] = DetPt();
      line = engagedPrimitive->Line();
      PreviousDimensionCommand = ModeLineHighlightOp(ID_OP8);
      app.AddStringToMessageList(std::wstring(L"Select the second line."));
      iLns = 1;
    }
  } else {
    if (iLns == 1) {
      if (SelectLineUsingPoint(cursorPosition) != nullptr) {
        auto* engagedPrimitive = static_cast<EoDbLine*>(EngagedPrimitive());

        rProjPt[1] = DetPt();
        if (EoGeLine::Intersection(line, engagedPrimitive->Line(), center)) {
          iLns++;
          app.AddStringToMessageList(std::wstring(L"Specify the location for the dimension arc."));
        }
      }
    } else {
      double sweepAngle{};

      const EoGeVector3d vCenterToProjPt(center, rProjPt[0]);
      const EoGeVector3d vCenterToCur(center, cursorPosition);
      auto normal = CrossProduct(vCenterToProjPt, vCenterToCur);
      normal.Unitize();
      if (SweepAngleFromNormalAnd3Points(normal, rProjPt[0], cursorPosition, rProjPt[1], center, sweepAngle)) {
        const double dRad = EoGeVector3d(center, cursorPosition).Length();

        line.begin = center.ProjectToward(rProjPt[0], dRad);
        line.end = line.begin.RotateAboutAxis(center, normal, sweepAngle);

        auto vXAx = EoGeVector3d(center, line.begin);
        EoGePoint3d ptRot(line.begin.RotateAboutAxis(center, normal, Eo::HalfPi));
        EoGeVector3d vYAx = EoGeVector3d(center, ptRot);
        EoGePoint3d ptArrow = line.begin.RotateAboutAxis(center, normal, Eo::Radian);

        auto* group = new EoDbGroup;
        GenerateLineEndItem(1, 0.1, ptArrow, line.begin, group);

        auto* radialArc = EoDbConic::CreateConicFromEllipsePrimitive(center, vXAx, vYAx, sweepAngle);
        radialArc->SetColor(1);
        radialArc->SetLineTypeName(L"CONTINUOUS");
        group->AddTail(radialArc);

        ptArrow = line.begin.RotateAboutAxis(center, normal, sweepAngle - Eo::Radian);
        GenerateLineEndItem(1, 0.1, ptArrow, line.end, group);

        auto* deviceContext = GetDC();
        int savedRenderState = Gs::renderState.Save();

        EoDbFontDefinition fontDefinition = Gs::renderState.FontDefinition();
        fontDefinition.SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);
        Gs::renderState.SetFontDefinition(deviceContext, fontDefinition);

        auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();
        characterCellDefinition.SetRotationAngle(0.0);
        characterCellDefinition.SetHeight(0.1);
        Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);

        EoGePoint3d origin = cursorPosition.ProjectToward(center, -0.25);
        GetReferenceAxesForCharacterCell(characterCellDefinition, normal, vXAx, vYAx);
        EoGeReferenceSystem referenceSystem(origin, vXAx, vYAx);
        CString note;
        app.FormatAngle(note, sweepAngle, 8, 3);
        group->AddTail(new EoDbText(fontDefinition, referenceSystem, note));
        document->AddWorkLayerGroup(group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        Gs::renderState.Restore(deviceContext, savedRenderState);
        ReleaseDC(deviceContext);
      }
      ModeLineUnhighlightOp(PreviousDimensionCommand);
      app.AddModeInformationToMessageList();
    }
  }
}

void AeSysView::OnDimensionModeConvert() {
  auto cursorPosition = GetCursorPosition();
  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }

  EoDbGroup* group{};
  EoDbPrimitive* primitive{};
  EoGePoint3d ptProj;

  POSITION posPrimCur;

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    group = GetNextVisibleGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      posPrimCur = primitivePosition;
      primitive = group->GetNext(primitivePosition);
      if (primitive->SelectUsingPoint(this, ptView, ptProj)) {
        if (primitive->Is(EoDb::kLinePrimitive)) {
          auto* line = static_cast<EoDbLine*>(primitive);
          auto* dimension = new EoDbLabeledLine();

          dimension->SetColor(line->Color());
          dimension->SetLineTypeName(line->LineTypeName());

          dimension->SetBeginPoint(line->Begin());
          dimension->SetEndPoint(line->End());

          dimension->SetDefaultNote();
          dimension->SetTextColor(5);
          dimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

          group->InsertAfter(posPrimCur, dimension);
          group->RemoveAt(posPrimCur);
          delete primitive;
          PreviousDimensionCursorPosition = ptProj;
          return;
        } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
          auto* pPrimDim = static_cast<EoDbLabeledLine*>(primitive);
          EoGeReferenceSystem referenceSystem = pPrimDim->ReferenceSystem();

          auto* linePrimitive =
              EoDbLine::CreateLine(pPrimDim->Line())
                  ->WithProperties(primitive->Color(), primitive->LineTypeName(), primitive->LineWeight());

          auto* pPrimText = new EoDbText(pPrimDim->FontDefinition(), referenceSystem, pPrimDim->Text());
          pPrimText->SetColor(pPrimDim->TextColor());
          group->InsertAfter(posPrimCur, linePrimitive);
          group->InsertAfter(posPrimCur, pPrimText);
          group->RemoveAt(posPrimCur);
          delete primitive;
          PreviousDimensionCursorPosition = ptProj;
          return;
        }
      }
    }
  }
  PreviousDimensionCursorPosition = cursorPosition;
}

void AeSysView::OnDimensionModeReturn() {
  const auto cursorPosition = GetCursorPosition();
  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
  PreviousDimensionCursorPosition = cursorPosition;
}

void AeSysView::OnDimensionModeEscape() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(PreviousDimensionCommand);
}
