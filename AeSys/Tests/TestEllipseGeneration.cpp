#include "Stdafx.h"

#include <Windows.h>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include "Eo.h"
#include "EoDbEllipse.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

#include "drw_base.h"

static double eps = 1e-6;

static bool AlmostEqual(const EoGePoint3d& a, const EoGePoint3d& b, double tol = eps) {
  return (fabs(a.x - b.x) <= tol && fabs(a.y - b.y) <= tol && fabs(a.z - b.z) <= tol);
}
static void DebugPrintW(const std::wstring& s) {
  OutputDebugStringW(s.c_str());
  OutputDebugStringW(L"\n");
}

void TestArc(double cx, double cy, double cz, double radius, double startAngle, double endAngle) {
  DRW_Coord center(cx, cy, cz);
  EoDbEllipse arc(center, radius, startAngle, endAngle);

  auto normalize = [](double ang) {
    while (ang < 0.0) ang += Eo::TwoPi;
    while (ang >= Eo::TwoPi) ang -= Eo::TwoPi;
    return ang;
  };

  double sNorm = normalize(startAngle);
  double eNorm = normalize(endAngle);
  double sweep = eNorm - sNorm;
  // bring sweep into (-2pi,2pi]
  if (sweep <= -Eo::TwoPi) sweep += Eo::TwoPi;
  if (sweep > Eo::TwoPi) sweep -= Eo::TwoPi;
  if (fabs(sweep) < DBL_EPSILON) sweep = Eo::TwoPi;  // full circle

  EoGePoint3d expectedBegin(cx + radius * cos(sNorm), cy + radius * sin(sNorm), cz);
  EoGePoint3d expectedEnd(cx + radius * cos(sNorm + sweep), cy + radius * sin(sNorm + sweep), cz);
  EoGePoint3d expectedMid(cx + radius * cos(sNorm + sweep * 0.5), cy + radius * sin(sNorm + sweep * 0.5), cz);

  EoGePoint3d beg = arc.GetBegPt();
  EoGePoint3d end = arc.GetEndPt();

  // compute mid using rotation of major axis half sweep
  EoGeVector3d vMaj = arc.MajorAxis();
  EoGeVector3d planeNormal = EoGeVector3d::positiveUnitZ;
  EoGeVector3d vHalf = vMaj;
  vHalf.RotAboutArbAx(planeNormal, sweep * 0.5);
  EoGePoint3d mid = arc.CenterPoint() + vHalf;

  std::wostringstream ss;
  ss << L"Test start=" << startAngle << L" end=" << endAngle << L" sweep=" << sweep << L"\n";
  ss << L" Expected Begin: (" << expectedBegin.x << L"," << expectedBegin.y << L"," << expectedBegin.z << L")\n";
  ss << L" Actual   Begin: (" << beg.x << L"," << beg.y << L"," << beg.z << L")\n";
  ss << L" Expected End: (" << expectedEnd.x << L"," << expectedEnd.y << L"," << expectedEnd.z << L")\n";
  ss << L" Actual   End: (" << end.x << L"," << end.y << L"," << end.z << L")\n";
  ss << L" Expected Mid: (" << expectedMid.x << L"," << expectedMid.y << L"," << expectedMid.z << L")\n";
  ss << L" Actual   Mid: (" << mid.x << L"," << mid.y << L"," << mid.z << L")\n";

  bool ok = AlmostEqual(beg, expectedBegin) && AlmostEqual(end, expectedEnd) && AlmostEqual(mid, expectedMid);
  ss << (ok ? L"PASS\n\n" : L"FAIL\n\n");
  DebugPrintW(ss.str());
}