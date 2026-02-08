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
#include "PrimState.h"
#include "Resource.h"

namespace {

constexpr double DimensionModePickTolerance{0.05};

EoGePoint3d PreviousDimensionCursorPosition{};
EoUInt16 PreviousDimensionCommand{};

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

  xAxisReference *= 0.6 * characterCellDefinition.Height() * characterCellDefinition.ExpansionFactor();

  yAxisReference.RotAboutArbAx(normal, characterCellDefinition.SlantAngle());
  yAxisReference *= characterCellDefinition.Height();
}

EoGePoint3d ProjPtToLn(EoGePoint3d pt) {
  auto* document = AeSysDoc::GetDoc();

  EoGeLine ln{};
  EoGePoint3d ptProj{};

  double dRel[2]{};

  auto GroupPosition = document->GetFirstWorkLayerGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = document->GetNextWorkLayerGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = Group->GetNext(PrimitivePosition);

      if (primitive->Is(EoDb::kLinePrimitive)) {
        static_cast<EoDbLine*>(primitive)->GetLine(ln);
      } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
        ln = static_cast<EoDbDimension*>(primitive)->Line();
      } else {
        continue;
      }

      if (ln.IsSelectedByPointXY(pt, DimensionModePickTolerance, ptProj, dRel))
        return (*dRel <= 0.5) ? ln.begin : ln.end;
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
  EoGeLine TestLine;
  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = Group->GetNext(PrimitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        EoDbLine* LinePrimitive = static_cast<EoDbLine*>(primitive);
        LinePrimitive->GetLine(TestLine);
      } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
        EoDbDimension* DimensionPrimitive = static_cast<EoDbDimension*>(primitive);
        TestLine = DimensionPrimitive->Line();
      } else {
        continue;
      }
      EoGePoint3d ptProj;
      double dRel[2]{};

      if (TestLine.IsSelectedByPointXY(cursorPosition, DimensionModePickTolerance, ptProj, dRel)) {
        EoGePoint3d pt;

        EoDbGroup* NewGroup = new EoDbGroup;
        if (*dRel <= 0.5) {
          GenerateLineEndItem(1, 0.1, TestLine.end, TestLine.begin, NewGroup);
          pt = TestLine.begin;
        } else {
          GenerateLineEndItem(1, 0.1, TestLine.begin, TestLine.end, NewGroup);
          pt = TestLine.end;
        }
        document->AddWorkLayerGroup(NewGroup);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);

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
      auto* Group = new EoDbGroup(new EoDbLine(1, 1, PreviousDimensionCursorPosition, cursorPosition));
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
      EoDbDimension* pDim = new EoDbDimension(1, 1, EoGeLine(PreviousDimensionCursorPosition, cursorPosition));
      pDim->SetTextColor(5);
      pDim->SetAlignment(EoDb::AlignCenter, EoDb::AlignMiddle);

      Group->AddTail(pDim);
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
      EoDbDimension* pDim = new EoDbDimension(1, 1, EoGeLine(PreviousDimensionCursorPosition, cursorPosition));
      pDim->SetTextColor(5);
      pDim->SetAlignment(EoDb::AlignCenter, EoDb::AlignMiddle);

      Group->AddTail(pDim);
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

      auto* Group = new EoDbGroup(new EoDbLine(1, 1, PreviousDimensionCursorPosition, cursorPosition));
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

      auto* radialDimension = new EoDbDimension(1, 1, EoGeLine(center, ptEnd));
      radialDimension->SetText(L"R" + radialDimension->Text());
      radialDimension->SetDefaultNote();

      radialDimension->SetTextColor(5);
      radialDimension->SetAlignment(EoDb::AlignCenter, EoDb::AlignMiddle);

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

      EoDbDimension* diametricDimension = new EoDbDimension(1, 1, EoGeLine(begin, end));
      diametricDimension->SetText(L"D" + diametricDimension->Text());
      diametricDimension->SetDefaultNote();

      diametricDimension->SetTextColor(5);
      diametricDimension->SetAlignment(EoDb::AlignCenter, EoDb::AlignMiddle);

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
  CDC* DeviceContext = GetDC();

  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  static EoGePoint3d rProjPt[2];
  static EoGePoint3d center;
  static int iLns;
  static EoGeLine ln;

  if (PreviousDimensionCommand != ID_OP8) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);

    if (SelectLineUsingPoint(cursorPosition) != 0) {
      EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

      rProjPt[0] = DetPt();
      pLine->GetLine(ln);
      PreviousDimensionCommand = ModeLineHighlightOp(ID_OP8);
      app.AddStringToMessageList(std::wstring(L"Select the second line."));
      iLns = 1;
    }
  } else {
    if (iLns == 1) {
      if (SelectLineUsingPoint(cursorPosition) != 0) {
        EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

        rProjPt[1] = DetPt();
        if (EoGeLine::Intersection(ln, pLine->Ln(), center)) {
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

        ln.begin = center.ProjectToward(rProjPt[0], dRad);
        ln.end = ln.begin.RotateAboutAxis(center, normal, sweepAngle);

        auto vXAx = EoGeVector3d(center, ln.begin);
        EoGePoint3d ptRot(ln.begin.RotateAboutAxis(center, normal, Eo::HalfPi));
        EoGeVector3d vYAx = EoGeVector3d(center, ptRot);
        EoGePoint3d ptArrow = ln.begin.RotateAboutAxis(center, normal, Eo::Radian);

        auto* Group = new EoDbGroup;
        GenerateLineEndItem(1, 0.1, ptArrow, ln.begin, Group);

        auto* radialArc = EoDbConic::CreateConicFromEllipsePrimitive(center, vXAx, vYAx, sweepAngle);
        radialArc->SetColor(1);
        radialArc->SetLineTypeIndex(1);
        Group->AddTail(radialArc);

        ptArrow = ln.begin.RotateAboutAxis(center, normal, sweepAngle - Eo::Radian);
        GenerateLineEndItem(1, 0.1, ptArrow, ln.end, Group);

        int PrimitiveState = pstate.Save();

        EoDbFontDefinition fontDefinition = pstate.FontDefinition();
        fontDefinition.SetAlignment(EoDb::AlignCenter, EoDb::AlignMiddle);
        pstate.SetFontDef(DeviceContext, fontDefinition);

        auto characterCellDefinition = pstate.CharacterCellDefinition();
        characterCellDefinition.SetRotationAngle(0.0);
        characterCellDefinition.SetHeight(0.1);
        pstate.SetCharCellDef(characterCellDefinition);

        EoGePoint3d ptPvt = cursorPosition.ProjectToward(center, -0.25);
        GetReferenceAxesForCharacterCell(characterCellDefinition, normal, vXAx, vYAx);
        EoGeReferenceSystem ReferenceSystem(ptPvt, vXAx, vYAx);
        CString Note;
        app.FormatAngle(Note, sweepAngle, 8, 3);
        Group->AddTail(new EoDbText(fontDefinition, ReferenceSystem, Note));
        document->AddWorkLayerGroup(Group);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        pstate.Restore(DeviceContext, PrimitiveState);
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
        EoGeLine ln;

        if (primitive->Is(EoDb::kLinePrimitive)) {
          EoDbLine* pPrimLine = static_cast<EoDbLine*>(primitive);
          pPrimLine->GetLine(ln);
          EoDbDimension* pPrimDim = new EoDbDimension(pPrimLine->Color(), pPrimLine->LineTypeIndex(), ln);
          pPrimDim->SetTextColor(5);
          pPrimDim->SetAlignment(EoDb::AlignCenter, EoDb::AlignMiddle);

          Group->InsertAfter(posPrimCur, pPrimDim);
          Group->RemoveAt(posPrimCur);
          delete primitive;
          PreviousDimensionCursorPosition = ptProj;
          return;
        } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
          EoDbDimension* pPrimDim = static_cast<EoDbDimension*>(primitive);
          EoGeReferenceSystem ReferenceSystem;
          pPrimDim->GetRefSys(ReferenceSystem);
          EoDbLine* pPrimLine = new EoDbLine(pPrimDim->Color(), pPrimDim->LineTypeIndex(), pPrimDim->Line());
          EoDbText* pPrimText = new EoDbText(pPrimDim->FontDefinition(), ReferenceSystem, pPrimDim->Text());
          pPrimText->SetColor(pPrimDim->TextColor());
          Group->InsertAfter(posPrimCur, pPrimLine);
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
