#include "Stdafx.h"

#include "AeSys.h"
#include "EoApOptions.h"

EoApOptions::EoApOptions() {}
EoApOptions::~EoApOptions() {}
void EoApOptions::Load() {
  m_viewBackground = static_cast<Eo::ViewBackground>(app.GetInt(L"ViewBackground", static_cast<int>(Eo::ViewBackground::Dark)));
}
void EoApOptions::Save() const {
  app.WriteInt(L"ViewBackground", static_cast<int>(m_viewBackground));
}
