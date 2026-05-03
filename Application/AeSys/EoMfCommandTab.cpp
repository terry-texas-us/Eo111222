#include "Stdafx.h"

#include "EoMfCommandTab.h"

#include <algorithm>
#include <format>
#include <functional>
#include <vector>

#include "AeSys.h"
#include "AeSysState.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoCommandLineMessages.h"
#include "EoCommandRegistry.h"
#include "EoCommandTokenizer.h"
#include "EoGePoint3d.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoMfCommandTab, CWnd)

BEGIN_MESSAGE_MAP(EoMfCommandTab, CWnd)
ON_WM_CREATE()
ON_WM_SIZE()
ON_WM_SETFOCUS()
END_MESSAGE_MAP()

BOOL EoMfCommandTab::Create(const RECT& rect, CWnd* parent, UINT id) {
  return CWnd::Create(nullptr, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, rect, parent, id);
}

int EoMfCommandTab::OnCreate(LPCREATESTRUCT createStruct) {
  if (CWnd::OnCreate(createStruct) == -1) { return -1; }

  m_font.CreateStockObject(DEFAULT_GUI_FONT);

  const CRect emptyRect{};
  constexpr DWORD listStyles =
      WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL;
  if (!m_history.Create(listStyles, emptyRect, this, IDC_COMMAND_HISTORY)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create command history list\n");
    return -1;
  }
  m_history.SetFont(&m_font);

  constexpr DWORD editStyles = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT;
  if (!m_edit.Create(editStyles, emptyRect, this, IDC_COMMAND_EDIT)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create command edit\n");
    return -1;
  }
  m_edit.SetFont(&m_font);

  AppendHistory(L"Command line ready. Type a command and press Enter.");
  return 0;
}

void EoMfCommandTab::OnSize(UINT type, int cx, int cy) {
  CWnd::OnSize(type, cx, cy);

  if (m_edit.GetSafeHwnd() == nullptr || m_history.GetSafeHwnd() == nullptr) { return; }

  const int editTop = std::max(0, cy - kEditHeight);
  const int historyHeight = std::max(0, editTop - kGap);

  m_history.SetWindowPos(nullptr, 0, 0, cx, historyHeight, SWP_NOZORDER | SWP_NOACTIVATE);
  m_edit.SetWindowPos(nullptr, 0, editTop, cx, kEditHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

void EoMfCommandTab::OnSetFocus(CWnd* /*oldWnd*/) {
  FocusCommandEdit();
  ShowModePrompt();
}

void EoMfCommandTab::FocusCommandEdit() {
  if (m_edit.GetSafeHwnd() != nullptr) { m_edit.SetFocus(); }
}

void EoMfCommandTab::AppendHistory(const std::wstring& line) {
  if (m_history.GetSafeHwnd() == nullptr) { return; }
  const int index = m_history.AddString(line.c_str());
  if (index >= 0) { m_history.SetTopIndex(index); }
}

void EoMfCommandTab::ApplyColorScheme() {
  m_history.SetColors(Eo::chromeColors.paneBackground, Eo::chromeColors.paneText);
}

void EoMfCommandTab::ShowModePrompt() {
  auto* view = AeSysView::GetActiveView();
  if (view == nullptr) { return; }
  auto* state = view->GetCurrentState();
  if (state == nullptr) { return; }
  const wchar_t* prompt = state->PromptString();
  if (prompt != nullptr && *prompt != L'\0') { AppendHistory(prompt); }
}

void EoMfCommandTab::TryTabComplete(const std::wstring& partial) {
  const auto upper = EoCommandTokenizer::NormalizeCommand(partial);

  // Invalidate the completion cycle if the user has typed something different.
  if (upper != m_tabPrefix) {
    m_tabPrefix = upper;
    m_tabCandidates.clear();
    m_tabIndex = 0;
    if (upper.empty()) { return; }
    for (const auto& key : EoCommandRegistry::Instance().AllKeys()) {
      if (key.size() >= upper.size() && key.compare(0, upper.size(), upper) == 0) {
        m_tabCandidates.push_back(key);
      }
    }
    if (m_tabCandidates.empty()) { return; }
    // Sort: exact matches first, then shorter keys, then alphabetical.
    std::sort(m_tabCandidates.begin(), m_tabCandidates.end(),
        [&upper](const std::wstring& a, const std::wstring& b) {
          if (a == upper) { return true; }
          if (b == upper) { return false; }
          if (a.size() != b.size()) { return a.size() < b.size(); }
          return a < b;
        });
  } else {
    // Successive Tab presses cycle forward through the candidate list.
    if (m_tabCandidates.empty()) { return; }
    m_tabIndex = (m_tabIndex + 1) % m_tabCandidates.size();
  }

  const auto& match = m_tabCandidates[m_tabIndex];
  if (m_edit.GetSafeHwnd() != nullptr) {
    m_edit.SetWindowTextW(match.c_str());
    m_edit.SetSel(static_cast<int>(match.size()), static_cast<int>(match.size()));
    // Keep prefix in sync so the next Tab press correctly cycles.
    m_tabPrefix = upper;
  }
}

std::wstring EoMfCommandTab::RecallPreviousCommand() {
  if (m_submittedHistory.empty()) { return {}; }
  if (m_recallIndex > 0) { --m_recallIndex; }
  return m_submittedHistory[m_recallIndex];
}

std::wstring EoMfCommandTab::RecallNextCommand() {
  if (m_submittedHistory.empty()) { return {}; }
  if (m_recallIndex < m_submittedHistory.size()) { ++m_recallIndex; }
  if (m_recallIndex >= m_submittedHistory.size()) { return {}; }
  return m_submittedHistory[m_recallIndex];
}

namespace {

/// @brief Parses every coordinate token in @p tokens starting at @p firstIndex,
///        resolving relative offsets against the running cursor seeded from
///        @p initialCursor. Bad tokens are reported via @p reportBad.
struct ParsedCoord {
  EoGePoint3d world;
  std::wstring sourceToken;
};

[[nodiscard]] std::vector<ParsedCoord> ParseCoordinateRun(
    const std::vector<std::wstring>& tokens, std::size_t firstIndex,
    const EoGePoint3d& initialCursor,
    const std::function<void(const std::wstring&)>& reportBad) {
  std::vector<ParsedCoord> result;
  EoGePoint3d cursor = initialCursor;
  for (std::size_t i = firstIndex; i < tokens.size(); ++i) {
    EoCommandTokenizer::EoCoordinate parsed{};
    if (!EoCommandTokenizer::TryParseCoordinate(tokens[i], parsed)) {
      if (reportBad) { reportBad(tokens[i]); }
      continue;
    }
    EoGePoint3d worldPoint{parsed.x, parsed.y, parsed.hasZ ? parsed.z : 0.0};
    if (parsed.isRelative) {
      worldPoint.x += cursor.x;
      worldPoint.y += cursor.y;
      if (parsed.hasZ) { worldPoint.z += cursor.z; }
    }
    cursor = worldPoint;
    result.push_back({worldPoint, tokens[i]});
  }
  return result;
}

}  // namespace

void EoMfCommandTab::ExecuteCommand(const std::wstring& commandLine) {
  const auto tokens = EoCommandTokenizer::SplitTokens(commandLine);
  if (tokens.empty()) { return; }

  // Record the raw submitted line for Up/Down recall and reset the recall cursor.
  if (m_submittedHistory.empty() || m_submittedHistory.back() != commandLine) {
    m_submittedHistory.push_back(commandLine);
  }
  m_recallIndex = m_submittedHistory.size();
  // Reset Tab-completion cycle so the next Tab starts fresh.
  m_tabCandidates.clear();
  m_tabIndex = 0;
  m_tabPrefix.clear();

  AeSysView* activeView = AeSysView::GetActiveView();

  // --- Coordinate-only continuation: no leading verb, just point tokens. ---
  // Inject every parsed coordinate as a synthetic click into the active view,
  // letting the currently active mode/state consume them in sequence. This
  // lets the user advance an in-progress line/polyline/circle without retyping
  // the verb on each subsequent line.
  if (EoCommandTokenizer::IsCoordinateToken(tokens.front())) {
    AppendHistory(L"> " + commandLine);
    if (activeView == nullptr) { return; }
    const EoGePoint3d cursorWorld = activeView->GetCursorPosition();
    auto coords = ParseCoordinateRun(tokens, 0, cursorWorld,
        [this](const std::wstring& bad) { AppendHistory(L"  Bad coordinate: " + bad); });
    for (const auto& coord : coords) {
      auto* heapPoint = new EoGePoint3d{coord.world};
      activeView->PostMessageW(WM_CMDLINE_INJECT_POINT, 0, reinterpret_cast<LPARAM>(heapPoint));
      AppendHistory(std::format(L"  Point: {:.4f}, {:.4f}", coord.world.x, coord.world.y));
    }
    activeView->SetFocus();
    return;
  }

  const auto verb = EoCommandTokenizer::NormalizeCommand(tokens.front());
  if (verb.empty()) { return; }

  AppendHistory(L"> " + verb);

  // Built-in: HELP / ? lists registered commands without dispatching anything.
  if (verb == L"HELP" || verb == L"?") {
    const auto& entries = EoCommandRegistry::Instance().Entries();
    AppendHistory(L"Registered commands:");
    for (const auto& entry : entries) {
      std::wstring line = L"  " + entry.canonicalName;
      if (!entry.helpText.empty()) { line += L" - " + entry.helpText; }
      AppendHistory(line);
    }
    return;
  }

  const auto* entry = EoCommandRegistry::Instance().Find(verb);
  if (entry == nullptr) {
    AppendHistory(L"Unknown command: " + verb);
    return;
  }

  auto* mainFrame = AfxGetMainWnd();
  if (mainFrame == nullptr) { return; }

  // Functor commands (ZOOM, UNITS, PURGE, etc.) — invoke directly, no WM_COMMAND.
  if (entry->functor) {
    entry->functor();
    return;
  }

  // --- Parse all coordinate tokens before dispatching anything. ---
  // This lets us pin the first coordinate during opId activation so that the
  // draw-mode handler (OnDrawModeCircle, OnDrawModeLine, etc.) captures the
  // correct first point instead of wherever the OS cursor happens to be parked.
  const EoGePoint3d cursorWorld =
      (activeView != nullptr) ? activeView->GetCursorPosition() : EoGePoint3d{};
  auto coords = ParseCoordinateRun(tokens, 1, cursorWorld,
      [this](const std::wstring& bad) { AppendHistory(L"  Bad coordinate: " + bad); });

  // Dispatch the mode-switch command (never reads cursor position).
  mainFrame->SendMessageW(WM_COMMAND, MAKEWPARAM(entry->modeId, 0), 0);

  // Dispatch the sub-op command (e.g. ID_DRAW_MODE_CIRCLE).
  // Many sub-op handlers call GetCursorPosition() immediately to capture the
  // first point of the gesture (center, line-start, etc.).  Pin the first
  // coordinate — if any — around this SendMessage so the handler reads the
  // commanded world point, not the OS cursor position.
  std::size_t injectStart = 0;  // index into coords[] for the post-inject loop
  if (entry->opId != 0) {
    if (activeView != nullptr && !coords.empty()) {
      activeView->PinCursorWorld(coords[0].world);
      mainFrame->SendMessageW(WM_COMMAND, MAKEWPARAM(entry->opId, 0), 0);
      activeView->UnpinCursorWorld();
      // Move the OS cursor to the first point for visual coherence.
      activeView->SetCursorPosition(coords[0].world);
      AppendHistory(std::format(L"  Point: {:.4f}, {:.4f}", coords[0].world.x, coords[0].world.y));
      injectStart = 1;  // first coord already consumed by activation
    } else {
      mainFrame->SendMessageW(WM_COMMAND, MAKEWPARAM(entry->opId, 0), 0);
    }
  }

  // Post the remaining coordinates as inject messages (one per pump cycle, in order).
  if (activeView != nullptr) {
    for (std::size_t i = injectStart; i < coords.size(); ++i) {
      auto* heapPoint = new EoGePoint3d{coords[i].world};
      activeView->PostMessageW(WM_CMDLINE_INJECT_POINT, 0, reinterpret_cast<LPARAM>(heapPoint));
      AppendHistory(std::format(L"  Point: {:.4f}, {:.4f}", coords[i].world.x, coords[i].world.y));
    }
  }

  // Hand focus back to the active view so the user can immediately click points.
  if (activeView != nullptr) { activeView->SetFocus(); }
}
