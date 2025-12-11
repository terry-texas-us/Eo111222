// ViewLightingSettingsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "ViewLightingSettingsDialog.h"


// ViewLightingSettingsDialog dialog

IMPLEMENT_DYNAMIC(ViewLightingSettingsDialog, CDialog)

BEGIN_MESSAGE_MAP(ViewLightingSettingsDialog, CDialog)
END_MESSAGE_MAP()

ViewLightingSettingsDialog::ViewLightingSettingsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ViewLightingSettingsDialog::IDD, pParent)
	, m_GlobalAmbientLightRed(0)
	, m_GlobalAmbientLightGreen(0)
	, m_GlobalAmbientLightBlue(0)
	, m_L0PositionX(0)
	, m_L0PositionY(0)
	, m_L0PositionZ(0)
	, m_L0AmbientRed(0)
	, m_L0AmbientGreen(0)
	, m_L0AmbientBlue(0)
	, m_L0DiffuseRed(0)
	, m_L0DiffuseGreen(0)
	, m_L0DiffuseBlue(0)
	, m_L0SpecularRed(0)
	, m_L0SpecularGreen(0)
	, m_L0SpecularBlue(0)
{

}

ViewLightingSettingsDialog::~ViewLightingSettingsDialog()
{
}

void ViewLightingSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LIGHT_AMBIENT_RED, m_GlobalAmbientLightRed);
	DDV_MinMaxFloat(pDX, m_GlobalAmbientLightRed, 0., 1.);
	DDX_Text(pDX, IDC_LIGHT_AMBIENT_GREEN, m_GlobalAmbientLightGreen);
	DDV_MinMaxFloat(pDX, m_GlobalAmbientLightGreen, 0., 1.);
	DDX_Text(pDX, IDC_LIGHT_AMBIENT_BLUE, m_GlobalAmbientLightBlue);
	DDV_MinMaxFloat(pDX, m_GlobalAmbientLightBlue, 0., 1.);
	DDX_Text(pDX, IDC_L0_POSITION_X, m_L0PositionX);
	DDX_Text(pDX, IDC_L0_POSITION_Y, m_L0PositionY);
	DDX_Text(pDX, IDC_L0_POSITION_Z, m_L0PositionZ);
	DDX_Text(pDX, IDC_L0_AMBIENT_RED, m_L0AmbientRed);
	DDV_MinMaxFloat(pDX, m_L0AmbientRed, 0., 1.);
	DDX_Text(pDX, IDC_L0_AMBIENT_GREEN, m_L0AmbientGreen);
	DDV_MinMaxFloat(pDX, m_L0AmbientGreen, 0., 1.);
	DDX_Text(pDX, IDC_L0_AMBIENT_BLUE, m_L0AmbientBlue);
	DDV_MinMaxFloat(pDX, m_L0AmbientBlue, 0., 1.);
	DDX_Text(pDX, IDC_L0_DIFFUSE_RED, m_L0DiffuseRed);
	DDV_MinMaxFloat(pDX, m_L0DiffuseRed, 0., 1.);
	DDX_Text(pDX, IDC_L0_DIFFUSE_GREEN, m_L0DiffuseGreen);
	DDV_MinMaxFloat(pDX, m_L0DiffuseGreen, 0., 1.);
	DDX_Text(pDX, IDC_L0_DIFFUSE_BLUE, m_L0DiffuseBlue);
	DDV_MinMaxFloat(pDX, m_L0DiffuseBlue, 0., 1.);
	DDX_Text(pDX, IDC_L0_SPECULAR_RED, m_L0SpecularRed);
	DDV_MinMaxFloat(pDX, m_L0SpecularRed, 0., 1.);
	DDX_Text(pDX, IDC_L0_SPECULAR_GREEN, m_L0SpecularGreen);
	DDV_MinMaxFloat(pDX, m_L0SpecularGreen, 0., 1.);
	DDX_Text(pDX, IDC_L0_SPECULAR_BLUE, m_L0SpecularBlue);
	DDV_MinMaxFloat(pDX, m_L0SpecularBlue, 0., 1.);
}

// ViewLightingSettingsDialog message handlers
