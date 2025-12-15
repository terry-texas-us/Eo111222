#include "stdafx.h"
#include "AeSysView.h"

#include "EoDlgEditOptions.h"

// EoDlgEditOptions dialog

IMPLEMENT_DYNAMIC(EoDlgEditOptions, CDialog)

BEGIN_MESSAGE_MAP(EoDlgEditOptions, CDialog)
	ON_COMMAND(IDC_EDIT_OP_ROTATION, &EoDlgEditOptions::OnEditOpRotation)
	ON_COMMAND(IDC_EDIT_OP_MIRRORING, &EoDlgEditOptions::OnEditOpMirroring)
	ON_COMMAND(IDC_EDIT_OP_SIZING, &EoDlgEditOptions::OnEditOpSizing)
	ON_BN_CLICKED(IDC_EDIT_OP_MIR_X, &EoDlgEditOptions::OnBnClickedEditOpMirX)
	ON_BN_CLICKED(IDC_EDIT_OP_MIR_Y, &EoDlgEditOptions::OnBnClickedEditOpMirY)
	ON_BN_CLICKED(IDC_EDIT_OP_MIR_Z, &EoDlgEditOptions::OnBnClickedEditOpMirZ)
END_MESSAGE_MAP()

EoDlgEditOptions::EoDlgEditOptions(CWnd* pParent /* = nullptr */) :
	CDialog(EoDlgEditOptions::IDD, pParent)
	, m_EditModeScaleX(0), m_EditModeScaleY(0), m_EditModeScaleZ(0)
	, m_EditModeRotationAngleX(0), m_EditModeRotationAngleY(0), m_EditModeRotationAngleZ(0) {
}
EoDlgEditOptions::EoDlgEditOptions(AeSysView* view, CWnd* pParent /* = nullptr */) :
	CDialog(EoDlgEditOptions::IDD, pParent), m_ActiveView(view) {
	}
EoDlgEditOptions::~EoDlgEditOptions() {
}
void EoDlgEditOptions::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_EDIT_OP_MIR_X, m_MirrorXCheckControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_MIR_Y, m_MirrorYCheckControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_MIR_Z, m_MirrorZCheckControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_SIZ_X, m_SizingXEditControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_SIZ_Y, m_SizingYEditControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_SIZ_Z, m_SizingZEditControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_ROT_X, m_RotationXEditControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_ROT_Y, m_RotationYEditControl);
	DDX_Control(dataExchange, IDC_EDIT_OP_ROT_Z, m_RotationZEditControl);
	DDX_Text(dataExchange, IDC_EDIT_OP_SIZ_X, m_EditModeScaleX);
	DDX_Text(dataExchange, IDC_EDIT_OP_SIZ_Y, m_EditModeScaleY);
	DDX_Text(dataExchange, IDC_EDIT_OP_SIZ_Z, m_EditModeScaleZ);
	DDX_Text(dataExchange, IDC_EDIT_OP_ROT_X, m_EditModeRotationAngleX);
	DDX_Text(dataExchange, IDC_EDIT_OP_ROT_Y, m_EditModeRotationAngleY);
	DDX_Text(dataExchange, IDC_EDIT_OP_ROT_Z, m_EditModeRotationAngleZ);
}
BOOL EoDlgEditOptions::OnInitDialog() {
	CDialog::OnInitDialog();

	if (m_ActiveView->m_EditModeMirrorScale.x < 0.0) {
		m_MirrorXCheckControl.SetCheck(BST_CHECKED);
	}
	else if (m_ActiveView->m_EditModeMirrorScale.y < 0.0) {
		m_MirrorYCheckControl.SetCheck(BST_CHECKED);
	}
	else {
		m_MirrorZCheckControl.SetCheck(BST_CHECKED);
	}
	return TRUE;
}
void EoDlgEditOptions::OnOK() {
	if (m_MirrorXCheckControl.GetCheck() == BST_CHECKED) {
		m_ActiveView->SetMirrorScale(- 1, 1., 1.);
	}
	else if (m_MirrorYCheckControl.GetCheck() == BST_CHECKED) {
		m_ActiveView->SetMirrorScale(1., - 1., 1.);
	}
	else {
		m_ActiveView->SetMirrorScale(1., 1., - 1.);
	}
	CDialog::OnOK();
}
void EoDlgEditOptions::OnEditOpRotation() {
	m_SizingXEditControl.EnableWindow(FALSE);
	m_SizingYEditControl.EnableWindow(FALSE);
	m_SizingZEditControl.EnableWindow(FALSE);
	m_RotationXEditControl.EnableWindow(TRUE);
	m_RotationYEditControl.EnableWindow(TRUE);
	m_RotationZEditControl.EnableWindow(TRUE);
	m_MirrorXCheckControl.EnableWindow(FALSE);
	m_MirrorYCheckControl.EnableWindow(FALSE);
	m_MirrorZCheckControl.EnableWindow(FALSE);
}
void EoDlgEditOptions::OnEditOpMirroring() {
	m_SizingXEditControl.EnableWindow(FALSE);
	m_SizingYEditControl.EnableWindow(FALSE);
	m_SizingZEditControl.EnableWindow(FALSE);
	m_RotationXEditControl.EnableWindow(FALSE);
	m_RotationYEditControl.EnableWindow(FALSE);
	m_RotationZEditControl.EnableWindow(FALSE);
	m_MirrorXCheckControl.EnableWindow(TRUE);
	m_MirrorYCheckControl.EnableWindow(TRUE);
	m_MirrorZCheckControl.EnableWindow(TRUE);
}
void EoDlgEditOptions::OnEditOpSizing() {
	m_SizingXEditControl.EnableWindow(TRUE);
	m_SizingYEditControl.EnableWindow(TRUE);
	m_SizingZEditControl.EnableWindow(TRUE);
	m_RotationXEditControl.EnableWindow(FALSE);
	m_RotationYEditControl.EnableWindow(FALSE);
	m_RotationZEditControl.EnableWindow(FALSE);
	m_MirrorXCheckControl.EnableWindow(FALSE);
	m_MirrorYCheckControl.EnableWindow(FALSE);
	m_MirrorZCheckControl.EnableWindow(FALSE);
}
void EoDlgEditOptions::OnBnClickedEditOpMirX() {
	m_MirrorXCheckControl.SetCheck(BST_CHECKED);
	m_MirrorYCheckControl.SetCheck(BST_UNCHECKED);
	m_MirrorZCheckControl.SetCheck(BST_UNCHECKED);
}
void EoDlgEditOptions::OnBnClickedEditOpMirY() {
	m_MirrorXCheckControl.SetCheck(BST_UNCHECKED);
	m_MirrorYCheckControl.SetCheck(BST_CHECKED);
	m_MirrorZCheckControl.SetCheck(BST_UNCHECKED);
}
void EoDlgEditOptions::OnBnClickedEditOpMirZ() {
	m_MirrorXCheckControl.SetCheck(BST_UNCHECKED);
	m_MirrorYCheckControl.SetCheck(BST_UNCHECKED);
	m_MirrorZCheckControl.SetCheck(BST_CHECKED);
}
