#pragma once

// EoCtrlNamedColorsButton

class EoCtrlNamedColorsButton : public CMFCButton
{
	DECLARE_DYNAMIC(EoCtrlNamedColorsButton)

	static const int m_CellHorizontalSize = 12;
	static const int m_CellSpacing = 8;
	
	CBrush m_WindowBrush;
	CBrush m_WindowFrameBrush;

	EoUInt16 m_SubItem;
	EoUInt16 m_SubItemState; 
	CRect m_SubItemRectangle;
	
	void SubItemRectangleByIndex(EoUInt16 index, CRect& rectangle);
	EoUInt16 SubItemRectangleByPosition(const CPoint& position, CRect& rectangle);
	
public:

	EoCtrlNamedColorsButton();   // standard constructor

	virtual ~EoCtrlNamedColorsButton();
	virtual void OnDraw(CDC* deviceContext, const CRect& rectangle, UINT state);

	afx_msg void OnLButtonDown(UINT flags, CPoint point);
	afx_msg void OnLButtonUp(UINT flags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT flags, CPoint point);
	afx_msg void OnMouseMove(UINT flags, CPoint point);
	afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()
};

