#pragma once


// ViewLightingSettingsDialog dialog

class ViewLightingSettingsDialog : public CDialog
{
	DECLARE_DYNAMIC(ViewLightingSettingsDialog)

public:
	ViewLightingSettingsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ViewLightingSettingsDialog();

// Dialog Data
	enum { IDD = IDD_VIEWLIGHTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	float m_GlobalAmbientLightRed;
	float m_GlobalAmbientLightGreen;
	float m_GlobalAmbientLightBlue;
	double m_L0PositionX;
	double m_L0PositionY;
	double m_L0PositionZ;
	float m_L0AmbientRed;
	float m_L0AmbientGreen;
	float m_L0AmbientBlue;
	float m_L0DiffuseRed;
	float m_L0DiffuseGreen;
	float m_L0DiffuseBlue;
	float m_L0SpecularRed;
	float m_L0SpecularGreen;
	float m_L0SpecularBlue;
};
