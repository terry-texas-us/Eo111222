#pragma once

#include "Resource.h"

class EoDlgFileManageLayers : public CDialog {
 public:
  EoDlgFileManageLayers(CWnd* parent = nullptr);
  EoDlgFileManageLayers(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgFileManageLayers(const EoDlgFileManageLayers&) = delete;
  EoDlgFileManageLayers& operator=(const EoDlgFileManageLayers&) = delete;

  virtual ~EoDlgFileManageLayers();

  enum { IDD = IDD_FILE_MANAGE_LAYERS };

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;

 public:
  enum LayerListColumnLabels { Status, Name, On, Freeze, Lock, Plot, Color, LineType, LineWeight };

  AeSysDoc* m_Document{};
  CImageList m_stateImages;
  CToolTipCtrl m_toolTip;

  CListCtrl m_layersListControl;
  CEdit m_groups;

  int m_numberOfColumns{};
  bool m_clickToColumnName{};
  int m_selectedItemForEdit{-1};
  int m_preClickSelectedItem{-1};
  int m_editItemForRepos{-1};
  HWND m_previewWindowHandle{};

  void DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& rcItem);

  afx_msg void OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct);

  /** @brief Handles clicks on the layers list control.
   * If the "On" column is clicked, the layer's visibility is toggled.
   * If the "Freeze" column is clicked, the layer's frozen state is toggled.
   * If the "Lock" column is clicked, the layer's static state is toggled.
   * If the "Color" column is clicked, a color selection dialog is opened to change the layer's color.
   * If the "LineType" column is clicked, a line type selection dialog is opened to change the layer's line type.
   * @param pNMHDR Pointer to the NMHDR structure that contains the notification code and other information about the
   * event.
   * @param pResult Pointer to an LRESULT variable that receives the result of the message processing. The value should
   * be set to 0 if the message is handled.
   */
  afx_msg void OnNMClickLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnNMRclickLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);

  /** @brief Handles double-clicks on the layers list control.
   * If the layer name column is double-clicked, the rename dialog is opened.
   * If the status column is double-clicked, the layer is set as the work layer.
   * @param pNMHDR Pointer to the NMHDR structure that contains the notification code and other information about the
   * event.
   * @param result Pointer to an LRESULT variable that receives the result of the message processing. The value should
   * be set to 0 if the message is handled.
   */
  afx_msg void OnNMDblclkLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);

  afx_msg void OnBnClickedMfcbuttonNew();

  afx_msg void OnBnClickedMfcbuttonDel();
  afx_msg void OnItemchangedLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnLvnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnLvnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg LRESULT OnDeferredEditLabel(WPARAM wParam, LPARAM lParam);
  afx_msg void OnTimer(UINT_PTR nIDEvent);

 protected:
  BOOL PreTranslateMessage(MSG* pMsg) override;
  DECLARE_MESSAGE_MAP()
};
