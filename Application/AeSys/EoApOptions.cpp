#include "Stdafx.h"

#include "AeSys.h"
#include "EoApOptions.h"

EoApOptions::EoApOptions() {}
EoApOptions::~EoApOptions() {}
void EoApOptions::Load() {
  m_colorScheme = static_cast<Eo::ColorScheme>(app.GetInt(L"ColorScheme", static_cast<int>(Eo::ColorScheme::Dark)));
}
void EoApOptions::Save() const {
  app.WriteInt(L"ColorScheme", static_cast<int>(m_colorScheme));
}
