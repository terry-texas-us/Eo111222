#pragma once
#include "Resource.h"

/// <summary>Modifies attributes of all group primatives in current trap to current settings.</summary>
/// <remarks>Trap color index is not modified.</remarks>
class EoDlgTrapModify : public CDialog {
  DECLARE_DYNAMIC(EoDlgTrapModify)

 public:
  EoDlgTrapModify(CWnd* parent = nullptr);
  EoDlgTrapModify(AeSysDoc* document, CWnd* parent = nullptr);

  EoDlgTrapModify(const EoDlgTrapModify&) = delete;
  EoDlgTrapModify& operator=(const EoDlgTrapModify&) = delete;

  virtual ~EoDlgTrapModify();

  // Dialog Data
  enum { IDD = IDD_TRAP_MODIFY };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual void OnOK();

  AeSysDoc* m_Document{};

 public:
  void ModifyPolygons();
};

const int TM_TEXT_ALL = 0;
const int TM_TEXT_FONT = 1;
const int TM_TEXT_HEIGHT = 2;
