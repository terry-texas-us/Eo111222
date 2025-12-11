#include "stdafx.h"
#include "AeSysDoc.h"

#include "EoDlgTrapModify.h"
#include "Hatch.h"

// EoDlgTrapModify dialog

IMPLEMENT_DYNAMIC(EoDlgTrapModify, CDialog)

BEGIN_MESSAGE_MAP(EoDlgTrapModify, CDialog)
END_MESSAGE_MAP()

EoDlgTrapModify::EoDlgTrapModify(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgTrapModify::IDD, pParent) {
}
EoDlgTrapModify::EoDlgTrapModify(AeSysDoc* document, CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgTrapModify::IDD, pParent), m_Document(document) {
}
EoDlgTrapModify::~EoDlgTrapModify() {
}
void EoDlgTrapModify::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
}
void EoDlgTrapModify::OnOK() {
	if (IsDlgButtonChecked(IDC_MOD_PEN)) {
		m_Document->ModifyTrappedGroupsPenColor(pstate.PenColor());
	}
	if (IsDlgButtonChecked(IDC_MOD_LINE)) {
		m_Document->ModifyTrappedGroupsLineType(pstate.LineType());
	}
	if (IsDlgButtonChecked(IDC_MOD_FILL)) {
		ModifyPolygons();
	}
	EoDbCharacterCellDefinition ccd;
	pstate.GetCharCellDef(ccd);

	EoDbFontDefinition fd;
	pstate.GetFontDef(fd);

	if (IsDlgButtonChecked(IDC_MOD_NOTE)) {
		m_Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_ALL);
	}
	else if (IsDlgButtonChecked(IDC_FONT)) {
		m_Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_FONT);
	}
	else if (IsDlgButtonChecked(IDC_HEIGHT)) {
		m_Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_HEIGHT);
	}

	CDialog::OnOK();
}
void EoDlgTrapModify::ModifyPolygons(void) {
	POSITION Position = m_Document->GetFirstTrappedGroupPosition();
	while (Position != 0) {
		EoDbGroup* Group = m_Document->GetNextTrappedGroup(Position);

		POSITION PrimitivePosition = Group->GetHeadPosition();
		while (PrimitivePosition != 0) {
			EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

			if (Primitive->Is(EoDb::kPolygonPrimitive)) {
				EoDbPolygon* pPolygon = static_cast<EoDbPolygon*>(Primitive);
				pPolygon->SetIntStyle(pstate.PolygonIntStyle());
				pPolygon->SetIntStyleId(pstate.PolygonIntStyleId());
				pPolygon->SetHatRefVecs(hatch::dOffAng, hatch::dXAxRefVecScal, hatch::dYAxRefVecScal);
			}
		}
	}
}
