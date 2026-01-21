#include "Stdafx.h"

#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbDimension.h"
#include "EoDbEllipse.h"
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
      auto* Primitive = Group->GetNext(PrimitivePosition);

      if (Primitive->Is(EoDb::kLinePrimitive))
        static_cast<EoDbLine*>(Primitive)->GetLine(ln);
      else if (Primitive->Is(EoDb::kDimensionPrimitive))
        ln = static_cast<EoDbDimension*>(Primitive)->Line();
      else
        continue;

      if (ln.IsSelectedByPointXY(pt, DimensionModePickTolerance, ptProj, dRel)) return (*dRel <= 0.5) ? ln.begin : ln.end;
    }
  }
  return (pt);
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
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();

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
      auto* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Is(EoDb::kLinePrimitive)) {
        EoDbLine* LinePrimitive = static_cast<EoDbLine*>(Primitive);
        LinePrimitive->GetLine(TestLine);
      } else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
        EoDbDimension* DimensionPrimitive = static_cast<EoDbDimension*>(Primitive);
        TestLine = DimensionPrimitive->Line();
      } else {
        continue;
      }
      EoGePoint3d ptProj;
      double dRel[2]{};

      if (TestLine.IsSelectedByPointXY(ptCur, DimensionModePickTolerance, ptProj, dRel)) {
        EoGePoint3d pt;

        EoDbGroup* NewGroup = new EoDbGroup;
        if (*dRel <= 0.5) {
          GenerateLineEndItem(1, 0.1, TestLine.end, TestLine.begin, NewGroup);
          pt = TestLine.begin;
        } else {
          GenerateLineEndItem(1, 0.1, TestLine.begin, TestLine.end, NewGroup);
          pt = TestLine.end;
        }
        Document->AddWorkLayerGroup(NewGroup);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);

        SetCursorPosition(pt);
        PreviousDimensionCursorPosition = pt;
        return;
      }
    }
  }
  PreviousDimensionCursorPosition = ptCur;
}

void AeSysView::OnDimensionModeLine() {
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();
  RubberBandingDisable();
  if (PreviousDimensionCommand != ID_OP2) {
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP2);
    PreviousDimensionCursorPosition = ptCur;
  } else {
    ptCur = SnapPointToAxis(PreviousDimensionCursorPosition, ptCur);
    if (PreviousDimensionCursorPosition != ptCur) {
      auto* Group = new EoDbGroup(new EoDbLine(1, 1, PreviousDimensionCursorPosition, ptCur));
      Document->AddWorkLayerGroup(Group);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
    PreviousDimensionCursorPosition = ptCur;
  }
  RubberBandingStartAtEnable(ptCur, Lines);
}

void AeSysView::OnDimensionModeDLine() {
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();
  if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
    RubberBandingDisable();
    if (PreviousDimensionCursorPosition != ptCur) {
      auto* Group = new EoDbGroup;

      if (PreviousDimensionCommand == ID_OP4) {
        GenerateLineEndItem(1, 0.1, ptCur, PreviousDimensionCursorPosition, Group);
        ModeLineUnhighlightOp(PreviousDimensionCommand);
        PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
      }
      EoDbDimension* pDim = new EoDbDimension(1, 1, EoGeLine(PreviousDimensionCursorPosition, ptCur));
      pDim->SetTextPenColor(5);
      pDim->SetTextHorAlign(EoDb::kAlignCenter);
      pDim->SetTextVerAlign(EoDb::kAlignMiddle);

      Group->AddTail(pDim);
      Document->AddWorkLayerGroup(Group);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      PreviousDimensionCursorPosition = ptCur;
    }
  } else {
    if (PreviousDimensionCommand != 0) {
      RubberBandingDisable();
      ModeLineUnhighlightOp(PreviousDimensionCommand);
    }
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP3);
    PreviousDimensionCursorPosition = ptCur;
  }
  SetCursorPosition(ptCur);
  RubberBandingStartAtEnable(ptCur, Lines);
}

void AeSysView::OnDimensionModeDLine2() {
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();
  if (PreviousDimensionCommand == 0) {
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
    PreviousDimensionCursorPosition = ptCur;
  } else if (PreviousDimensionCommand == ID_OP3 || PreviousDimensionCommand == ID_OP4) {
    RubberBandingDisable();
    if (PreviousDimensionCursorPosition != ptCur) {
      auto* Group = new EoDbGroup;
      if (PreviousDimensionCommand == ID_OP4)
        GenerateLineEndItem(1, 0.1, ptCur, PreviousDimensionCursorPosition, Group);
      else {
        ModeLineUnhighlightOp(PreviousDimensionCommand);
        PreviousDimensionCommand = ModeLineHighlightOp(ID_OP4);
      }
      EoDbDimension* pDim = new EoDbDimension(1, 1, EoGeLine(PreviousDimensionCursorPosition, ptCur));
      pDim->SetTextPenColor(5);
      pDim->SetTextHorAlign(EoDb::kAlignCenter);
      pDim->SetTextVerAlign(EoDb::kAlignMiddle);

      Group->AddTail(pDim);
      GenerateLineEndItem(1, 0.1, PreviousDimensionCursorPosition, ptCur, Group);
      Document->AddWorkLayerGroup(Group);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      PreviousDimensionCursorPosition = ptCur;
    } else
      app.AddModeInformationToMessageList();
  } else {
    // error finish prior op first
  }
  SetCursorPosition(ptCur);
  RubberBandingStartAtEnable(ptCur, Lines);
}

void AeSysView::OnDimensionModeExten() {
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();
  if (PreviousDimensionCommand != ID_OP5) {
    RubberBandingDisable();
    PreviousDimensionCursorPosition = ProjPtToLn(ptCur);
    ModeLineUnhighlightOp(PreviousDimensionCommand);
    PreviousDimensionCommand = ModeLineHighlightOp(ID_OP5);
  } else {
    ptCur = ProjPtToLn(ptCur);
    if (PreviousDimensionCursorPosition != ptCur) {
      ptCur = ptCur.ProjectToward(PreviousDimensionCursorPosition, -0.1875);
      PreviousDimensionCursorPosition = PreviousDimensionCursorPosition.ProjectToward(ptCur, 0.0625);

      auto* Group = new EoDbGroup(new EoDbLine(1, 1, PreviousDimensionCursorPosition, ptCur));
      Document->AddWorkLayerGroup(Group);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
    PreviousDimensionCursorPosition = ptCur;
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
}

void AeSysView::OnDimensionModeRadius() {
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();

  if (SelectGroupAndPrimitive(ptCur) != 0) {
    EoGePoint3d ptEnd = DetPt();

    if ((EngagedPrimitive())->Is(EoDb::kEllipsePrimitive)) {
      EoDbEllipse* pArc = static_cast<EoDbEllipse*>(EngagedPrimitive());

      EoGePoint3d ptBeg = pArc->CenterPoint();

      auto* Group = new EoDbGroup;

      EoDbDimension* pDim = new EoDbDimension(1, 1, EoGeLine(ptBeg, ptEnd));
      pDim->SetText(L"R" + pDim->Text());
      pDim->SetDefaultNote();

      pDim->SetTextPenColor(5);
      pDim->SetTextHorAlign(EoDb::kAlignCenter);
      pDim->SetTextVerAlign(EoDb::kAlignMiddle);

      Group->AddTail(pDim);

      GenerateLineEndItem(1, 0.1, ptBeg, ptEnd, Group);
      Document->AddWorkLayerGroup(Group);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      PreviousDimensionCursorPosition = ptEnd;
    }
  } else {  // error arc not identified
    PreviousDimensionCursorPosition = ptCur;
  }
}

void AeSysView::OnDimensionModeDiameter() {
  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();

  if (SelectGroupAndPrimitive(ptCur) != 0) {
    EoGePoint3d ptEnd = DetPt();

    if ((EngagedPrimitive())->Is(EoDb::kEllipsePrimitive)) {
      EoDbEllipse* pArc = static_cast<EoDbEllipse*>(EngagedPrimitive());

      EoGePoint3d ptBeg = ptEnd.ProjectToward(pArc->CenterPoint(), 2. * pArc->MajorAxis().Length());

      auto* Group = new EoDbGroup;

      GenerateLineEndItem(1, 0.1, ptEnd, ptBeg, Group);

      EoDbDimension* pDim = new EoDbDimension(1, 1, EoGeLine(ptBeg, ptEnd));
      pDim->SetText(L"D" + pDim->Text());
      pDim->SetDefaultNote();

      pDim->SetTextPenColor(5);
      pDim->SetTextHorAlign(EoDb::kAlignCenter);
      pDim->SetTextVerAlign(EoDb::kAlignMiddle);
      Group->AddTail(pDim);

      GenerateLineEndItem(1, 0.1, ptBeg, ptEnd, Group);
      Document->AddWorkLayerGroup(Group);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      PreviousDimensionCursorPosition = ptEnd;
    }
  } else {
    PreviousDimensionCursorPosition = ptCur;
  }
}

void AeSysView::OnDimensionModeAngle() {
  CDC* DeviceContext = GetDC();

  auto* Document = GetDocument();
  EoGePoint3d ptCur = GetCursorPosition();

  static EoGePoint3d rProjPt[2];
  static EoGePoint3d ptCen;
  static int iLns;
  static EoGeLine ln;

  if (PreviousDimensionCommand != ID_OP8) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);

    if (SelectLineUsingPoint(ptCur) != 0) {
      EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

      rProjPt[0] = DetPt();
      pLine->GetLine(ln);
      PreviousDimensionCommand = ModeLineHighlightOp(ID_OP8);
      app.AddStringToMessageList(std::wstring(L"Select the second line."));
      iLns = 1;
    }
  } else {
    if (iLns == 1) {
      if (SelectLineUsingPoint(ptCur) != 0) {
        EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

        rProjPt[1] = DetPt();
        if (EoGeLine::Intersection(ln, pLine->Ln(), ptCen)) {
          iLns++;
          app.AddStringToMessageList(std::wstring(L"Specify the location for the dimension arc."));
        }
      }
    } else {
      double dAng;

      EoGeVector3d vCenterToProjPt(ptCen, rProjPt[0]);
      EoGeVector3d vCenterToCur(ptCen, ptCur);
      EoGeVector3d vPlnNorm = EoGeCrossProduct(vCenterToProjPt, vCenterToCur);
      vPlnNorm.Normalize();
      if (SweepAngleFromNormalAnd3Points(vPlnNorm, rProjPt[0], ptCur, rProjPt[1], ptCen, &dAng)) {
        double dRad = EoGeVector3d(ptCen, ptCur).Length();

        ln.begin = ptCen.ProjectToward(rProjPt[0], dRad);
        ln.end = ln.begin.RotateAboutAxis(ptCen, vPlnNorm, dAng);

        EoGeVector3d vXAx = EoGeVector3d(ptCen, ln.begin);
        EoGePoint3d ptRot(ln.begin.RotateAboutAxis(ptCen, vPlnNorm, Eo::HalfPi));
        EoGeVector3d vYAx = EoGeVector3d(ptCen, ptRot);
        EoGePoint3d ptArrow = ln.begin.RotateAboutAxis(ptCen, vPlnNorm, Eo::Radian);

        auto* Group = new EoDbGroup;
        GenerateLineEndItem(1, 0.1, ptArrow, ln.begin, Group);
        Group->AddTail(new EoDbEllipse(1, 1, ptCen, vXAx, vYAx, dAng));
        ptArrow = ln.begin.RotateAboutAxis(ptCen, vPlnNorm, dAng - Eo::Radian);
        GenerateLineEndItem(1, 0.1, ptArrow, ln.end, Group);

        int PrimitiveState = pstate.Save();

        EoDbFontDefinition fd;
        pstate.GetFontDef(fd);
        fd.HorizontalAlignment(EoDb::kAlignCenter);
        fd.VerticalAlignment(EoDb::kAlignMiddle);
        pstate.SetFontDef(DeviceContext, fd);

        EoDbCharacterCellDefinition ccd;
        pstate.GetCharCellDef(ccd);
        ccd.TextRotAngSet(0.0);
        ccd.ChrHgtSet(0.1);
        pstate.SetCharCellDef(ccd);

        EoGePoint3d ptPvt = ptCur.ProjectToward(ptCen, -0.25);
        CharCellDef_EncdRefSys(vPlnNorm, ccd, vXAx, vYAx);
        EoGeReferenceSystem ReferenceSystem(ptPvt, vXAx, vYAx);
        CString Note;
        app.FormatAngle(Note, dAng, 8, 3);
        Group->AddTail(new EoDbText(fd, ReferenceSystem, Note));
        Document->AddWorkLayerGroup(Group);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        pstate.Restore(DeviceContext, PrimitiveState);
      }
      ModeLineUnhighlightOp(PreviousDimensionCommand);
      app.AddModeInformationToMessageList();
    }
  }
}

void AeSysView::OnDimensionModeConvert() {
  EoGePoint3d ptCur = GetCursorPosition();
  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }

  EoDbGroup* Group{};
  EoDbPrimitive* Primitive;
  EoGePoint3d ptProj;

  POSITION posPrimCur;

  EoGePoint4d ptView(ptCur);
  ModelViewTransformPoint(ptView);

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    Group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      posPrimCur = PrimitivePosition;
      Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->SelectUsingPoint(this, ptView, ptProj)) {
        EoGeLine ln;

        if (Primitive->Is(EoDb::kLinePrimitive)) {
          EoDbLine* pPrimLine = static_cast<EoDbLine*>(Primitive);
          pPrimLine->GetLine(ln);
          EoDbDimension* pPrimDim = new EoDbDimension(pPrimLine->Color(), pPrimLine->LineTypeIndex(), ln);
          pPrimDim->SetTextPenColor(5);
          pPrimDim->SetTextHorAlign(EoDb::kAlignCenter);
          pPrimDim->SetTextVerAlign(EoDb::kAlignMiddle);
          Group->InsertAfter(posPrimCur, pPrimDim);
          Group->RemoveAt(posPrimCur);
          delete Primitive;
          PreviousDimensionCursorPosition = ptProj;
          return;
        } else if (Primitive->Is(EoDb::kDimensionPrimitive)) {
          EoDbDimension* pPrimDim = static_cast<EoDbDimension*>(Primitive);
          EoGeReferenceSystem ReferenceSystem;
          pPrimDim->GetRefSys(ReferenceSystem);
          EoDbLine* pPrimLine = new EoDbLine(pPrimDim->Color(), pPrimDim->LineTypeIndex(), pPrimDim->Line());
          EoDbText* pPrimText = new EoDbText(pPrimDim->FontDef(), ReferenceSystem, pPrimDim->Text());
          pPrimText->SetColor(pPrimDim->TextPenColor());
          Group->InsertAfter(posPrimCur, pPrimLine);
          Group->InsertAfter(posPrimCur, pPrimText);
          Group->RemoveAt(posPrimCur);
          delete Primitive;
          PreviousDimensionCursorPosition = ptProj;
          return;
        }
      }
    }
  }
  PreviousDimensionCursorPosition = ptCur;
}

void AeSysView::OnDimensionModeReturn() {
  EoGePoint3d ptCur = GetCursorPosition();
  if (PreviousDimensionCommand != 0) {
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousDimensionCommand);
  }
  PreviousDimensionCursorPosition = ptCur;
}

void AeSysView::OnDimensionModeEscape() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(PreviousDimensionCommand);
}
