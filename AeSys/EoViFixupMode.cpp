#include "Stdafx.h"

#include <algorithm>
#include <cmath>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPrimitive.h"
#include "EoDlgFixupOptions.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderState.h"
#include "Resource.h"

/** @todo Color and lineType assignment for chamfer/fillet operations
 *
 * Determine color and lineTypeIndex for newly created chamfer/fillet primitives:
 * - Option 1: Use global primitive state (pstate)
 * - Option 2: Inherit from one of the two source lines
 * 
 * @attention If one line is a reference line, the non-reference line should 
 * define properties.
 * 
 * @note When both lines are trimmed and have different properties, should we 
 * prompt the user? Currently defaults to current line properties.
 */

namespace {

std::uint16_t previousCommand{};

EoDbGroup* previousGroup{};
EoDbPrimitive* previousPrimitive{};
EoGeLine previousLine{};

EoDbGroup* referenceGroup{};
EoDbPrimitive* referencePrimitive{};
EoGeLine referenceLine{};

EoDbGroup* currentGroup{};
EoDbPrimitive* currentPrimitive{};
EoGeLine currentLine{};

/** @brief Finds center point of a circle given radius and two tangent vectors.
 * @param radius The radius of the circle.
 * @param firstLine The first line.
 * @param secondLine The second line.
 * @param[out] center computed center point of the circle.
 * @note A radius and two lines define four center points.  The center point
 *       selected is on the concave side of the angle formed by the two vectors
 *       defined by the line endpoints.	These two vectors are oriented with
 *       the tail of the second vector at the head of the first.
 * @return true if center point found, false otherwise.
 */
[[nodiscard]] bool FindCenterFromRadiusAnd4Points(double radius, EoGeLine firstLine, EoGeLine secondLine, EoGePoint3d* center) {
  EoGeVector3d u(firstLine.begin, firstLine.end);  // Determine vector defined by endpoints of first line
  double firstLineLength = u.Length();
  if (firstLineLength < Eo::geometricTolerance) { return false; }

  EoGeVector3d v(secondLine.begin, secondLine.end);
  double secondLineLength = v.Length();
  if (secondLineLength < Eo::geometricTolerance) { return false; }

  auto normal = CrossProduct(u, v);  // Determine vector normal to tangent vectors
  normal.Normalize();
  if (normal.IsNearNull()) { return false; }

  if (fabs((DotProduct(normal, EoGeVector3d(firstLine.begin, secondLine.begin)))) > Eo::geometricTolerance) {
    // Four points are not coplanar
    return false;
  }

  EoGeTransformMatrix transformMatrix(firstLine.begin, normal);

  firstLine.end = transformMatrix * firstLine.end;
  secondLine.begin = transformMatrix * secondLine.begin;
  secondLine.end = transformMatrix * secondLine.end;
  double dA1 = -firstLine.end.y / firstLineLength;
  double dB1 = firstLine.end.x / firstLineLength;
  v.x = secondLine.end.x - secondLine.begin.x;
  v.y = secondLine.end.y - secondLine.begin.y;
  double dA2 = -v.y / secondLineLength;
  double dB2 = v.x / secondLineLength;
  double dDet = dA2 * dB1 - dA1 * dB2;

  double dSgnRad =
      (firstLine.end.x * secondLine.end.y - secondLine.end.x * firstLine.end.y) >= 0. ? -fabs(radius) : fabs(radius);

  double dC1RAB1 = dSgnRad;
  double dC2RAB2 =
      (secondLine.begin.x * secondLine.end.y - secondLine.end.x * secondLine.begin.y) / secondLineLength + dSgnRad;
  (*center).x = (dB2 * dC1RAB1 - dB1 * dC2RAB2) / dDet;
  (*center).y = (dA1 * dC2RAB2 - dA2 * dC1RAB1) / dDet;
  (*center).z = 0.;
  transformMatrix.Inverse();
  *center = transformMatrix * (*center);
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
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  EoGePoint3d center;

  if (referenceGroup != nullptr) { document->UpdateAllViews(nullptr, EoDb::kPrimitive, referencePrimitive); }
  referenceGroup = SelectGroupAndPrimitive(cursorPosition);
  if (referenceGroup == nullptr) { return; }
  referencePrimitive = EngagedPrimitive();
  if (!referencePrimitive->Is(EoDb::kLinePrimitive)) { return; }
  cursorPosition = DetPt();
  referenceLine = static_cast<EoDbLine*>(referencePrimitive)->Line();

  if (previousCommand == 0)
    previousCommand = ModeLineHighlightOp(ID_OP1);
  else if (previousCommand == ID_OP1)
    ;
  else {
    auto* line = static_cast<EoDbLine*>(previousPrimitive);
    EoGePoint3d intersection;
    if (!EoGeLine::Intersection(previousLine, referenceLine, intersection)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (previousCommand == ID_OP2) {
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
      if (EoGeVector3d(line->End(), intersection).Length() < EoGeVector3d(line->End(), intersection).Length())
        line->SetBeginPoint(intersection);
      else
        line->SetEndPoint(intersection);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
    } else if (previousCommand == ID_OP3) {
      if (EoGeVector3d(previousLine.begin, intersection).Length() <
          EoGeVector3d(previousLine.end, intersection).Length())
        previousLine.begin = previousLine.end;
      previousLine.end = intersection;
      if (EoGeVector3d(referenceLine.end, intersection).Length() <
          EoGeVector3d(referenceLine.begin, intersection).Length())
        referenceLine.end = referenceLine.begin;
      referenceLine.begin = intersection;
      if (FindCenterFromRadiusAnd4Points(m_FixupModeCornerSize, previousLine, referenceLine, &center)) {
        previousLine.end = previousLine.ProjectPointToLine(center);
        referenceLine.begin = referenceLine.ProjectPointToLine(center);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        line->SetBeginPoint(previousLine.begin);
        line->SetEndPoint(previousLine.end);
        previousGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), previousLine.end, referenceLine.begin));
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
      }
    } else if (previousCommand == ID_OP4) {
      if (EoGeVector3d(previousLine.begin, intersection).Length() <
          EoGeVector3d(previousLine.end, intersection).Length())
        previousLine.begin = previousLine.end;
      previousLine.end = intersection;
      if (EoGeVector3d(referenceLine.end, intersection).Length() <
          EoGeVector3d(referenceLine.begin, intersection).Length())
        referenceLine.end = referenceLine.begin;
      referenceLine.begin = intersection;
      if (FindCenterFromRadiusAnd4Points(m_FixupModeCornerSize, previousLine, referenceLine, &center)) {
        double angle;
        previousLine.end = previousLine.ProjectPointToLine(center);
        referenceLine.begin = referenceLine.ProjectPointToLine(center);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        line->SetBeginPoint(previousLine.begin);
        line->SetEndPoint(previousLine.end);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);

        EoGeVector3d previousEndToIntersection(previousLine.end, intersection);
        EoGeVector3d previousEndToReferenceBegin(previousLine.end, referenceLine.begin);
        auto normal = CrossProduct(previousEndToIntersection, previousEndToReferenceBegin);
        normal.Normalize();
        if (SweepAngleFromNormalAnd3Points(normal, previousLine.end, intersection, referenceLine.begin, center,
                                           angle)) {
          auto majorAxis = EoGeVector3d(center, previousLine.end);
          auto minorAxis = CrossProduct(normal, majorAxis);

          auto* radialArc = EoDbConic::CreateConicFromEllipsePrimitive(center, majorAxis, minorAxis, angle);
          radialArc->SetColor(line->Color());
          radialArc->SetLineTypeIndex(line->LineTypeIndex());

          auto* group = new EoDbGroup(radialArc);
          document->AddWorkLayerGroup(group);
          document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
        }
      }
    }
    ModeLineUnhighlightOp(previousCommand);
  }
}

void AeSysView::OnFixupModeMend() {
  auto* document = GetDocument();

  auto cursorPosition = GetCursorPosition();

  EoGePoint3d intersection;
  EoGePoint3d center;

  currentGroup = SelectGroupAndPrimitive(cursorPosition);
  if (currentGroup == nullptr) { return; }
  currentPrimitive = EngagedPrimitive();

  auto* pLine = static_cast<EoDbLine*>(currentPrimitive);
  currentLine = pLine->Line();

  if (previousCommand == 0) {
    previousGroup = currentGroup;
    previousPrimitive = currentPrimitive;
    previousLine.begin = currentLine.begin;
    previousLine.end = currentLine.end;
    previousCommand = ModeLineHighlightOp(ID_OP2);
  } else if (previousCommand == ID_OP1) {
    if (!EoGeLine::Intersection(referenceLine, currentLine, intersection)) {
      app.AddModeInformationToMessageList();
      return;
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, currentGroup);
    if (EoGeVector3d(pLine->Begin(), intersection).Length() <
        EoGeVector3d(pLine->End(), intersection).Length())
      pLine->SetBeginPoint(intersection);
    else
      pLine->SetEndPoint(intersection);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, currentGroup);
  } else {
    if (!EoGeLine::Intersection(previousLine, currentLine, intersection)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (previousCommand == ID_OP2) {
      pLine = static_cast<EoDbLine*>(previousPrimitive);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
      if (EoGeVector3d(pLine->Begin(), intersection).Length() <
          EoGeVector3d(pLine->End(), intersection).Length())
        pLine->SetBeginPoint(intersection);
      else
        pLine->SetEndPoint(intersection);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
    } else if (previousCommand == ID_OP3) {
      if (EoGeVector3d(previousLine.begin, intersection).Length() <
          EoGeVector3d(previousLine.end, intersection).Length())
        previousLine.begin = previousLine.end;
      previousLine.end = intersection;
      if (EoGeVector3d(currentLine.end, intersection).Length() < EoGeVector3d(currentLine.begin, intersection).Length())
        currentLine.end = currentLine.begin;
      currentLine.begin = intersection;
      if (FindCenterFromRadiusAnd4Points(m_FixupModeCornerSize, previousLine, currentLine, &center)) {
        pLine = static_cast<EoDbLine*>(previousPrimitive);
        previousLine.end = previousLine.ProjectPointToLine(center);
        currentLine.begin = currentLine.ProjectPointToLine(center);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        pLine->SetBeginPoint(previousLine.begin);
        pLine->SetEndPoint(previousLine.end);
        previousGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), previousLine.end, currentLine.begin));
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
      }
    } else if (previousCommand == ID_OP4) {
      if (EoGeVector3d(previousLine.begin, intersection).Length() <
          EoGeVector3d(previousLine.end, intersection).Length())
        previousLine.begin = previousLine.end;
      previousLine.end = intersection;
      if (EoGeVector3d(currentLine.end, intersection).Length() < EoGeVector3d(currentLine.begin, intersection).Length())
        currentLine.end = currentLine.begin;
      currentLine.begin = intersection;
      if (FindCenterFromRadiusAnd4Points(m_FixupModeCornerSize, previousLine, currentLine, &center)) {
        pLine = static_cast<EoDbLine*>(previousPrimitive);
        previousLine.end = previousLine.ProjectPointToLine(center);
        currentLine.begin = currentLine.ProjectPointToLine(center);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        pLine->SetBeginPoint(previousLine.begin);
        pLine->SetEndPoint(previousLine.end);
        EoGeVector3d rPrvEndInter(previousLine.end, intersection);
        EoGeVector3d rPrvEndSecBeg(previousLine.end, currentLine.begin);
        EoGeVector3d normal = CrossProduct(rPrvEndInter, rPrvEndSecBeg);
        normal.Normalize();
        double angle{};
        if (SweepAngleFromNormalAnd3Points(normal, previousLine.end, intersection, currentLine.begin, center, angle)) {
          auto majorAxis = EoGeVector3d(center, previousLine.end);
          auto minorAxis = CrossProduct(normal, majorAxis);

          auto* radialArc = EoDbConic::CreateConicFromEllipsePrimitive(center, majorAxis, minorAxis, angle);
          radialArc->SetColor(pstate.Color());
          radialArc->SetLineTypeIndex(pstate.LineTypeIndex());

          previousGroup->AddTail(radialArc);
          document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
        }
      }
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, currentGroup);
    pLine = static_cast<EoDbLine*>(currentPrimitive);
    if (EoGeVector3d(pLine->Begin(), intersection).Length() <
        EoGeVector3d(pLine->End(), intersection).Length())
      pLine->SetBeginPoint(intersection);
    else
      pLine->SetEndPoint(intersection);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, currentGroup);
    ModeLineUnhighlightOp(previousCommand);
  }
}

void AeSysView::OnFixupModeChamfer() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  currentGroup = SelectGroupAndPrimitive(cursorPosition);
  currentPrimitive = EngagedPrimitive();
  EoDbLine* linePrimitive = static_cast<EoDbLine*>(currentPrimitive);
  currentLine = linePrimitive->Line();

  if (previousCommand == 0) {
    previousGroup = currentGroup;
    previousPrimitive = currentPrimitive;
    previousLine = currentLine;
    previousCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (previousCommand == ID_OP1) {
      previousGroup = currentGroup;
      previousPrimitive = currentPrimitive;
      previousLine = referenceLine;
    }
    EoGePoint3d intersection{};
    if (!EoGeLine::Intersection(previousLine, currentLine, intersection)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (EoGeVector3d(previousLine.begin, intersection).Length() < EoGeVector3d(previousLine.end, intersection).Length())
      previousLine.begin = previousLine.end;
    previousLine.end = intersection;
    if (EoGeVector3d(currentLine.end, intersection).Length() < EoGeVector3d(currentLine.begin, intersection).Length())
      currentLine.end = currentLine.begin;
    currentLine.begin = intersection;
    EoGePoint3d center;
    if (FindCenterFromRadiusAnd4Points(m_FixupModeCornerSize, previousLine, currentLine, &center)) {
      // Center point is defined .. determine arc endpoints
      previousLine.end = previousLine.ProjectPointToLine(center);
      currentLine.begin = currentLine.ProjectPointToLine(center);
      if (previousCommand == ID_OP1)
        ;
      else if (previousCommand == ID_OP2) {
        linePrimitive = static_cast<EoDbLine*>(previousPrimitive);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        linePrimitive->SetBeginPoint(previousLine.begin);
        linePrimitive->SetEndPoint(intersection);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
      } else if (previousCommand == ID_OP3 || previousCommand == ID_OP4) {
        linePrimitive = static_cast<EoDbLine*>(previousPrimitive);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        linePrimitive->SetBeginPoint(previousLine.begin);
        linePrimitive->SetEndPoint(previousLine.end);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
      }
      linePrimitive = static_cast<EoDbLine*>(currentPrimitive);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, currentGroup);
      linePrimitive->SetBeginPoint(currentLine.begin);
      linePrimitive->SetEndPoint(currentLine.end);

      currentGroup->AddTail(new EoDbLine(pstate.Color(), pstate.LineTypeIndex(), previousLine.end, currentLine.begin));

      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, currentGroup);
    }
    ModeLineUnhighlightOp(previousCommand);
  }
}

void AeSysView::OnFixupModeFillet() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  currentGroup = SelectGroupAndPrimitive(cursorPosition);
  currentPrimitive = EngagedPrimitive();
  EoDbLine* linePrimitive = static_cast<EoDbLine*>(currentPrimitive);
  currentLine = linePrimitive->Line();

  if (previousCommand == 0) {
    previousGroup = currentGroup;
    previousPrimitive = currentPrimitive;
    previousLine = currentLine;
    previousCommand = ModeLineHighlightOp(ID_OP4);
  } else {
    if (previousCommand == ID_OP1) {
      previousGroup = currentGroup;
      previousPrimitive = currentPrimitive;
      previousLine = referenceLine;
    }
    EoGePoint3d intersection{};
    if (!EoGeLine::Intersection(previousLine, currentLine, intersection)) {
      app.AddModeInformationToMessageList();
      return;
    }
    if (EoGeVector3d(previousLine.begin, intersection).Length() < EoGeVector3d(previousLine.end, intersection).Length())
      previousLine.begin = previousLine.end;
    previousLine.end = intersection;
    if (EoGeVector3d(currentLine.end, intersection).Length() < EoGeVector3d(currentLine.begin, intersection).Length())
      currentLine.end = currentLine.begin;
    currentLine.begin = intersection;

    EoGePoint3d center;
    if (FindCenterFromRadiusAnd4Points(m_FixupModeCornerSize, previousLine, currentLine, &center)) {
      // Center point is defined .. determine arc endpoints
      previousLine.end = previousLine.ProjectPointToLine(center);
      currentLine.begin = currentLine.ProjectPointToLine(center);
      if (previousCommand == ID_OP1)
        ;
      else if (previousCommand == ID_OP2) {
        linePrimitive = static_cast<EoDbLine*>(previousPrimitive);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        linePrimitive->SetBeginPoint(previousLine.begin);
        linePrimitive->SetEndPoint(intersection);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
      } else if (previousCommand == ID_OP3 || previousCommand == ID_OP4) {
        linePrimitive = static_cast<EoDbLine*>(previousPrimitive);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, previousGroup);
        linePrimitive->SetBeginPoint(previousLine.begin);
        linePrimitive->SetEndPoint(previousLine.end);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, previousGroup);
      }
      linePrimitive = static_cast<EoDbLine*>(currentPrimitive);
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, currentGroup);
      linePrimitive->SetBeginPoint(currentLine.begin);
      linePrimitive->SetEndPoint(currentLine.end);

      EoGeVector3d normal;
      EoGeVector3d previousEndToIntersection(previousLine.end, intersection);
      EoGeVector3d previousEndToCurrentBegin(previousLine.end, currentLine.begin);
      normal = CrossProduct(previousEndToIntersection, previousEndToCurrentBegin);
      normal.Normalize();
      double angle;
      if (SweepAngleFromNormalAnd3Points(normal, previousLine.end, intersection, currentLine.begin, center, angle)) {
        auto majorAxis = EoGeVector3d(center, previousLine.end);
        auto minorAxis = CrossProduct(normal, majorAxis);

        // auto* radialArc = EoDbConic::CreateRadialArc(center, normal, majorAxis.Length(), 0.0, angle);
        auto* radialArc = EoDbConic::CreateConicFromEllipsePrimitive(center, majorAxis, minorAxis, angle);
        radialArc->SetColor(linePrimitive->Color());
        radialArc->SetLineTypeIndex(linePrimitive->LineTypeIndex());
        
        currentGroup->AddTail(radialArc);
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, currentGroup);
      }
    }
    ModeLineUnhighlightOp(previousCommand);
  }
}

void AeSysView::OnFixupModeSquare() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  EoDbLine* linePrimitive{};

  currentGroup = SelectGroupAndPrimitive(cursorPosition);
  if (currentGroup == nullptr) { return; }
  currentPrimitive = EngagedPrimitive();
  cursorPosition = DetPt();
  if (!currentPrimitive->Is(EoDb::kLinePrimitive)) { return; }

  linePrimitive = static_cast<EoDbLine*>(currentPrimitive);
  currentLine = linePrimitive->Line();
  double dLen = currentLine.Length();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, currentGroup);
  currentLine.begin = SnapPointToAxis(cursorPosition, currentLine.begin);
  currentLine.end = currentLine.begin.ProjectToward(cursorPosition, dLen);
  linePrimitive->SetBeginPoint(SnapPointToGrid(currentLine.begin));
  linePrimitive->SetEndPoint(SnapPointToGrid(currentLine.end));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, currentGroup);
}

void AeSysView::OnFixupModeParallel() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  EoDbLine* pLine;

  currentGroup = SelectGroupAndPrimitive(cursorPosition);
  if (referenceGroup != nullptr && currentGroup != nullptr) {
    currentPrimitive = EngagedPrimitive();
    if (currentPrimitive->Is(EoDb::kLinePrimitive)) {
      pLine = static_cast<EoDbLine*>(currentPrimitive);

      currentLine.begin = referenceLine.ProjectPointToLine(pLine->Begin());
      currentLine.end = referenceLine.ProjectPointToLine(pLine->End());
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, currentGroup);
      pLine->SetBeginPoint(currentLine.begin.ProjectToward(pLine->Begin(), app.DimensionLength()));
      pLine->SetEndPoint(currentLine.end.ProjectToward(pLine->End(), app.DimensionLength()));
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, currentGroup);
    }
  }
}

void AeSysView::OnFixupModeReturn() {
  auto* document = GetDocument();

  if (referenceGroup != nullptr) {
    document->UpdateAllViews(nullptr, EoDb::kPrimitive, referencePrimitive);
    referenceGroup = nullptr;
    referencePrimitive = nullptr;
  }
  ModeLineUnhighlightOp(previousCommand);
}

void AeSysView::OnFixupModeEscape() {
  auto* document = GetDocument();

  if (referenceGroup != nullptr) {
    document->UpdateAllViews(nullptr, EoDb::kPrimitive, referencePrimitive);
    referenceGroup = nullptr;
    referencePrimitive = nullptr;
  }
  ModeLineUnhighlightOp(previousCommand);
}
