#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbLayer.h"
#include "Preview.h"
#include "Resource.h"

UINT_PTR CALLBACK OFNHookProcFileTracing(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
  auto* document = AeSysDoc::GetDoc();
  auto* activeView = AeSysView::GetActiveView();

  switch (uiMsg) {
    case WM_INITDIALOG:
      WndProcPreviewClear(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW));
      return (TRUE);

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
            layer = new EoDbLayer(L"", EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive);

            document->TracingLoadLayer(psz, layer);
            WndProcPreviewUpdateLayer(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), layer);

            layer->DeleteGroupsAndRemoveAll();
            delete layer;
          }
        }
      }
      return (TRUE);
    }
    case WM_COMMAND: {
      wchar_t szFilePath[MAX_PATH]{};
      ::SendMessage(GetParent(hDlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM)(LPTSTR)szFilePath);
      CFileStatus fs;
      if (!CFile::GetStatus(szFilePath, fs)) {
        app.WarningMessageBox(IDS_MSG_FILE_NOT_FOUND, szFilePath);
        return (TRUE);
      }
      switch (LOWORD(wParam)) {
        case IDC_APPEND: {
          EoDbLayer* Layer = document->GetWorkLayer();

          document->TracingLoadLayer(szFilePath, Layer);
          document->UpdateAllViews(nullptr, EoDb::kLayerSafe, Layer);
          return (TRUE);
        }

        case IDC_TRAMAP:
          document->TracingMap(szFilePath);
          return (TRUE);

        case IDC_TRAP: {
          EoDbLayer* pLayer = new EoDbLayer(L"", EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive);

          document->TracingLoadLayer(szFilePath, pLayer);

          document->RemoveAllTrappedGroups();
          document->AddGroupsToTrap(pLayer);
          document->CopyTrappedGroupsToClipboard(activeView);
          document->RemoveAllTrappedGroups();

          pLayer->DeleteGroupsAndRemoveAll();
          delete pLayer;

          return (TRUE);
        }

        case IDC_TRAVIEW:
          document->TracingView(szFilePath);

          return (TRUE);
      }
    }
  }
  return (FALSE);  // Message for default dialog handlers
}
