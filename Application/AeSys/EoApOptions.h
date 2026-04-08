#pragma once

#include "Eo.h"

class EoApOptions {
 public:
  EoApOptions();
  ~EoApOptions();

  /// @brief Active view background preference (Dark or White). Persisted to the registry.
  Eo::ViewBackground m_viewBackground{Eo::ViewBackground::Dark};

 public:
  void Load();
  void Save() const;
};
