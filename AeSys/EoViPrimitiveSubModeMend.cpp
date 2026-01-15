#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "Resource.h"

void AeSysView::OnModePrimitiveMend() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  EoGePoint4d ptView(CurrentPnt);
  ModelViewTransformPoint(ptView);

  m_PrimitiveToMend = 0;

  if (GroupIsEngaged()) {  // Group is currently engaged, see if cursor is on a control point
    EoGePoint3d ptDet;

    EoDbPrimitive* Primitive = EngagedPrimitive();

    EoDbPolygon::EdgeToEvaluate() = EoDbPolygon::Edge();

    if (Primitive->SelectUsingPoint(this, ptView,
                                    ptDet)) {  // Cursor is close enough to engaged primitive to use it first
      m_PrimitiveToMend = Primitive;
    }
  }
  if (m_PrimitiveToMend == 0) {  // No engaged group, or engaged primitive to far from cursor
    if (SelectGroupAndPrimitive(m_MendPrimitiveBegin) != 0) {  // Group successfully engaged
      m_PrimitiveToMend = EngagedPrimitive();
    }
  }
  m_MendPrimitiveBegin = CurrentPnt;

  if (m_PrimitiveToMend != 0) {
    m_PrimitiveToMend->Copy(m_PrimitiveToMendCopy);
    m_MendPrimitiveBegin = m_PrimitiveToMend->SelectAtControlPoint(this, ptView);
    m_MendPrimitiveVertexIndex = 1U << EoDbPrimitive::ControlPointIndex();

    app.LoadModeResources(ID_MODE_PRIMITIVE_MEND);
  }
}
void AeSysView::PreviewMendPrimitive() {
  auto* document = GetDocument();
  EoGePoint3d CurrentPnt = GetCursorPosition();
  EoGeVector3d Translate(m_MendPrimitiveBegin, CurrentPnt);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_PrimitiveToMendCopy);
  m_PrimitiveToMendCopy->TranslateUsingMask(Translate, m_MendPrimitiveVertexIndex);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_PrimitiveToMendCopy);
  m_MendPrimitiveBegin = CurrentPnt;
}
void AeSysView::MendPrimitiveReturn() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_PrimitiveToMendCopy);
  m_PrimitiveToMend->Assign(m_PrimitiveToMendCopy);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_PrimitiveToMend);

  delete m_PrimitiveToMendCopy;

  app.LoadModeResources(app.PrimaryMode());
}
void AeSysView::MendPrimitiveEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_PrimitiveToMendCopy);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_PrimitiveToMend);

  delete m_PrimitiveToMendCopy;

  app.LoadModeResources(app.PrimaryMode());
}
