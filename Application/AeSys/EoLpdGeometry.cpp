#include "Stdafx.h"

#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGsRenderState.h"
#include "EoLpdGeometry.h"
#include "Section.h"

namespace Lpd {

void GenerateEndCap(const EoGePoint3d& beginPoint,
    const EoGePoint3d& endPoint,
    Section section,
    EoDbGroup* group) {
  const auto midpoint = EoGePoint3d::Mid(beginPoint, endPoint);
  const double data[] = {section.Width(), section.Depth()};

  auto* pointPrimitive = new EoDbPoint(15, 8, midpoint);
  pointPrimitive->SetDat(2, data);
  group->AddTail(pointPrimitive);
  group->AddTail(EoDbLine::CreateLine(beginPoint, endPoint)->WithProperties(Gs::renderState));
}

void GenerateRectangularSection(const EoGeLine& referenceLine,
    double eccentricity,
    Section section,
    EoDbGroup* group) {
  EoGeLine leftLine;
  EoGeLine rightLine;

  if (referenceLine.GetParallels(section.Width(), eccentricity, leftLine, rightLine)) {
    GenerateEndCap(leftLine.begin, rightLine.begin, section, group);
    group->AddTail(EoDbLine::CreateLine(leftLine.begin, leftLine.end)->WithProperties(Gs::renderState));
    GenerateEndCap(leftLine.end, rightLine.end, section, group);
    group->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  }
}

void GenerateRectangularElbow(EoGeLine& previousReferenceLine,
    Section previousSection,
    EoGeLine& currentReferenceLine,
    Section currentSection,
    double ductSeamSize,
    double centerLineEccentricity,
    bool generateTurningVanes,
    EoDbGroup* group) {
  if (previousReferenceLine.ParallelTo(currentReferenceLine)) { return; }

  previousReferenceLine.end = previousReferenceLine.end.ProjectToward(
      previousReferenceLine.begin, ductSeamSize + previousSection.Width() * centerLineEccentricity);

  EoGeLine previousLeftLine;
  EoGeLine previousRightLine;
  if (!previousReferenceLine.GetParallels(
          previousSection.Width(), centerLineEccentricity, previousLeftLine, previousRightLine)) {
    return;
  }
  currentReferenceLine.begin = currentReferenceLine.begin.ProjectToward(
      currentReferenceLine.end, ductSeamSize + previousSection.Width() * centerLineEccentricity);

  EoGeLine currentLeftLine;
  EoGeLine currentRightLine;
  if (!currentReferenceLine.GetParallels(
          currentSection.Width(), centerLineEccentricity, currentLeftLine, currentRightLine)) {
    return;
  }
  EoGePoint3d insideCorner;
  EoGePoint3d outsideCorner;
  if (!EoGeLine::Intersection_xy(previousLeftLine, currentLeftLine, insideCorner)) { return; }
  if (!EoGeLine::Intersection_xy(previousRightLine, currentRightLine, outsideCorner)) { return; }

  GenerateEndCap(previousLeftLine.end, previousRightLine.end, previousSection, group);
  group->AddTail(EoDbLine::CreateLine(previousLeftLine.end, insideCorner)->WithProperties(Gs::renderState));
  group->AddTail(EoDbLine::CreateLine(insideCorner, currentLeftLine.begin)->WithProperties(Gs::renderState));
  group->AddTail(EoDbLine::CreateLine(previousRightLine.end, outsideCorner)->WithProperties(Gs::renderState));
  group->AddTail(EoDbLine::CreateLine(outsideCorner, currentRightLine.begin)->WithProperties(Gs::renderState));
  if (generateTurningVanes) {
    group->AddTail(EoDbLine::CreateLine(insideCorner, outsideCorner)->WithProperties(2, L"Dash2"));
  }
  GenerateEndCap(currentLeftLine.begin, currentRightLine.begin, currentSection, group);
}

void GenerateTransition(const EoGeLine& referenceLine,
    double eccentricity,
    EJust justification,
    double slope,
    Section previousSection,
    Section currentSection,
    EoDbGroup* group) {
  const double referenceLength = referenceLine.Length();
  if (referenceLength < Eo::geometricTolerance) { return; }

  const double widthChange = currentSection.Width() - previousSection.Width();
  double transitionLength = LengthOfTransition(justification, slope, previousSection, currentSection);
  transitionLength = std::min(transitionLength, referenceLength);

  EoGeLine leftLine;
  EoGeLine rightLine;
  if (!referenceLine.GetParallels(previousSection.Width(), eccentricity, leftLine, rightLine)) { return; }

  if (justification == Center) {
    leftLine.ProjPtFrom_xy(transitionLength, widthChange * 0.5, &leftLine.end);
    rightLine.ProjPtFrom_xy(transitionLength, -widthChange * 0.5, &rightLine.end);
  } else if (justification == Right) {
    rightLine.ProjPtFrom_xy(transitionLength, -widthChange, &rightLine.end);
  } else {
    leftLine.ProjPtFrom_xy(transitionLength, widthChange, &leftLine.end);
  }
  GenerateEndCap(leftLine.begin, rightLine.begin, previousSection, group);
  group->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
  GenerateEndCap(rightLine.end, leftLine.end, currentSection, group);
  group->AddTail(EoDbLine::CreateLine(leftLine.end, leftLine.begin)->WithProperties(Gs::renderState));
}

void GenerateRiseDrop(const std::wstring& indicatorLineTypeName,
    Section section,
    EoGeLine& referenceLine,
    double ductSeamSize,
    double eccentricity,
    EoDbGroup* group) {
  const double sectionLength = referenceLine.Length();

  EoGeLine leftLine;
  EoGeLine rightLine;
  if (!referenceLine.GetParallels(section.Width(), eccentricity, leftLine, rightLine)) { return; }

  if (sectionLength >= section.Depth() * 0.5 + ductSeamSize) {
    EoGeLine seamReferenceLine(referenceLine);
    seamReferenceLine.end = seamReferenceLine.begin.ProjectToward(seamReferenceLine.end, ductSeamSize);
    if (!seamReferenceLine.GetParallels(section.Width(), eccentricity, leftLine, rightLine)) { return; }
    group->AddTail(EoDbLine::CreateLine(leftLine.begin, leftLine.end)->WithProperties(Gs::renderState));
    group->AddTail(EoDbLine::CreateLine(rightLine.begin, rightLine.end)->WithProperties(Gs::renderState));
    referenceLine.begin = seamReferenceLine.end;
  }
  if (!referenceLine.GetParallels(section.Width(), eccentricity, leftLine, rightLine)) { return; }
  GenerateRectangularSection(referenceLine, eccentricity, section, group);

  group->AddTail(EoDbLine::CreateLine(leftLine.begin, rightLine.end)
          ->WithProperties(Gs::renderState.Color(), indicatorLineTypeName));
  group->AddTail(EoDbLine::CreateLine(rightLine.begin, leftLine.end)
          ->WithProperties(Gs::renderState.Color(), indicatorLineTypeName));
}

double LengthOfTransition(EJust justification,
    double slope,
    Section previousSection,
    Section currentSection) noexcept {
  const double widthChange = currentSection.Width() - previousSection.Width();
  const double depthChange = currentSection.Depth() - previousSection.Depth();
  double length = std::max(std::abs(widthChange), std::abs(depthChange)) * slope;
  if (justification == Center) { length *= 0.5; }
  return length;
}

}  // namespace Lpd
