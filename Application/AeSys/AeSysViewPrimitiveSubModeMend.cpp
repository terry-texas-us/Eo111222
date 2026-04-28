#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "PrimitiveMendState.h"
#include "Resource.h"

void AeSysView::OnModePrimitiveMend() {
  // Save primary mode context so PrimitiveMendState::OnExit can restore it.
  app.SetPrimaryMode(app.CurrentMode());
  app.SetPrimaryModeResourceIdentifier(app.ModeResourceIdentifier());

  auto state = std::make_unique<PrimitiveMendState>();
  // OnEnter performs the selection; only push if something was engaged.
  state->OnEnter(this);
  if (!state->IsEngaged()) { return; }

  // Push with OnEnter already called — use the raw push to avoid double-OnEnter.
  if (!m_stateStack.empty()) { m_stateStack.top()->OnExit(this); }
  m_stateStack.push(std::move(state));
  Invalidate();
}

