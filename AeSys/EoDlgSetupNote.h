#pragma once

// EoDlgSetupNote dialog

class EoDlgSetupNote : public CDialog {
	DECLARE_DYNAMIC(EoDlgSetupNote)

public:
	EoDlgSetupNote(CWnd* pParent = NULL);
	EoDlgSetupNote(EoDbFontDefinition* fontDefinition, CWnd* pParent = NULL);
	virtual ~EoDlgSetupNote();

// Dialog Data
	enum { IDD = IDD_SETUP_NOTE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	EoDbFontDefinition* m_FontDefinition;

	CMFCFontComboBox m_MfcFontComboControl;

	double m_TextHeight;
	double m_TextExpansionFactor;
	double m_CharacterSlantAngle;
	double m_TextRotationAngle;

protected:
	DECLARE_MESSAGE_MAP()
};
