#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "Resource.h"

namespace {
constexpr int statusOp0{3};  // mode panes start at index 3 (after message, length, angle)
}

void AeSysView::ModeLineDisplay() {
  if (app.CurrentMode() == 0) { return; }

  const auto modeInformation = App::LoadStringResource(UINT(app.CurrentMode()));

  CString paneText;

  auto* context = GetDC();
  const auto& schemeColors = Eo::chromeColors;

  for (int i = 0; i < 10; i++) {
    AfxExtractSubString(paneText, modeInformation, i + 1, '\n');

    const auto textExtent = context->GetTextExtent(paneText);

    GetStatusBar().SetPaneInfo(::statusOp0 + i, static_cast<UINT>(ID_OP0 + i), SBPS_NORMAL, textExtent.cx);
    GetStatusBar().SetPaneText(::statusOp0 + i, paneText);
    GetStatusBar().SetTipText(::statusOp0 + i, L"Mode Command Tip Text");

    bool isHighlighted = (m_OpHighlighted != 0 && static_cast<std::uint16_t>(ID_OP0 + i) == m_OpHighlighted);

    if (isHighlighted) {
      GetStatusBar().SetPaneBackgroundColor(::statusOp0 + i, schemeColors.captionActiveBackground);
      GetStatusBar().SetPaneTextColor(::statusOp0 + i, schemeColors.captionActiveText);
    } else {
      GetStatusBar().SetPaneBackgroundColor(::statusOp0 + i);
      GetStatusBar().SetPaneTextColor(::statusOp0 + i, schemeColors.statusBarText);
    }
  }
  ReleaseDC(context);
}

std::uint16_t AeSysView::ModeLineHighlightOp(std::uint16_t command) {
  ModeLineUnhighlightOp(m_OpHighlighted);

  m_OpHighlighted = command;

  if (command == 0) { return 0; }
  const int paneIndex = ::statusOp0 + m_OpHighlighted - ID_OP0;

  const auto& schemeColors = Eo::chromeColors;
  GetStatusBar().SetPaneBackgroundColor(paneIndex, schemeColors.captionActiveBackground);
  GetStatusBar().SetPaneTextColor(paneIndex, schemeColors.captionActiveText);

  return command;
}

void AeSysView::ModeLineUnhighlightOp(std::uint16_t& command) {
  if (command == 0 || m_OpHighlighted == 0) { return; }
  const int paneIndex = ::statusOp0 + m_OpHighlighted - ID_OP0;

  const auto& schemeColors = Eo::chromeColors;
  GetStatusBar().SetPaneBackgroundColor(paneIndex);
  GetStatusBar().SetPaneTextColor(paneIndex, schemeColors.statusBarText);

  command = 0;
  m_OpHighlighted = 0;
}
