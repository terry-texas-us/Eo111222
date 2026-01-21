#include "Stdafx.h"

#include <Windows.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbEllipse.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDlgFixupOptions.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "Resource.h"

namespace {

EoUInt16 PreviousFixupCommand{};

EoDbGroup* pSegPrv{};
EoDbPrimitive* pPrimPrv{};
EoGeLine lnPrv{};

EoDbGroup* referenceBlock{};
EoDbPrimitive* referencePrimitive{};
EoGeLine referenceLine{};

EoDbGroup* pSegSec{};
EoDbPrimitive* pPrimSec{};
EoGeLine lnSec{};


/** @brief Finds center point of a circle given radius and two tangent vectors.
 * @param radius The radius of the circle.
 * @param arLn1Beg The beginning point of the first line.
 * @param arLn1End The ending point of the first line.
 * @param arLn2Beg The beginning point of the second line.
 * @param arLn2End The ending point of the second line.
 * @param centerPoint Output parameter that receives the center point of the circle.
 * @note A radius and two lines define four center points.  The center point
 *       selected is on the concave side of the angle formed by the two vectors
 *       defined by the line endpoints.	These two vectors are oriented with
 *       the tail of the second vector at the head of the first.
 * @return true if center point found, false otherwise.
 */
bool pFndCPGivRadAnd4Pts(double radius, EoGePoint3d arLn1Beg, EoGePoint3d arLn1End, EoGePoint3d arLn2Beg, EoGePoint3d arLn2End, EoGePoint3d* centerPoint) {
  double dA1, dA2, dB1, dB2, dC1RAB1, dC2RAB2, dDet, dSgnRad, dV1Mag, dV2Mag;
  EoGeVector3d vPlnNorm;

  EoGeVector3d v1(arLn1Beg, arLn1End);  // Determine vector defined by endpoints of first line
  dV1Mag = v1.Length();
  if (dV1Mag <= DBL_EPSILON) return false;

  EoGeVector3d v2(arLn2Beg, arLn2End);
  dV2Mag = v2.Length();
  if (dV2Mag <= DBL_EPSILON) return false;

  vPlnNorm = EoGeCrossProduct(v1, v2);  // Determine vector normal to tangent vectors
  vPlnNorm.Normalize();
  if (vPlnNorm.IsNearNull()) return false;

  if (fabs((EoGeDotProduct(vPlnNorm, EoGeVector3d(arLn1Beg, arLn2Beg)))) > DBL_EPSILON)  // Four points are not coplanar
    return false;

  EoGeTransformMatrix tm(arLn1Beg, vPlnNorm);

  arLn1End = tm * arLn1End;
  arLn2Beg = tm * arLn2Beg;
  arLn2End = tm * arLn2End;
  dA1 = -arLn1End.y / dV1Mag;
  dB1 = arLn1End.x / dV1Mag;
  v2.x = arLn2End.x - arLn2Beg.x;
  v2.y = arLn2End.y - arLn2Beg.y;
  dA2 = -v2.y / dV2Mag;
  dB2 = v2.x / dV2Mag;
  dDet = dA2 * dB1 - dA1 * dB2;

  dSgnRad = (arLn1End.x * arLn2End.y - arLn2End.x * arLn1End.y) >= 0. ? -fabs(radius) : fabs(radius);

  dC1RAB1 = dSgnRad;
  dC2RAB2 = (arLn2Beg.x * arLn2End.y - arLn2End.x * arLn2Beg.y) / dV2Mag + dSgnRad;
  (*centerPoint).x = (dB2 * dC1RAB1 - dB1 * dC2RAB2) / dDet;
  (*centerPoint).y = (dA1 * dC2RAB2 - dA2 * dC1RAB1) / dDet;
  (*centerPoint).z = 0.;
  tm.Inverse();
  *centerPoint = tm * (*centerPoint);
  return true;
}
}  // namespace

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
  auto* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoGePoint3d ptInt;
  EoGePoint3d ptCP;

  EoGeVector3d vMinAx;
  EoGeVector3d vMajAx;
  EoGeVector3d vPlnNorm;

  if (referenceBlock != nullptr) { Document->UpdateAllViews(nullptr, EoDb::kPrimitive, referencePrimitive); }
  referenceBlock = SelectGroupAndPrimitive(ptCurPos);
  if (referenceBlock == nullptr) { return; }
  referencePrimitive = EngagedPrimitive();
  if (!referencePrimitive->Is(EoDb::kLinePrimitive)) { return; }
  ptCurPos = DetPt();
  static_cast<EoDbLine*>(referencePrimitive)->GetLine(referenceLine);

  if (PreviousFixupCommand == 0)
    PreviousFixupCommand = ModeLineHighlightOp(ID_OP1);
  else if (PreviousFixupCommand == ID_OP1)
    ;
  else {
    EoDbLine* pLinePrv = static_cast<EoDbLine*>(pPrimPrv);
    if (!EoGeLine::Intersection(lnPrv, referenceLine, ptInt)) {
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
      if (EoGeVector3d(referenceLine.end, ptInt).Length() < EoGeVector3d(referenceLine.begin, ptInt).Length()) referenceLine.end = referenceLine.begin;
      referenceLine.begin = ptInt;
      if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, referenceLine.begin, referenceLine.end, &ptCP)) {
        lnPrv.end = lnPrv.ProjPt(ptCP);
        referenceLine.begin = referenceLine.ProjPt(ptCP);
        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLinePrv->BeginPoint(lnPrv.begin);
        pLinePrv->EndPoint(lnPrv.end);
        pSegPrv->AddTail(new EoDbLine(lnPrv.end, referenceLine.begin));
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);
      }
    } else if (PreviousFixupCommand == ID_OP4) {
      if (EoGeVector3d(lnPrv.begin, ptInt).Length() < EoGeVector3d(lnPrv.end, ptInt).Length()) lnPrv.begin = lnPrv.end;
      lnPrv.end = ptInt;
      if (EoGeVector3d(referenceLine.end, ptInt).Length() < EoGeVector3d(referenceLine.begin, ptInt).Length()) referenceLine.end = referenceLine.begin;
      referenceLine.begin = ptInt;
      if (pFndCPGivRadAnd4Pts(m_FixupModeCornerSize, lnPrv.begin, lnPrv.end, referenceLine.begin, referenceLine.end, &ptCP)) {
        double dAng;
        lnPrv.end = lnPrv.ProjPt(ptCP);
        referenceLine.begin = referenceLine.ProjPt(ptCP);

        Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegPrv);
        pLinePrv->BeginPoint(lnPrv.begin);
        pLinePrv->EndPoint(lnPrv.end);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegPrv);

        EoGeVector3d rPrvEndInter(lnPrv.end, ptInt);
        EoGeVector3d rPrvEndRefBeg(lnPrv.end, referenceLine.begin);
        vPlnNorm = EoGeCrossProduct(rPrvEndInter, rPrvEndRefBeg);
        vPlnNorm.Normalize();
        SweepAngleFromNormalAnd3Points(vPlnNorm, lnPrv.end, ptInt, referenceLine.begin, ptCP, &dAng);
        vMajAx = EoGeVector3d(ptCP, lnPrv.end);
        EoGePoint3d rTmp = lnPrv.end.RotateAboutAxis(ptCP, vPlnNorm, Eo::HalfPi);
        vMinAx = EoGeVector3d(ptCP, rTmp);

        auto* Group = new EoDbGroup(new EoDbEllipse(ptCP, vMajAx, vMinAx, dAng));
        Document->AddWorkLayerGroup(Group);
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    ModeLineUnhighlightOp(PreviousFixupCommand);
  }
}

void AeSysView::OnFixupModeMend() {
  auto* Document = GetDocument();

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
    if (!EoGeLine::Intersection(referenceLine, lnSec, ptInt)) {
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
        SweepAngleFromNormalAnd3Points(vPlnNorm, lnPrv.end, ptInt, lnSec.begin, ptCP, &dAng);
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
  auto* Document = GetDocument();

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
      lnPrv = referenceLine;
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
  auto* Document = GetDocument();

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
      lnPrv = referenceLine;
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
      SweepAngleFromNormalAnd3Points(vPlnNorm, lnPrv.end, ptInt, lnSec.begin, ptCP, &dAng);
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
  auto* Document = GetDocument();

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
  auto* Document = GetDocument();

  EoGePoint3d ptCurPos = GetCursorPosition();

  EoDbLine* pLine;

  pSegSec = SelectGroupAndPrimitive(ptCurPos);
  if (referenceBlock != nullptr && pSegSec != 0) {
    pPrimSec = EngagedPrimitive();
    if (pPrimSec->Is(EoDb::kLinePrimitive)) {
      pLine = static_cast<EoDbLine*>(pPrimSec);

      lnSec.begin = referenceLine.ProjPt(pLine->BeginPoint());
      lnSec.end = referenceLine.ProjPt(pLine->EndPoint());
      Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, pSegSec);
      pLine->BeginPoint(lnSec.begin.ProjectToward(pLine->BeginPoint(), app.DimensionLength()));
      pLine->EndPoint(lnSec.end.ProjectToward(pLine->EndPoint(), app.DimensionLength()));
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, pSegSec);
    }
  }
}

void AeSysView::OnFixupModeReturn() {
  auto* Document = GetDocument();

  if (referenceBlock != nullptr) {
    Document->UpdateAllViews(nullptr, EoDb::kPrimitive, referencePrimitive);
    referenceBlock = nullptr;
    referencePrimitive = nullptr;
  }
  ModeLineUnhighlightOp(PreviousFixupCommand);
}

void AeSysView::OnFixupModeEscape() {
  auto* Document = GetDocument();

  if (referenceBlock != nullptr) {
    Document->UpdateAllViews(nullptr, EoDb::kPrimitive, referencePrimitive);
    referenceBlock = nullptr;
    referencePrimitive = nullptr;
  }
  ModeLineUnhighlightOp(PreviousFixupCommand);
}
