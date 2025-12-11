#include "stdafx.h"

#if defined(USING_ODA)
#include "EoLineWeightDialog.h"

extern CString StringByLineWeight(int lineWeight, bool lineWeightByIndex);
extern OdDb::LineWeight LineWeightByIndex(char lineWeight);

// EoLineWeightDialog dialog

IMPLEMENT_DYNAMIC(EoLineWeightDialog, CDialog)

BEGIN_MESSAGE_MAP(EoLineWeightDialog, CDialog)
	ON_LBN_DBLCLK(IDC_LIST_LINEWEIGHT, &EoLineWeightDialog::OnLbnDblclkListLineweight)
	ON_BN_CLICKED(IDOK, &EoLineWeightDialog::OnBnClickedOk)
END_MESSAGE_MAP()

EoLineWeightDialog::EoLineWeightDialog(CWnd* parent /*=NULL*/)
	: CDialog(EoLineWeightDialog::IDD, parent)
	, m_OriginalLineWeight(0)
{
}

EoLineWeightDialog::EoLineWeightDialog(int originalLineWeight, CWnd* parent)
	: CDialog(EoLineWeightDialog::IDD, parent)
	, m_OriginalLineWeight(originalLineWeight)
	, m_LineWeight((OdDb::LineWeight) originalLineWeight)
{
}

EoLineWeightDialog::~EoLineWeightDialog()
{
}

void EoLineWeightDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LINEWEIGHT, m_LineWeightList);
}


BOOL EoLineWeightDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	m_LineWeightList.InsertString(0, StringByLineWeight(OdDb::kLnWtByLwDefault, false));
	m_LineWeightList.SetItemData(0, (DWORD_PTR) OdDb::kLnWtByLwDefault);
	for (int Index = 1; Index < 25; ++Index)
	{
		m_LineWeightList.InsertString(Index, StringByLineWeight(Index - 1, true));
		m_LineWeightList.SetItemData(Index, (DWORD_PTR) LineWeightByIndex(char(Index - 1)));
	}
	m_LineWeightList.SelectString(- 1, StringByLineWeight(m_OriginalLineWeight, false));
	CString Text;
	Text.Format(L"Original : %s", StringByLineWeight(m_OriginalLineWeight, false));
	GetDlgItem(IDC_STATIC_LINEWEIGHT_ORIGINAL)->SetWindowTextW(Text);
	return TRUE;
}

// EoLineWeightDialog message handlers

void EoLineWeightDialog::OnBnClickedOk()
{
	int Index = m_LineWeightList.GetCurSel();
	m_LineWeight = (OdDb::LineWeight) m_LineWeightList.GetItemData(Index);

	CDialog::OnOK();
}

void EoLineWeightDialog::OnLbnDblclkListLineweight()
{
	OnBnClickedOk();
}
#endif // USING_ODA