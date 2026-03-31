#pragma once

#include "Eo.h"

class EoApOptions {
 public:
  EoApOptions();
  ~EoApOptions();

  /// @brief Active view color scheme (Dark or Light). Persisted to the registry.
  Eo::ColorScheme m_colorScheme{Eo::ColorScheme::Dark};

 public:
  void Load();
  void Save() const;
};
