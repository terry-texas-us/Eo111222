#pragma once
#include "Resource.h"

/// @brief Modifies attributes of all group primatives in current trap to current settings.
/// @note Trap color index is not modified.
class EoDlgTrapModify : public CDialog {
 public:
  EoDlgTrapModify(CWnd* parent = nullptr);
  EoDlgTrapModify(AeSysDoc* document, CWnd* parent = nullptr);

  EoDlgTrapModify(const EoDlgTrapModify&) = delete;
  EoDlgTrapModify& operator=(const EoDlgTrapModify&) = delete;

  virtual ~EoDlgTrapModify();

  enum { IDD = IDD_TRAP_MODIFY };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);
  virtual void OnOK();

  AeSysDoc* m_Document{};

 public:
  void ModifyPolygons();
};

constexpr int TM_TEXT_ALL{0};
constexpr int TM_TEXT_FONT{1};
constexpr int TM_TEXT_HEIGHT{2};