#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbLine.h"
#include "EoDbPolygon.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGsRenderState.h"
#include "Resource.h"

#if defined(USING_STATE_PATTERN)
#include "AeSysState.h"
#endif

void AeSysView::DoCustomMouseClick(const CString& characters) {
  int Position = 0;

  while (Position < characters.GetLength()) {
    if (characters.Find(L'{', Position) == Position) {
      Position++;
      CString VirtualKey = characters.Tokenize(L"}", Position);
      PostMessageW(WM_KEYDOWN, static_cast<WPARAM>(_wtoi(VirtualKey)), 0L);
    } else {
      PostMessageW(WM_CHAR, characters[Position++], 0L);
    }
  }
}
#if defined(USING_STATE_PATTERN)
BOOL AeSysView::PreTranslateMessage(MSG* pMsg) {
  if (pMsg->message == WM_KEYDOWN) {
    auto* state = GetCurrentState();
    if (state && state->HandleKeypad(this, static_cast<UINT>(pMsg->wParam), 1, static_cast<UINT>(pMsg->lParam))) {
      return TRUE;  // Handled by state
    }
  }
  return CView::PreTranslateMessage(pMsg);
}
#endif

void AeSysView::OnLButtonDown(UINT flags, CPoint point) {
#if defined(USING_STATE_PATTERN)
  auto* state = GetCurrentState();
  if (state) {
    state->OnLButtonDown(this, flags, point);
    /*if ( handled )*/ { return; }
  }
  // Fallback to existing logic
#endif
  if (app.CustomLButtonDownCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnLButtonDown(flags, point);
  } else {
    DoCustomMouseClick(app.CustomLButtonDownCharacters);
  }
}

void AeSysView::OnLButtonUp(UINT flags, CPoint point) {
  if (app.CustomLButtonUpCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnLButtonUp(flags, point);
  } else {
    DoCustomMouseClick(app.CustomLButtonUpCharacters);
  }
}

void AeSysView::OnRButtonDown(UINT flags, CPoint point) {
  if (app.CustomRButtonDownCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnRButtonDown(flags, point);
  } else {
    DoCustomMouseClick(app.CustomRButtonDownCharacters);
  }
}

void AeSysView::OnRButtonUp(UINT flags, CPoint point) {
  if (app.CustomRButtonUpCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnRButtonUp(flags, point);
  } else {
    DoCustomMouseClick(app.CustomRButtonUpCharacters);
  }
}

void AeSysView::OnMButtonDown([[maybe_unused]] UINT flags, CPoint point) {
  m_middleButtonPanStartPoint = point;
  m_middleButtonPanInProgress = true;
  SetCapture();
}

void AeSysView::OnMButtonUp([[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  if (m_middleButtonPanInProgress) {
    m_middleButtonPanInProgress = false;
    ReleaseCapture();
  }
}

void AeSysView::OnMouseMove([[maybe_unused]] UINT flags, CPoint point) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView::OnMouseMove - flags: %u, point: (%d, %d)\n", flags, point.x, point.y);
#if defined(USING_STATE_PATTERN)
  auto* state = GetCurrentState();
  if (state) { state->OnMouseMove(this, flags, point); }
#endif
  if (m_middleButtonPanInProgress) {
    auto delta = point - m_middleButtonPanStartPoint;
    m_middleButtonPanStartPoint = point;

    auto target = m_ViewTransform.Target();

    // Convert delta to world coordinates (scale as needed)
    target.x += static_cast<double>(-delta.cx) * m_ViewTransform.UExtent() / m_Viewport.Width();
    target.y += static_cast<double>(delta.cy) * m_ViewTransform.VExtent() / m_Viewport.Height();

    m_ViewTransform.SetTarget(target);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    InvalidateScene();  // Redraw view
  }
  DisplayOdometer();

  switch (app.CurrentMode()) {
    case ID_MODE_ANNOTATE:
      DoAnnotateModeMouseMove();
      break;

    case ID_MODE_DRAW:
      DoDrawModeMouseMove();
      break;

    case ID_MODE_DRAW2:
      DoDraw2ModeMouseMove();
      break;

    case ID_MODE_LPD:
      DoDuctModeMouseMove();
      break;

    case ID_MODE_NODAL:
      DoNodalModeMouseMove();
      break;

    case ID_MODE_PIPE:
      DoPipeModeMouseMove();
      break;

    case ID_MODE_POWER:
      DoPowerModeMouseMove();
      break;

    case ID_MODE_PRIMITIVE_EDIT:
      PreviewPrimitiveEdit();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      PreviewMendPrimitive();
      break;

    case ID_MODE_GROUP_EDIT:
      PreviewGroupEdit();
      break;
  }
  if (m_rubberbandType == Lines || m_rubberbandType == Rectangles) {
    if (m_useD2D) {
      // D2D path: update endpoint and invalidate — rubberband drawn as overlay in OnDraw
      m_rubberbandLogicalEnd = point;
      InvalidateScene();
    } else if (m_rubberbandType == Lines) {
      auto* deviceContext = GetDC();
      auto drawMode = deviceContext->SetROP2(R2_XORPEN);
      CPen grayPen(PS_SOLID, 0, Eo::RubberbandColor());
      auto* pen = deviceContext->SelectObject(&grayPen);

      deviceContext->MoveTo(m_rubberbandLogicalBegin);
      deviceContext->LineTo(m_rubberbandLogicalEnd);

      m_rubberbandLogicalEnd = point;
      deviceContext->MoveTo(m_rubberbandLogicalBegin);
      deviceContext->LineTo(m_rubberbandLogicalEnd);
      deviceContext->SelectObject(pen);
      deviceContext->SetROP2(drawMode);
      ReleaseDC(deviceContext);
    } else {  // Rectangles, GDI path
      auto* deviceContext = GetDC();
      auto drawMode = deviceContext->SetROP2(R2_XORPEN);
      CPen grayPen(PS_SOLID, 0, Eo::RubberbandColor());
      auto* pen = deviceContext->SelectObject(&grayPen);
      auto* brush = deviceContext->SelectStockObject(NULL_BRUSH);

      deviceContext->Rectangle(
          m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y, m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);

      m_rubberbandLogicalEnd = point;
      deviceContext->Rectangle(
          m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y, m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);
      deviceContext->SelectObject(brush);
      deviceContext->SelectObject(pen);
      deviceContext->SetROP2(drawMode);
      ReleaseDC(deviceContext);
    }
  }
}

BOOL AeSysView::OnMouseWheel(UINT flags, std::int16_t zDelta, CPoint point) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>OnMouseWheel(%i, %i, %08.8lx)\n", this, flags, zDelta, point);

  if (zDelta > 0) {
    OnWindowZoomIn();
  } else {
    OnWindowZoomOut();
  }
  return __super::OnMouseWheel(flags, zDelta, point);
}

void AeSysView::RubberBandingDisable() {
  if (m_rubberbandType != None) {
    if (m_useD2D) {
      // D2D path: just clear the type — next OnDraw omits the rubberband overlay
      m_rubberbandType = None;
      InvalidateScene();
      return;
    }
    auto* deviceContext = GetDC();
    int drawMode = deviceContext->SetROP2(R2_XORPEN);
    CPen grayPen(PS_SOLID, 0, Eo::RubberbandColor());
    auto* pen = deviceContext->SelectObject(&grayPen);

    if (m_rubberbandType == Lines) {
      deviceContext->MoveTo(m_rubberbandLogicalBegin);
      deviceContext->LineTo(m_rubberbandLogicalEnd);
    } else if (m_rubberbandType == Rectangles) {
      auto* brush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
      deviceContext->Rectangle(
          m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y, m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);
      deviceContext->SelectObject(brush);
    }
    deviceContext->SelectObject(pen);
    deviceContext->SetROP2(drawMode);
    ReleaseDC(deviceContext);
    m_rubberbandType = None;
  }
}

void AeSysView::RubberBandingStartAtEnable(EoGePoint3d point, ERubs type) {
  EoGePoint4d ndcPoint(point);

  ModelViewTransformPoint(ndcPoint);

  if (ndcPoint.IsInView()) {
    m_rubberbandBegin = point;
    m_rubberbandLogicalBegin = ProjectToClient(ndcPoint);
    m_rubberbandLogicalEnd = m_rubberbandLogicalBegin;
  }
  m_rubberbandType = type;
}

EoGePoint3d AeSysView::GetCursorPosition() {
  CPoint cursorPosition;

  ::GetCursorPos(&cursorPosition);
  ScreenToClient(&cursorPosition);

  EoGePoint3d pt(double(cursorPosition.x), double(cursorPosition.y), m_ptCursorPosDev.z);
  if (pt != m_ptCursorPosDev) {
    m_ptCursorPosDev = pt;
    m_ptCursorPosWorld = m_ptCursorPosDev;

    DoProjectionInverse(m_ptCursorPosWorld);

    m_ptCursorPosWorld = ModelViewGetMatrixInverse() * m_ptCursorPosWorld;
    m_ptCursorPosWorld = SnapPointToGrid(m_ptCursorPosWorld);
  }
  return m_ptCursorPosWorld;
}

void AeSysView::SetCursorPosition(const EoGePoint3d& position) {
  EoGePoint4d ndcPoint(position);
  ModelViewTransformPoint(ndcPoint);

  if (!ndcPoint.IsInView()) {
    // Redefine the view so targeted position becomes center
    m_ViewTransform.SetTarget(position);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    InvalidateScene();

    ndcPoint = EoGePoint4d(position);
    ModelViewTransformPoint(ndcPoint);
  }
  // Move the cursor to specified position.
  CPoint clientPoint = ProjectToClient(ndcPoint);

  m_ptCursorPosDev =
      EoGePoint3d(static_cast<double>(clientPoint.x), static_cast<double>(clientPoint.y), ndcPoint.z / ndcPoint.w);

  m_ptCursorPosWorld = position;

  ClientToScreen(&clientPoint);
  ::SetCursorPos(clientPoint.x, clientPoint.y);
}

void AeSysView::SetModeCursor(int mode) {
  ATLTRACE2(
      traceGeneral, 1, L"AeSysView::SetModeCursor(Mode: %i, Cursor Size: %i)\n", mode, GetSystemMetrics(SM_CXCURSOR));

  auto isWhiteBackground = Eo::activeViewBackground == Eo::ViewBackground::White;
  std::uint16_t resourceIdentifier{};

  switch (mode) {
    case ID_MODE_ANNOTATE:
      resourceIdentifier = IDR_ANNOTATE_MODE;
      break;

    case ID_MODE_CUT:
      resourceIdentifier = IDR_CUT_MODE;
      break;

    case ID_MODE_DIMENSION:
      resourceIdentifier = IDR_DIMENSION_MODE;
      break;

    case ID_MODE_DRAW:
      resourceIdentifier = isWhiteBackground ? IDR_DRAW_MODE_WHITE : IDR_DRAW_MODE;
      break;

    case ID_MODE_LPD:
      resourceIdentifier = IDR_LPD_MODE;
      break;

    case ID_MODE_PIPE:
      resourceIdentifier = IDR_PIPE_MODE;
      break;

    case ID_MODE_POWER:
      resourceIdentifier = IDR_POWER_MODE;
      break;

    case ID_MODE_DRAW2:
      resourceIdentifier = IDR_DRAW2_MODE;
      break;

    case ID_MODE_EDIT:
      resourceIdentifier = IDR_EDIT_MODE;
      break;

    case ID_MODE_FIXUP:
      resourceIdentifier = IDR_FIXUP_MODE;
      break;

    case ID_MODE_NODAL:
      resourceIdentifier = IDR_NODAL_MODE;
      break;

    case ID_MODE_NODALR:
      resourceIdentifier = IDR_NODALR_MODE;
      break;

    case ID_MODE_TRAP:
      resourceIdentifier = IDR_TRAP_MODE;
      break;

    case ID_MODE_TRAPR:
      resourceIdentifier = IDR_TRAPR_MODE;
      break;

    default:
      SetCursor(static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE)));
      return;
  }
  auto cursorHandle = static_cast<HCURSOR>(
      LoadImageW(AeSys::GetInstance(), MAKEINTRESOURCE(resourceIdentifier), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  VERIFY(cursorHandle);
  SetCursor(cursorHandle);

  SetClassLongPtr(this->GetSafeHwnd(), GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursorHandle));
}
