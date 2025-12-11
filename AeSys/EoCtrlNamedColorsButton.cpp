#include "stdafx.h"
#include "AeSys.h"

#include "EoCtrlNamedColorsButton.h"

IMPLEMENT_DYNAMIC(EoCtrlNamedColorsButton, CMFCButton)

BEGIN_MESSAGE_MAP(EoCtrlNamedColorsButton, CMFCButton)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_PAINT()
END_MESSAGE_MAP()

EoCtrlNamedColorsButton::EoCtrlNamedColorsButton()
{
	m_SubItem = 0;
	m_SubItemState = 0;
	m_WindowBrush.CreateSysColorBrush(CTLCOLOR_DLG);
	m_WindowFrameBrush.CreateSysColorBrush(COLOR_WINDOWFRAME);
}
EoCtrlNamedColorsButton::~EoCtrlNamedColorsButton()
{
}
void EoCtrlNamedColorsButton::SubItemRectangleByIndex(EoUInt16 index, CRect& rectangle)
{
	GetClientRect(rectangle);
	rectangle.top += m_CellSpacing;
	rectangle.left = m_CellSpacing + (index - 1) * (m_CellHorizontalSize + m_CellSpacing);
	rectangle.bottom-= m_CellSpacing;
	rectangle.right = rectangle.left + m_CellHorizontalSize; 
}
EoUInt16 EoCtrlNamedColorsButton::SubItemRectangleByPosition(const CPoint& position, CRect& rectangle)
{
	rectangle.SetRectEmpty();

	for (EoUInt16 Index = 1; Index <= 9; Index++)
	{
		SubItemRectangleByIndex(Index, rectangle);
		if (rectangle.PtInRect(position) == TRUE)
		{
			return Index;
		}
	}
	return 0;
}
void EoCtrlNamedColorsButton::OnDraw(CDC* deviceContext, const CRect& rectangle, UINT state)
{
	deviceContext->FillRect(&rectangle, &m_WindowBrush);
	if ((state & ODS_FOCUS) == ODS_FOCUS)
	{
		deviceContext->FrameRect(&rectangle, &m_WindowFrameBrush);
	}
	CRect CellRectangle(rectangle);
	for (EoUInt16 Index = 1; Index < 10; Index++)
	{
		CBrush Brush(ColorPalette[Index]);
		SubItemRectangleByIndex(Index, CellRectangle);
		deviceContext->FillRect(&CellRectangle, &Brush);
		CellRectangle.InflateRect(1, 1);
		deviceContext->FrameRect(&CellRectangle, &m_WindowFrameBrush);
	}
	m_SubItemState = 0;
}
void EoCtrlNamedColorsButton::OnLButtonDown(UINT flags, CPoint point)
{
	CRect CurrentSubItemRectangle;
	EoUInt16 CurrentSubItem = SubItemRectangleByPosition(point, CurrentSubItemRectangle);
		
	if (CurrentSubItem != 0)
	{
		CDC* DeviceContext = GetDC();
		HBRUSH Brush;
		Brush = ::GetSysColorBrush(CTLCOLOR_DLG);

		CRect PreviousSubItemRectangle;
		::FrameRect(DeviceContext->m_hDC, &m_SubItemRectangle, Brush);

		ReleaseDC(DeviceContext);
	}
	CMFCButton::OnLButtonDown(flags, point);
}
void EoCtrlNamedColorsButton::OnLButtonUp(UINT flags, CPoint point)
{
	CRect CurrentSubItemRectangle;
	EoUInt16 CurrentSubItem = SubItemRectangleByPosition(point, CurrentSubItemRectangle);

	if (CurrentSubItem != 0)
	{
		CDC* DeviceContext = GetDC();
		HBRUSH Brush;
		Brush = ::GetSysColorBrush(COLOR_HIGHLIGHT);

		::InflateRect(m_SubItemRectangle, 2, 2);
		::FrameRect(DeviceContext->m_hDC, &m_SubItemRectangle, Brush);
		::InflateRect(m_SubItemRectangle, - 2, - 2);
		
		m_SubItem = CurrentSubItem;
		m_SubItemState = ODS_SELECTED;
		ReleaseDC(DeviceContext);
	}
	CMFCButton::OnLButtonUp(flags, point);
}
void EoCtrlNamedColorsButton::OnLButtonDblClk(UINT flags, CPoint point)
{
	//EoDlgSetupColor* Parent = (EoDlgSetupColor*) GetParent();
	//Parent->OnDoubleclickedNamedColors();
}
void EoCtrlNamedColorsButton::OnMouseMove(UINT flags, CPoint point)
{
	CDC* DeviceContext = GetDC();
	CRect CurrentSubItemRectangle;
	
	EoUInt16 CurrentSubItem = SubItemRectangleByPosition(point, CurrentSubItemRectangle);
		
	if ((m_SubItemState & ODS_SELECTED) && m_SubItem != CurrentSubItem)
	{
		HBRUSH Brush;
		Brush = ::GetSysColorBrush(CTLCOLOR_DLG);

		CRect PreviousSubItemRectangle;
		::FrameRect(DeviceContext->m_hDC, &m_SubItemRectangle, Brush);
	}
	EoUInt16 PreviousSubItem = m_SubItem;

	m_SubItem = SubItemRectangleByPosition(point, m_SubItemRectangle);

	if (m_SubItem != 0)
	{
		HBRUSH Brush;
		Brush = ::GetSysColorBrush(COLOR_WINDOWFRAME);

		::InflateRect(m_SubItemRectangle, 2, 2);
		::FrameRect(DeviceContext->m_hDC, &m_SubItemRectangle, Brush);
		
		m_SubItemState = ODS_SELECTED;
	}
	ReleaseDC(DeviceContext);

	CMFCButton::OnMouseMove(flags, point);
}
void EoCtrlNamedColorsButton::OnPaint()
{
	CMFCButton::OnPaint();
}
