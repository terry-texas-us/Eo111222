#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "Preview.h"

UINT CALLBACK OFNHookProcFileTracing(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
	AeSysDoc* Document = AeSysDoc::GetDoc();
	AeSysView* ActiveView = AeSysView::GetActiveView();

	switch (uiMsg) {
	case WM_INITDIALOG:
		WndProcPreviewClear(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW));
		return (TRUE);

	case WM_NOTIFY: {
			LPOFNOTIFY lpofn;
			lpofn = (LPOFNOTIFY) lParam;
			if (lpofn->hdr.code == CDN_FOLDERCHANGE) {
				WndProcPreviewClear(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW));
			}
			else if (lpofn->hdr.code == CDN_SELCHANGE) {
				WCHAR psz[MAX_PATH];
				memset(psz, 0, MAX_PATH);
				::SendMessage(GetParent(hDlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM) (LPTSTR) psz);

				CFileStatus	fs;
				if (CFile::GetStatus(psz, fs)) {
					EoDbLayer* Layer = Document->GetLayerTableLayer(psz);

					if (Layer != 0) {
						_WndProcPreviewUpdate(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), Layer);
					}
					else {
						Layer = new EoDbLayer(L"",  EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive);

						Document->TracingLoadLayer(psz, Layer);
						_WndProcPreviewUpdate(::GetDlgItem(hDlg, IDC_LAYER_PREVIEW), Layer);

						Layer->DeleteGroupsAndRemoveAll();
						delete Layer;
					}
				}
			}
			return (TRUE);
		}
	case WM_COMMAND: {
			WCHAR szFilePath[MAX_PATH];
			memset(szFilePath, 0, MAX_PATH);
			::SendMessage(GetParent(hDlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM) (LPTSTR) szFilePath);
			CFileStatus	fs;
			if (!CFile::GetStatus(szFilePath, fs)) {
				app.WarningMessageBox(IDS_MSG_FILE_NOT_FOUND, szFilePath);
				return (TRUE);
			}
			switch (LOWORD(wParam)) {
			case IDC_APPEND: {
					EoDbLayer*	Layer = Document->GetWorkLayer();

					Document->TracingLoadLayer(szFilePath, Layer);
					Document->UpdateAllViews(NULL, EoDb::kLayerSafe, Layer);
					return (TRUE);
				}

			case IDC_TRAMAP:
				Document->TracingMap(szFilePath);
				return (TRUE);

			case IDC_TRAP: {
					EoDbLayer* pLayer = new EoDbLayer(L"", EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive);

					Document->TracingLoadLayer(szFilePath, pLayer);

					Document->RemoveAllTrappedGroups();
					Document->AddGroupsToTrap(pLayer);
					Document->CopyTrappedGroupsToClipboard(ActiveView);
					Document->RemoveAllTrappedGroups();

					pLayer->DeleteGroupsAndRemoveAll();
					delete pLayer;

					return (TRUE);
				}

			case IDC_TRAVIEW:
				Document->TracingView(szFilePath);

				return (TRUE);
			}
		}
	}
	return (FALSE); 		// Message for default dialog handlers
}

