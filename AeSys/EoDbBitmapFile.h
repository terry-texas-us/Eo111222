#pragma once
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>

class EoDbBitmapFile : public CFile {
 public:
  EoDbBitmapFile() {}
  EoDbBitmapFile(const CString& strPathName);
  EoDbBitmapFile(const EoDbBitmapFile&) = delete;
  EoDbBitmapFile& operator=(const EoDbBitmapFile&) = delete;

  ~EoDbBitmapFile() {}
  bool Load(const CString& strPathName, CBitmap& bm, CPalette& pal);
};
