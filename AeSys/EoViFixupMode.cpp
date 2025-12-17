#include "stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbEllipse.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDlgFixupOptions.h"

EoUInt16 PreviousFixupCommand = 0;

EoDbGroup* pSegPrv;
EoDbGroup* pSegRef;
EoDbGroup* pSegSec;

EoDbPrimitive* pPrimPrv;
EoDbPrimitive* pPrimRef;
EoDbPrimitive* pPrimSec;

EoGeLine lnPrv;
EoGeLine lnRef;
EoGeLine lnSec;

void AeSysView::OnFixupModeOptions() {
  EoDlgFixupOptions Dialog;
  Dialog.m_FixupAxisTolerance = m_FixupModeAxisTolerance;
  Dialog.m_FixupModeCornerSize = m_FixupModeCornerSize;
  if (Dialog.DoModal() == IDOK) {
    m_FixupModeCornerSize = std::max(0.0, Dialog.m_FixupModeCornerSize);
    m_FixupModeAxisTolerance = std::max(0.0, Dialog.m_FixupAxisTolerance);
    SetAxisConstraintInfluenceAngle(m_FixupModeAxisTolerance);
  }
}

void AeSysView::OnFixupModeReference() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoGePoint3d ptInt;
  EoGePoint3d ptCP;

  EoGeVector3d vMinAx;
  EoGeVector3d vMajAx;
  EoGeVector3d vPlnNorm;

  if (pSegRef != 0) { Document->UpdateAllViews(nullptr, EoDb::kPrimitive, pPrimRef); }
  pSegRef = SelectGroupAndPrimitive(ptCurPos);
  if (pSegRef == 0) { return; }
  pPrimRef = EngagedPrimitive();
  if (!pPrimRef->Is(EoDb::kLinePrimitive)) { return; }
  ptCurPos = DetPt();
  static_cast<EoDbLine*>(pPrimRef)->GetLine(lnRef);

  if (PreviousFixupCommand == 0)
    PreviousFixupCommand = ModeLineHighlightOp(ID_OP1);
  else if (PreviousFixupCommand == ID_OP1)
    ;
  else {
    EoDbLine* pLinePrv = static_cast<EoDbLine*>(pPrimPrv);
    if (!EoGeLine::Intersection(lnPrv, lnRef, ptInt)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (PreviousFixupCommand == ID_OP2) {
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
      if (EoGeVector3d(pLinePrv->EndPoint(), ptInt).Length() < EoGeVector3d(pLinePrv->EndPoint(), ptInt).Length())
        pLinePrv->BeginPoint(ptInt);
      else
        pLinePrv->EndPoint(ptInt);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
    } else if (PreviousFixupCommand == ID_OP3) {
      if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
      lnPrv.end = ptInt;
      if (EoGeVector3d(lnRef.end, ptInt).Length() < EoGeVector3d(lnRef.begin, ptInt).Length()) lnRef.end = lnRef.begin;
      lnRef.begin = ptInt;
      if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, lnRef.begin, lnRef.end, &ptCP)) {
        lnPrv.end = lnPrv.ProjPt(ptCP);
        lnRef.begin = lnRef.ProjPt(ptCP);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLinePrv->BeginPoint(lnPrv.begin);
        pLinePrv->EndPoint(lnPrv.end);
        pSegPrv->AddTail(new EoDbLine(lnPrv.end, lnRef.begin));
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      }
    } else if (PreviousFixupCommand == ID_OP4) {
      if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
      lnPrv.end = ptInt;
      if (EoGeVector3d(lnRef.end, ptInt).Length() < EoGeVector3d(lnRef.begin, ptInt).Length()) lnRef.end = lnRef.begin;
      lnRef.begin = ptInt;
      if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, lnRef.begin, lnRef.end, &ptCP)) {
        double dAng;
        lnPrv.end = lnPrv.ProjPt(ptCP);
        lnRef.begin = lnRef.ProjPt(ptCP);

        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLinePrv->BeginPoint(lnPrv.begin);
        pLinePrv->EndPoint(lnPrv.end);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);

        EoGeVector3d rPrvEndInter(lnPrv.end, ptInt);
        EoGeVector3d rPrvEndRefBeg(lnPrv.end, lnRef.begin);
        vPlnNorm = EoGeCrossProduct(rPrvEndInter, rPrvEndRefBeg);
        vPlnNorm.Normalize();
        pFndSwpAngGivPlnAnd3Lns(vPlnNorm, lnPrv.end, ptInt, lnRef.begin, ptCP, &dAng);
        vMajAx = EoGeVector3d(ptCP, lnPrv.end);
        EoGePoint3d rTmp = lnPrv.end.RotateAboutAxis(ptCP, vPlnNorm, Eo::HalfPi);
        vMinAx = EoGeVector3d(ptCP, rTmp);

        EoDbGroup* Group = new EoDbGroup(new EoDbEllipse(ptCP, vMajAx, vMinAx, dAng));
        Document->AddWorkLayerGroup(Group);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    ModeLineUnhighlightOp(PreviousFixupCommand);
  }
}
void AeSysView::OnFixupModeMend() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoGePoint3d ptInt;
  EoGePoint3d ptCP;

  EoGeVector3d vMinAx;
  EoGeVector3d vMajAx;
  EoGeVector3d vPlnNorm;

  EoDbLine* pLine;

  pSegSec = SelectGroupAndPrimitive(ptCurPos);
  if (pSegSec == 0) { return; }
  pPrimSec = EngagedPrimitive();
  pLine = static_cast<EoDbLine*>(pPrimSec);

  pLine->GetLine(lnSec);

  if (PreviousFixupCommand == 0) {
    pSegPrv = pSegSec;
    pPrimPrv = pPrimSec;
    lnPrv.begin = lnSec.begin;
    lnPrv.end = lnSec.end;
    PreviousFixupCommand = ModeLineHighlightOp(ID_OP2);
  } else if (PreviousFixupCommand == ID_OP1) {
    if (!EoGeLine::Intersection(lnRef, lnSec, ptInt)) {
      app.AddModeInformationToMessageList();
      return;
    }
    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
    if (EoGeVector3d(pLine->BeginPoint(), ptInt).Length() < EoGeVector3d(pLine->EndPoint(), ptInt).Length())
      pLine->BeginPoint(ptInt);
    else
      pLine->EndPoint(ptInt);
    Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
  } else {
    if (!EoGeLine::Intersection(lnPrv, lnSec, ptInt)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (PreviousFixupCommand == ID_OP2) {
      pLine = static_cast<EoDbLine*>(pPrimPrv);
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
      if (EoGeVector3d(pLine->BeginPoint(), ptInt).Length() < EoGeVector3d(pLine->EndPoint(), ptInt).Length())
        pLine->BeginPoint(ptInt);
      else
        pLine->EndPoint(ptInt);
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
    } else if (PreviousFixupCommand == ID_OP3) {
      if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
      lnPrv.end = ptInt;
      if (EoGeVector3d(lnSec.end, ptInt).Length() < EoGeVector3d(lnSec.begin, ptInt).Length()) lnSec.end = lnSec.begin;
      lnSec.begin = ptInt;
      if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, lnSec.begin, lnSec.end, &ptCP)) {
        pLine = static_cast<EoDbLine*>(pPrimPrv);
        lnPrv.end = lnPrv.ProjPt(ptCP);
        lnSec.begin = lnSec.ProjPt(ptCP);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLine->BeginPoint(lnPrv.begin);
        pLine->EndPoint(lnPrv.end);
        pSegPrv->AddTail(new EoDbLine(lnPrv.end, lnSec.begin));
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      }
    } else if (PreviousFixupCommand == ID_OP4) {
      if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
      lnPrv.end = ptInt;
      if (EoGeVector3d(lnSec.end, ptInt).Length() < EoGeVector3d(lnSec.begin, ptInt).Length()) lnSec.end = lnSec.begin;
      lnSec.begin = ptInt;
      if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, lnSec.begin, lnSec.end, &ptCP)) {
        double dAng;
        pLine = static_cast<EoDbLine*>(pPrimPrv);
        lnPrv.end = lnPrv.ProjPt(ptCP);
        lnSec.begin = lnSec.ProjPt(ptCP);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLine->BeginPoint(lnPrv.begin);
        pLine->EndPoint(lnPrv.end);
        EoGeVector3d rPrvEndInter(lnPrv.end, ptInt);
        EoGeVector3d rPrvEndSecBeg(lnPrv.end, lnSec.begin);
        vPlnNorm = EoGeCrossProduct(rPrvEndInter, rPrvEndSecBeg);
        vPlnNorm.Normalize();
        pFndSwpAngGivPlnAnd3Lns(vPlnNorm, lnPrv.end, ptInt, lnSec.begin, ptCP, &dAng);
        vMajAx = EoGeVector3d(ptCP, lnPrv.end);
        EoGePoint3d rTmp = lnPrv.end.RotateAboutAxis(ptCP, vPlnNorm, Eo::HalfPi);
        vMinAx = EoGeVector3d(ptCP, rTmp);
        pSegPrv->AddTail(new EoDbEllipse(ptCP, vMajAx, vMinAx, dAng));
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      }
    }
    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
    pLine = static_cast<EoDbLine*>(pPrimSec);
    if (EoGeVector3d(pLine->BeginPoint(), ptInt).Length() < EoGeVector3d(pLine->EndPoint(), ptInt).Length())
      pLine->BeginPoint(ptInt);
    else
      pLine->EndPoint(ptInt);
    Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
    ModeLineUnhighlightOp(PreviousFixupCommand);
  }
}
void AeSysView::OnFixupModeChamfer() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoGePoint3d ptInt;
  EoGePoint3d ptCP;

  EoGeVector3d vMinAx;
  EoGeVector3d vMajAx;
  EoGeVector3d vPlnNorm;

  EoDbLine* pLine;

  pSegSec = SelectGroupAndPrimitive(ptCurPos);
  pPrimSec = EngagedPrimitive();
  pLine = static_cast<EoDbLine*>(pPrimSec);
  pLine->GetLine(lnSec);

  if (PreviousFixupCommand == 0) {
    pSegPrv = pSegSec;
    pPrimPrv = pPrimSec;
    lnPrv = lnSec;
    PreviousFixupCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (PreviousFixupCommand == ID_OP1) {
      pSegPrv = pSegSec;
      pPrimPrv = pPrimSec;
      lnPrv = lnRef;
    }
    if (!EoGeLine::Intersection(lnPrv, lnSec, ptInt)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
    lnPrv.end = ptInt;
    if (EoGeVector3d(lnSec.end, ptInt).Length() < EoGeVector3d(lnSec.begin, ptInt).Length()) lnSec.end = lnSec.begin;
    lnSec.begin = ptInt;
    if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, lnSec.begin, lnSec.end,
                            &ptCP)) {  // Center point is defined .. determine arc endpoints
      lnPrv.end = lnPrv.ProjPt(ptCP);
      lnSec.begin = lnSec.ProjPt(ptCP);
      if (PreviousFixupCommand == ID_OP1)
        ;
      else if (PreviousFixupCommand == ID_OP2) {
        pLine = static_cast<EoDbLine*>(pPrimPrv);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLine->BeginPoint(lnPrv.begin);
        pLine->EndPoint(ptInt);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      } else if (PreviousFixupCommand == ID_OP3 || PreviousFixupCommand == ID_OP4) {
        pLine = static_cast<EoDbLine*>(pPrimPrv);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLine->BeginPoint(lnPrv.begin);
        pLine->EndPoint(lnPrv.end);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      }
      pLine = static_cast<EoDbLine*>(pPrimSec);
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
      pLine->BeginPoint(lnSec.begin);
      pLine->EndPoint(lnSec.end);

      pSegSec->AddTail(new EoDbLine(lnPrv.end, lnSec.begin));

      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
    }
    ModeLineUnhighlightOp(PreviousFixupCommand);
  }
}
void AeSysView::OnFixupModeFillet() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoGePoint3d ptInt;
  EoGePoint3d ptCP;

  EoGeVector3d vMinAx;
  EoGeVector3d vMajAx;
  EoGeVector3d vPlnNorm;

  EoDbLine* pLine;

  pSegSec = SelectGroupAndPrimitive(ptCurPos);
  pPrimSec = EngagedPrimitive();
  pLine = static_cast<EoDbLine*>(pPrimSec);
  pLine->GetLine(lnSec);

  if (PreviousFixupCommand == 0) {
    pSegPrv = pSegSec;
    pPrimPrv = pPrimSec;
    lnPrv = lnSec;
    PreviousFixupCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (PreviousFixupCommand == ID_OP1) {
      pSegPrv = pSegSec;
      pPrimPrv = pPrimSec;
      lnPrv = lnRef;
    }
    if (!EoGeLine::Intersection(lnPrv, lnSec, ptInt)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
    lnPrv.end = ptInt;
    if (EoGeVector3d(lnSec.end, ptInt).Length() < EoGeVector3d(lnSec.begin, ptInt).Length()) lnSec.end = lnSec.begin;
    lnSec.begin = ptInt;
    if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, lnSec.begin, lnSec.end,
                            &ptCP)) {  // Center point is defined .. determine arc endpoints
      lnPrv.end = lnPrv.ProjPt(ptCP);
      lnSec.begin = lnSec.ProjPt(ptCP);
      if (PreviousFixupCommand == ID_OP1)
        ;
      else if (PreviousFixupCommand == ID_OP2) {
        pLine = static_cast<EoDbLine*>(pPrimPrv);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLine->BeginPoint(lnPrv.begin);
        pLine->EndPoint(ptInt);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      } else if (PreviousFixupCommand == ID_OP3 || PreviousFixupCommand == ID_OP4) {
        pLine = static_cast<EoDbLine*>(pPrimPrv);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLine->BeginPoint(lnPrv.begin);
        pLine->EndPoint(lnPrv.end);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      }
      pLine = static_cast<EoDbLine*>(pPrimSec);
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
      pLine->BeginPoint(lnSec.begin);
      pLine->EndPoint(lnSec.end);

      double dAng;
      EoGeVector3d rPrvEndInter(lnPrv.end, ptInt);
      EoGeVector3d rPrvEndSecBeg(lnPrv.end, lnSec.begin);
      vPlnNorm = EoGeCrossProduct(rPrvEndInter, rPrvEndSecBeg);
      vPlnNorm.Normalize();
      pFndSwpAngGivPlnAnd3Lns(vPlnNorm, lnPrv.end, ptInt, lnSec.begin, ptCP, &dAng);
      vMajAx = EoGeVector3d(ptCP, lnPrv.end);
      EoGePoint3d rTmp = lnPrv.end.RotateAboutAxis(ptCP, vPlnNorm, Eo::HalfPi);
      vMinAx = EoGeVector3d(ptCP, rTmp);
      pSegSec->AddTail(new EoDbEllipse(ptCP, vMajAx, vMinAx, dAng));

      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
    }
    ModeLineUnhighlightOp(PreviousFixupCommand);
  }
}
void AeSysView::OnFixupModeSquare() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoDbLine* pLine;

  pSegSec = SelectGroupAndPrimitive(ptCurPos);
  if (pSegSec != 0) {
    pPrimSec = EngagedPrimitive();
    ptCurPos = DetPt();
    if (pPrimSec->Is(EoDb::kLinePrimitive)) {
      pLine = static_cast<EoDbLine*>(pPrimSec);
      pLine->GetLine(lnSec);
      double dLen = lnSec.Length();
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
      lnSec.begin = SnapPointToAxis(ptCurPos, lnSec.begin);
      lnSec.end = lnSec.begin.ProjectToward(ptCurPos, dLen);
      pLine->BeginPoint(SnapPointToGrid(lnSec.begin));
      pLine->EndPoint(SnapPointToGrid(lnSec.end));
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
    }
  }
}
void AeSysView::OnFixupModeParallel() {
  AeSysDoc* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoDbLine* pLine;

  pSegSec = SelectGroupAndPrimitive(ptCurPos);
  if (pSegRef != 0 && pSegSec != 0) {
    pPrimSec = EngagedPrimitive();
    if (pPrimSec->Is(EoDb::kLinePrimitive)) {
      pLine = static_cast<EoDbLine*>(pPrimSec);

      lnSec.begin = lnRef.ProjPt(pLine->BeginPoint());
      lnSec.end = lnRef.ProjPt(pLine->EndPoint());
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
      pLine->BeginPoint(lnSec.begin.ProjectToward(pLine->BeginPoint(), app.DimensionLength()));
      pLine->EndPoint(lnSec.end.ProjectToward(pLine->EndPoint(), app.DimensionLength()));
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
    }
  }
}
void AeSysView::OnFixupModeReturn() {
  AeSysDoc* Document = GetDocument();

  if (pSegRef != 0) {
    Document->UpdateAllViews(nullptr, EoDb::kPrimitive, pPrimRef);
    pSegRef = 0;
    pPrimRef = 0;
  }
  ModeLineUnhighlightOp(PreviousFixupCommand);
}
void AeSysView::OnFixupModeEscape() {
  AeSysDoc* Document = GetDocument();

  if (pSegRef != 0) {
    Document->UpdateAllViews(nullptr, EoDb::kPrimitive, pPrimRef);
    pSegRef = 0;
    pPrimRef = 0;
  }
  ModeLineUnhighlightOp(PreviousFixupCommand);
}
