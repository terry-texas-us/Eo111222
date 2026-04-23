#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "Resource.h"

void AeSysView::OnModePrimitiveMend() {
  const EoGePoint3d cursorPosition = GetCursorPosition();
  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  m_PrimitiveToMend = nullptr;

  if (GroupIsEngaged()) {  // Group is currently engaged, see if cursor is on a control point
    EoGePoint3d ptDet;

    auto* primitive = EngagedPrimitive();

    EoDbPolygon::EdgeToEvaluate() = EoDbPolygon::Edge();

    if (primitive->SelectUsingPoint(this, ptView,
            ptDet)) {  // Cursor is close enough to engaged primitive to use it first
      m_PrimitiveToMend = primitive;
    }
  }
  if (m_PrimitiveToMend == nullptr) {  // No engaged group, or engaged primitive to far from cursor
    if (SelectGroupAndPrimitive(m_MendPrimitiveBegin) != nullptr) {  // Group successfully engaged
      m_PrimitiveToMend = EngagedPrimitive();
    }
  }
  m_MendPrimitiveBegin = cursorPosition;

  if (m_PrimitiveToMend != nullptr) {
    m_PrimitiveToMend->Copy(m_PrimitiveToMendCopy);
    m_MendPrimitiveBegin = m_PrimitiveToMend->SelectAtControlPoint(this, ptView);
    m_MendPrimitiveVertexIndex = 1U << EoDbPrimitive::ControlPointIndex();

    app.LoadModeResources(ID_MODE_PRIMITIVE_MEND);
  }
}

void AeSysView::PreviewMendPrimitive() {
  auto* document = GetDocument();
  const EoGePoint3d cursorPosition = GetCursorPosition();
  const EoGeVector3d Translate(m_MendPrimitiveBegin, cursorPosition);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, m_PrimitiveToMendCopy);
  m_PrimitiveToMendCopy->TranslateUsingMask(Translate, m_MendPrimitiveVertexIndex);
  document->UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, m_PrimitiveToMendCopy);
  m_MendPrimitiveBegin = cursorPosition;
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
