#include "Stdafx.h"

#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbConic.h"
#include "EoDbDimension.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
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
 *  @param characterCellDefinition The character cell definition containing the rotation and slant angles, expansion factor, and height for the character cell.
 *  @param normal The normal vector of the text plane.
 *  @param[out] xAxisReference Receives the reference X axis vector for the character cell.
 *  @param[out] yAxisReference Receives the reference Y axis vector for the character cell.
 */
void GetReferenceAxesForCharacterCell(EoDbCharacterCellDefinition& characterCellDefinition, const EoGeVector3d& normal,
    EoGeVector3d& xAxisReference, EoGeVector3d& yAxisReference) {
  xAxisReference = ComputeArbitraryAxis(normal);
  xAxisReference.RotAboutArbAx(normal, characterCellDefinition.RotationAngle());

  yAxisReference = CrossProduct(normal, xAxisReference);

  xAxisReference *= Eo::defaultCharacterCellAspectRatio * characterCellDefinition.Height() *
                    characterCellDefinition.ExpansionFactor();

  yAxisReference.RotAboutArbAx(normal, characterCellDefinition.SlantAngle());
  yAxisReference *= characterCellDefinition.Height();
}

EoGePoint3d ProjPtToLn(EoGePoint3d pt) {
  auto* document = AeSysDoc::GetDoc();

  EoGeLine line{};
  EoGePoint3d ptProj{};

  double dRel[2]{};

  auto GroupPosition = document->GetFirstWorkLayerGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = document->GetNextWorkLayerGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = Group->GetNext(PrimitivePosition);

      if (primitive->Is(EoDb::kLinePrimitive)) {
        line = static_cast<EoDbLine*>(primitive)->Line();
      } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
        line = static_cast<EoDbDimension*>(primitive)->Line();
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
  auto cursorPosition = GetCursorPosition();

  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
  EoGeLine testLine;
  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = Group->GetNext(PrimitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        auto* LinePrimitive = static_cast<EoDbLine*>(primitive);
        testLine = LinePrimitive->Line();
      } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
        auto* DimensionPrimitive = static_cast<EoDbDimension*>(primitive);
        testLine = DimensionPrimitive->Line();
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
      auto* Group =
          new EoDbGroup(EoDbLine::CreateLine(PreviousDimensionCursorPosition, cursorPosition)->WithProperties(1, 1));
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
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
      auto* Group = new EoDbGroup;

      if (PreviousDimensionCommand == ID_OP4) {
        GenerateLineEndItem(1, 0.1, cursorPosition, PreviousDimensionCursorPosition, Group);
        ModeLineUnhighlightOp(PreviousDimensionCommand);
        PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
      }
      auto* linearDimension = new EoDbDimension();

      linearDimension->SetColor(1);
      linearDimension->SetLineTypeIndex(1);

      linearDimension->SetBeginPoint(PreviousDimensionCursorPosition);
      linearDimension->SetEndPoint(cursorPosition);

      linearDimension->SetDefaultNote();
      linearDimension->SetTextColor(5);
      linearDimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      Group->AddTail(linearDimension);
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

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
      auto* Group = new EoDbGroup;
      if (PreviousDimensionCommand == ID_OP4)
        GenerateLineEndItem(1, 0.1, cursorPosition, PreviousDimensionCursorPosition, Group);
      else {
        ModeLineUnhighlightOp(PreviousDimensionCommand);
        PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
      }
      auto* linearDimension = new EoDbDimension();

      linearDimension->SetColor(1);
      linearDimension->SetLineTypeIndex(1);

      linearDimension->SetBeginPoint(PreviousDimensionCursorPosition);
      linearDimension->SetEndPoint(cursorPosition);

      linearDimension->SetDefaultNote();
      linearDimension->SetTextColor(5);
      linearDimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      Group->AddTail(linearDimension);
      GenerateLineEndItem(1, 0.1, PreviousDimensionCursorPosition, cursorPosition, Group);
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      PreviousDimensionCursorPosition = cursorPosition;
    } else
      app.AddModeInformationToMessageList();
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

      auto* Group =
          new EoDbGroup(EoDbLine::CreateLine(PreviousDimensionCursorPosition, cursorPosition)->WithProperties(1, 1));
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
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

      auto* radialDimension = new EoDbDimension();

      radialDimension->SetColor(1);
      radialDimension->SetLineTypeIndex(1);

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

      auto* diametricDimension = new EoDbDimension();

      diametricDimension->SetColor(1);
      diametricDimension->SetLineTypeIndex(1);

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
  auto cursorPosition = GetCursorPosition();

  static EoGePoint3d rProjPt[2];
  static EoGePoint3d center;
  static int iLns;
  static EoGeLine line;

  if (PreviousDimensionCommand != ID_OP8) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);

    if (SelectLineUsingPoint(cursorPosition) != 0) {
      auto* engagedPrimitive = static_cast<EoDbLine*>(EngagedPrimitive());

      rProjPt[0] = DetPt();
      line = engagedPrimitive->Line();
      PreviousDimensionCommand = ModeLineHighlightOp(ID_OP8);
      app.AddStringToMessageList(std::wstring(L"Select the second line."));
      iLns = 1;
    }
  } else {
    if (iLns == 1) {
      if (SelectLineUsingPoint(cursorPosition) != 0) {
        auto* engagedPrimitive = static_cast<EoDbLine*>(EngagedPrimitive());

        rProjPt[1] = DetPt();
        if (EoGeLine::Intersection(line, engagedPrimitive->Line(), center)) {
          iLns++;
          app.AddStringToMessageList(std::wstring(L"Specify the location for the dimension arc."));
        }
      }
    } else {
      double sweepAngle{};

      EoGeVector3d vCenterToProjPt(center, rProjPt[0]);
      EoGeVector3d vCenterToCur(center, cursorPosition);
      auto normal = CrossProduct(vCenterToProjPt, vCenterToCur);
      normal.Normalize();
      if (SweepAngleFromNormalAnd3Points(normal, rProjPt[0], cursorPosition, rProjPt[1], center, sweepAngle)) {
        double dRad = EoGeVector3d(center, cursorPosition).Length();

        line.begin = center.ProjectToward(rProjPt[0], dRad);
        line.end = line.begin.RotateAboutAxis(center, normal, sweepAngle);

        auto vXAx = EoGeVector3d(center, line.begin);
        EoGePoint3d ptRot(line.begin.RotateAboutAxis(center, normal, Eo::HalfPi));
        EoGeVector3d vYAx = EoGeVector3d(center, ptRot);
        EoGePoint3d ptArrow = line.begin.RotateAboutAxis(center, normal, Eo::Radian);

        auto* Group = new EoDbGroup;
        GenerateLineEndItem(1, 0.1, ptArrow, line.begin, Group);

        auto* radialArc = EoDbConic::CreateConicFromEllipsePrimitive(center, vXAx, vYAx, sweepAngle);
        radialArc->SetColor(1);
        radialArc->SetLineTypeIndex(1);
        Group->AddTail(radialArc);

        ptArrow = line.begin.RotateAboutAxis(center, normal, sweepAngle - Eo::Radian);
        GenerateLineEndItem(1, 0.1, ptArrow, line.end, Group);

        auto* deviceContext = GetDC();
        int savedRenderState = renderState.Save();

        EoDbFontDefinition fontDefinition = renderState.FontDefinition();
        fontDefinition.SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);
        renderState.SetFontDefinition(deviceContext, fontDefinition);

        auto characterCellDefinition = renderState.CharacterCellDefinition();
        characterCellDefinition.SetRotationAngle(0.0);
        characterCellDefinition.SetHeight(0.1);
        renderState.SetCharacterCellDefinition(characterCellDefinition);

        EoGePoint3d origin = cursorPosition.ProjectToward(center, -0.25);
        GetReferenceAxesForCharacterCell(characterCellDefinition, normal, vXAx, vYAx);
        EoGeReferenceSystem ReferenceSystem(origin, vXAx, vYAx);
        CString Note;
        app.FormatAngle(Note, sweepAngle, 8, 3);
        Group->AddTail(new EoDbText(fontDefinition, ReferenceSystem, Note));
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        renderState.Restore(deviceContext, savedRenderState);
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

  EoDbGroup* Group{};
  EoDbPrimitive* primitive{};
  EoGePoint3d ptProj;

  POSITION posPrimCur;

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    Group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      posPrimCur = PrimitivePosition;
      primitive = Group->GetNext(PrimitivePosition);
      if (primitive->SelectUsingPoint(this, ptView, ptProj)) {
        if (primitive->Is(EoDb::kLinePrimitive)) {
          EoDbLine* line = static_cast<EoDbLine*>(primitive);
          auto* dimension = new EoDbDimension();

          dimension->SetColor(line->Color());
          dimension->SetLineTypeIndex(line->LineTypeIndex());

          dimension->SetBeginPoint(line->Begin());
          dimension->SetEndPoint(line->End());

          dimension->SetDefaultNote();
          dimension->SetTextColor(5);
          dimension->SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

          Group->InsertAfter(posPrimCur, dimension);
          Group->RemoveAt(posPrimCur);
          delete primitive;
          PreviousDimensionCursorPosition = ptProj;
          return;
        } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
          EoDbDimension* pPrimDim = static_cast<EoDbDimension*>(primitive);
          EoGeReferenceSystem ReferenceSystem = pPrimDim->ReferenceSystem();

          auto* linePrimitive =
              EoDbLine::CreateLine(pPrimDim->Line())->WithProperties(primitive->Color(), primitive->LineTypeIndex());

          EoDbText* pPrimText = new EoDbText(pPrimDim->FontDefinition(), ReferenceSystem, pPrimDim->Text());
          pPrimText->SetColor(pPrimDim->TextColor());
          Group->InsertAfter(posPrimCur, linePrimitive);
          Group->InsertAfter(posPrimCur, pPrimText);
          Group->RemoveAt(posPrimCur);
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
  auto cursorPosition = GetCursorPosition();
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
