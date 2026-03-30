#pragma once

#include "Eo.h"

class EoApOptions {
 public:
  EoApOptions();
  ~EoApOptions();

  enum TabsStyle { None, Standard, Grouped };
  TabsStyle m_tabsStyle;

  CMDITabInfo m_mdiTabInfo;

  BOOL m_tabsContextMenu;
  BOOL m_disableSetRedraw;

  /// @brief Active view color scheme (Dark or Light). Persisted to the registry.
  Eo::ColorScheme m_colorScheme{Eo::ColorScheme::Dark};

 public:
  void Load();
  void Save() const;
};
