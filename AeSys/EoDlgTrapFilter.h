#pragma once
#include "Resource.h"

class EoDlgTrapFilter : public CDialog {
 public:
  EoDlgTrapFilter(CWnd* pParent = nullptr);
  EoDlgTrapFilter(AeSysDoc* document, CWnd* pParent = nullptr);
  EoDlgTrapFilter(const EoDlgTrapFilter&) = delete;
  EoDlgTrapFilter& operator=(const EoDlgTrapFilter&) = delete;

  virtual ~EoDlgTrapFilter();

  enum { IDD = IDD_TRAP_FILTER };

  AeSysDoc* m_Document{};

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

 public:
  CComboBox m_FilterLineComboBoxControl;
  CListBox m_FilterPrimitiveTypeListBoxControl;

  void FilterByColor(EoInt16 colorIndex);
  void FilterByLineType(int lineType);

  /**
   * Filters trapped groups by the specified primitive type.
   *
   * This function iterates through all trapped groups in the document and checks each primitive within those groups.
   * If a primitive matches the specified primitive type, the entire group is removed from the trapped list.
   * After filtering, the view is updated to reflect the changes in the trapped groups.
   *
   * @param primitiveType The type of primitive to filter by, as defined in EoDb::PrimitiveTypes.
   */
  void FilterByPrimitiveType(const EoDb::PrimitiveTypes primitiveType);
};
