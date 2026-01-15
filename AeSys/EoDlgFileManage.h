#pragma once
#include "Resource.h"

class EoDlgFileManage : public CDialog {
  DECLARE_DYNAMIC(EoDlgFileManage)

 public:
  EoDlgFileManage(CWnd* parent = nullptr);
  EoDlgFileManage(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgFileManage(const EoDlgFileManage&) = delete;
  EoDlgFileManage& operator=(const EoDlgFileManage&) = delete;

  virtual ~EoDlgFileManage();

  // Dialog Data
  enum { IDD = IDD_FILE_MANAGE };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;

 public:
  enum LayerListColumnLabels {
    Status,
    Name,
    On,
    Freeze,
    Lock,
    Color,
    LineType,
    LineWeight,
    PlotStyle,
    Plot,
    VpFreeze,
    VpColor,
    VpLineType,
    VpLineWeight,
    VpPlotStyle,
    Descr
  };

  AeSysDoc* m_Document{nullptr};
  CImageList m_StateImages;

  CListBox m_BlocksList;
  CListCtrl m_LayersListControl;
  CListBox m_TracingList;
  CButton m_TracingCloakRadioButton;
  CButton m_TracingOpenRadioButton;
  CButton m_TracingMapRadioButton;
  CButton m_TracingViewRadioButton;
  CEdit m_Groups;
  CStatic m_References;

  int NumberOfColumns{0};
  bool m_ClickToColumnName{false};

  HWND m_PreviewWindowHandle{nullptr};

  void DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& rcItem);

  afx_msg void OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct);

  afx_msg void OnBnClickedLayerRename();
  /// <summary>Selected layer is converted to a tracing.</summary>
  afx_msg void OnBnClickedLayerMelt();
  afx_msg void OnBnClickedTracingOpen();
  afx_msg void OnBnClickedTracingMap();
  afx_msg void OnBnClickedTracingView();
  afx_msg void OnBnClickedTracingCloak();
  afx_msg void OnBnClickedTracingFuse();
  afx_msg void OnBnClickedTracingExclude();
  afx_msg void OnBnClickedTracingInclude();
  afx_msg void OnLbnSelchangeBlocksList();
  afx_msg void OnLbnSelchangeTracingList();
  afx_msg void OnNMClickLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnNMDblclkLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnBnClickedMfcbuttonWork();
  afx_msg void OnBnClickedMfcbuttonNew();
  /// <summary>Selected layer is deleted.</summary>
  /// <remarks>Hot layer must be warm'ed before it may be deleted. Layer "0" may never be deleted.</remarks>
  afx_msg void OnBnClickedMfcbuttonDel();
  afx_msg void OnItemchangedLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);

 protected:
  DECLARE_MESSAGE_MAP()
};
