#include "stdafx.h"

CLayer::CLayer(const CString& strName, EoUInt16 wStateFlgs)
{
	m_strName = strName;
	m_wTracingFlgs = 0;
	m_wStateFlgs = wStateFlgs;
	m_PenColor = 1;
	m_strLineTypeName = _T("Continuous");
}
void CLayer::Display(AeSysView* view, CDC* deviceContext)
{
	EoDbPrimitive::LayerPenColor() = PenColor();
	EoDbPrimitive::LayerLineType() = LineType();

	COLORREF* pCurColTbl = pColTbl;
	
	pColTbl = (IsOpened() || IsWork() || IsActive()) ? ColorPalette : GreyPalette;

	EoDbGroupList::Display(view, deviceContext);
	pColTbl = pCurColTbl;
}
void CLayer::Display(AeSysView* view, CDC* deviceContext, bool identifyTrap)
{
	ATLTRACE2(atlTraceGeneral, 1, _T("CLayer<%08.8lx>::Display(%08.8lx, %08.8lx, %i) + Name: %s\n"), this, view, deviceContext, identifyTrap, this->GetName());
	
	AeSysDoc* Document = AeSysDoc::GetDoc();

	try
	{
		if (IsOn())
		{
			EoDbPrimitive::LayerPenColor() = PenColor();
			EoDbPrimitive::LayerLineType() = LineType();
		
			COLORREF* pCurColTbl = pColTbl;
		
			bool LayerIsDetectable = IsOpened() || IsWork() || IsActive();
		
			pColTbl = LayerIsDetectable ? ColorPalette : GreyPalette;

			POSITION position = GetHeadPosition();
			while (position != 0)
			{
				EoDbGroup* Group = GetNext(position);
	
				if (Group->IsInView(view))
				{
					if (LayerIsDetectable)
					{
						Document->AddGroupToAllViews(Group);
					}
					if (identifyTrap && Document->FindTrappedGroup(Group) != 0)
					{
						EoDbPrimitive::SpecPenColor() = app.TrapHighlightPenColor();
						Group->Display(view, deviceContext);
						EoDbPrimitive::SpecPenColor() = 0;
					}
					else
					{
						Group->Display(view, deviceContext);
					}					
				}
			}
			pColTbl = pCurColTbl;
		}
	}
	catch (CException* e)
	{
		e->Delete();
	}
}
EoInt16 CLayer::LineType()
{
	EoUInt16 LineTypeIndex = 1;
	
	EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();
	
	EoDbLineType* LineType;
	if (LineTypeTable->Lookup(m_strLineTypeName, LineType))
	{
		LineTypeIndex = LineType->Index();
	}
	return LineTypeIndex;
}
void CLayer::PenTranslation(EoUInt16 wCols, EoInt16* pColNew, EoInt16* pCol)
{
	for (int i = 0; i < wCols; i++)
	{
		if (m_PenColor == pCol[i])
		{
			m_PenColor = pColNew[i];
			break;
		}
	}
	EoDbGroupList::PenTranslation(wCols, pColNew, pCol);
}
void CLayer::SetStateCold()
{
	ClrStateFlg(EoDb::kIsWork | EoDb::kIsActive | EoDb::kIsOff);
	SetStateFlg(EoDb::kIsStatic);
}
void CLayer::SetStateWork()
{
	ClrStateFlg(EoDb::kIsActive | EoDb::kIsStatic | EoDb::kIsOff);
	SetStateFlg(EoDb::kIsWork);
}
void CLayer::SetStateOff()
{
	ClrStateFlg(EoDb::kIsWork | EoDb::kIsActive | EoDb::kIsStatic);
	SetStateFlg(EoDb::kIsOff);
}
void CLayer::SetStateActive()
{
	ClrStateFlg(EoDb::kIsWork | EoDb::kIsStatic | EoDb::kIsOff);
	SetStateFlg(EoDb::kIsActive);
}