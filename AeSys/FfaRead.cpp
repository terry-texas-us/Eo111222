#include "stdafx.h"
#include "AeSysDoc.h"

// Format: 
// line 1: // <name of the .ffa file>
// line 2: //
// line 3 - end: <4 char key> // <var number of printable chars describing key>
// The key identifies the .jb1 to use. The description becomes its .peg name.

// TODO: Need to test ffa read when open pass non peg files

void ffaReadFile(const CString& strPathName)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CStdioFile fl;
						
	if (fl.Open(strPathName, CFile::modeRead | CFile::typeText))
	{
		WCHAR szLine[256];
		while (fl.ReadString(szLine, sizeof(szLine) / sizeof(WCHAR) - 1) != 0)
		{
			CString strLine(szLine);
			int nComment = strLine.Find(L"//");
			
			if (nComment == 0 || nComment == - 1) continue;
			
			CString strName = strLine.Left(nComment);
			strName.TrimRight();
			strName += L".jb1";

			if (Document->FindLayerTableLayer(strName) < 0)
			{
				Document->TracingMap(strName);
				Document->TracingFuse(strName);
				EoDbLayer* pLayer = Document->GetLayerTableLayer(strName);
				if (pLayer != 0)
				{
					strName = strLine.Mid(nComment + 2);
					strName.Trim();
					pLayer->SetName(strName);
				}
			}
		}
	}
}
