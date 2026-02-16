#include "Stdafx.h"

#include <cstdint>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbLayer.h"
#include "Resource.h"
#include "WndProcPreview.h"

UINT_PTR CALLBACK OFNHookProcFileTracing(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
  auto* document = AeSysDoc::GetDoc();
  auto* activeView = AeSysView::GetActiveView();

  switch (uiMsg) {
    case WM_INITDIALOG:
      WndProcPreviewClear(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW));
      return TRUE;

    case WM_NOTIFY: {
      LPOFNOTIFY lpofn;
      lpofn = (LPOFNOTIFY)lParam;
      if (lpofn->hdr.code == CDN_FOLDERCHANGE) {
        WndProcPreviewClear(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW));
      } else if (lpofn->hdr.code == CDN_SELCHANGE) {
        wchar_t psz[MAX_PATH]{};
        ::SendMessage(GetParent(hDlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM)(LPTSTR)psz);

        CFileStatus fs;
        if (CFile::GetStatus(psz, fs)) {
          auto* layer = document->GetLayerTableLayer(psz);

          if (layer != nullptr) {
            WndProcPreviewUpdateLayer(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), layer);
          } else {
            constexpr EoDbLayer::State commonState =
                EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;
            layer = new EoDbLayer(L"", commonState);

            document->TracingLoadLayer(psz, layer);
            WndProcPreviewUpdateLayer(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), layer);

            layer->DeleteGroupsAndRemoveAll();
            delete layer;
          }
        }
      }
      return TRUE;
    }
    case WM_COMMAND: {
      wchar_t filePath[MAX_PATH]{};
      ::SendMessage(GetParent(hDlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM)(LPTSTR)filePath);
      CFileStatus fs;
      if (!CFile::GetStatus(filePath, fs)) {
        app.WarningMessageBox(IDS_MSG_FILE_NOT_FOUND, filePath);
        return TRUE;
      }
      switch (LOWORD(wParam)) {
        case IDC_APPEND: {
          auto* Layer = document->GetWorkLayer();

          document->TracingLoadLayer(filePath, Layer);
          document->UpdateAllViews(nullptr, EoDb::kLayerSafe, Layer);
          return TRUE;
        }

        case IDC_TRAMAP:
          document->TracingMap(filePath);
          return TRUE;

        case IDC_TRAP: {
          constexpr EoDbLayer::State commonState =
              EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;
          auto* layer = new EoDbLayer(L"", commonState);

          document->TracingLoadLayer(filePath, layer);

          document->RemoveAllTrappedGroups();
          document->AddGroupsToTrap(layer);
          document->CopyTrappedGroupsToClipboard(activeView);
          document->RemoveAllTrappedGroups();

          layer->DeleteGroupsAndRemoveAll();
          delete layer;

          return TRUE;
        }

        case IDC_TRAVIEW:
          document->TracingView(filePath);

          return TRUE;
      }
    }
  }
  return FALSE;  // Message for default dialog handlers
}
