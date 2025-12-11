#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"

#include "EoDlgModeLetter.h"

// EoDlgModeLetter dialog

IMPLEMENT_DYNAMIC(EoDlgModeLetter, CDialog)

BEGIN_MESSAGE_MAP(EoDlgModeLetter, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

EoGePoint3d EoDlgModeLetter::m_Point = EoGePoint3d::kOrigin;

EoDlgModeLetter::EoDlgModeLetter(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgModeLetter::IDD, pParent) {
}
EoDlgModeLetter::~EoDlgModeLetter() {
}
void EoDlgModeLetter::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TEXT, m_TextEditControl);
}
BOOL EoDlgModeLetter::OnInitDialog() {
	CDialog::OnInitDialog();

	m_Point = app.GetCursorPosition();

	return TRUE;
}
void EoDlgModeLetter::OnOK() {
	EoDbCharacterCellDefinition ccd;
	pstate.GetCharCellDef(ccd);
	EoGeReferenceSystem ReferenceSystem(m_Point, ccd);

	EoDbFontDefinition FontDefinition;
	pstate.GetFontDef(FontDefinition);

	if (m_TextEditControl.GetWindowTextLengthW() != 0) {
		CString Text;
		m_TextEditControl.GetWindowTextW(Text);
		m_TextEditControl.SetWindowTextW(L"");

		EoDbText* TextPrimitive = new EoDbText(FontDefinition, ReferenceSystem, Text);
		EoDbGroup* Group = new EoDbGroup(TextPrimitive);
		AeSysDoc::GetDoc()->AddWorkLayerGroup(Group);
		AeSysDoc::GetDoc()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
	}
	m_Point = text_GetNewLinePos(FontDefinition, ReferenceSystem, 1., 0);
	m_TextEditControl.SetFocus();

	CDialog::OnOK();
}
void EoDlgModeLetter::OnSize(UINT type, int cx, int cy) {
	CDialog::OnSize(type, cx, cy);

	if (::IsWindow(m_TextEditControl.GetSafeHwnd())) {
		m_TextEditControl.MoveWindow(0, 0, cx, cy, TRUE);
	}
}
