#pragma once
#include "Resource.h"

class EoDlgFileManage : public CDialog {
 public:
  EoDlgFileManage(CWnd* parent = nullptr);
  EoDlgFileManage(AeSysDoc* document, CWnd* parent = nullptr);
  EoDlgFileManage(const EoDlgFileManage&) = delete;
  EoDlgFileManage& operator=(const EoDlgFileManage&) = delete;

  virtual ~EoDlgFileManage();

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
    Plot,
    Color,
    LineType,
    LineWeight
  };

  AeSysDoc* m_Document{};
  CImageList m_stateImages;

  CListBox m_blocksList;
  CListCtrl m_layersListControl;
  CListBox m_tracingList;
  CButton m_tracingCloakRadioButton;
  CButton m_tracingOpenRadioButton;
  CButton m_tracingMapRadioButton;
  CButton m_tracingViewRadioButton;
  CEdit m_groups;
  CStatic m_references;

  int m_numberOfColumns{};
  bool m_clickToColumnName{};
  HWND m_previewWindowHandle{};

  void DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& rcItem);

  afx_msg void OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct);

  afx_msg void OnBnClickedLayerRename();
  /// @brief Selected layer is converted to a tracing.
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

  /** @brief Handles double-clicks on the layers list control.
   * If the layer name column is double-clicked, the rename dialog is opened.
   * If the status column is double-clicked, the layer is set as the work layer.
   * @param pNMHDR Pointer to the NMHDR structure that contains the notification code and other information about the
   * event.
   * @param result Pointer to an LRESULT variable that receives the result of the message processing. The value should
   * be set to 0 if the message is handled.
   */
  afx_msg void OnNMDblclkLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnBnClickedMfcbuttonWork();

  /** @brief Creates a new layer with a unique name and adds it to the document and the list control.
   */
  afx_msg void OnBnClickedMfcbuttonNew();

  /// @brief Selected layer is deleted.
  /// <remarks>Hot layer must be warm'ed before it may be deleted. Layer "0" may never be deleted.</remarks>
  afx_msg void OnBnClickedMfcbuttonDel();
  afx_msg void OnItemchangedLayersListControl(NMHDR* pNMHDR, LRESULT* pResult);

 protected:
  DECLARE_MESSAGE_MAP()
};
