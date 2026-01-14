#pragma once

#if defined(USING_ODA)

#include "LyLayerFilter.h"

// EoDlgLayerPropertiesManager dialog

class EoDlgLayerPropertiesManager : public CDialog {
  DECLARE_DYNAMIC(EoDlgLayerPropertiesManager)

 public:
  EoDlgLayerPropertiesManager(CWnd* parent = nullptr);
  EoDlgLayerPropertiesManager(OdDbDatabase* database, CWnd* parent = nullptr);

  virtual ~EoDlgLayerPropertiesManager();

  // Dialog Data
  enum { IDD = IDD_LAYER_PROPERTIES_MANAGER };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()
 public:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct);
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg void OnSizing(UINT side, LPRECT rectangle);
  afx_msg void OnBnClickedButtonAdd();
  afx_msg void OnBnClickedButtonCurrent();
  afx_msg void OnNMDblclkLayerFilterTree(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnTvnKeydownLayerFilterTree(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnNMClickListLayersList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnNMDblclkListLayersList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnNMRClickListLayersList(NMHDR* pNMHDR, LRESULT* pResult);

 public:
  enum ColumnLabels {
    column_status,
    column_name,
    column_on,
    column_freeze,
    column_lock,
    column_color,
    column_linetype,
    column_lineweight,
    column_plotstyle,
    column_plot,
    column_vpfreeze,
    column_vpcolor,
    column_vplinetype,
    column_vplineweight,
    column_vpplotstyle,
    column_descr
  };
  int m_InititialWidth;
  int m_InititialHeight;

  int m_DeltaWidth;
  int m_DeltaHeight;

  int column_desc;
  int columns_count;

  OdDbDatabase* m_Database;
  OdDbObjectId m_ActiveViewport;

  bool m_ClickToColumnName;
  CListCtrl m_LayerList;
  CTreeCtrl m_TreeFilters;

  OdLyLayerFilterPtr m_pRootFilter;

  CImageList m_pTreeImages;
  CImageList m_stateImages;

  bool OpenSelectedLayer(OdSmartPtr<OdDbLayerTableRecord>& layer);
  void UpdateFiltersTree();
  void UpdateCurrentLayerInfoField();
  void DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& rcItem);
};
#endif
