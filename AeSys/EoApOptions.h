#pragma once

class EoApOptions {
 public:
  EoApOptions();
  ~EoApOptions();

  enum TabsStyle { None, Standard, Grouped };
  TabsStyle m_tabsStyle;

  CMDITabInfo m_mdiTabInfo;

  BOOL m_tabsContextMenu;
  BOOL m_disableSetRedraw;

 public:
  void Load();
  void Save() const;
};
