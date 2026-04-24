#pragma once

class EoDbBitmapFile : public CFile {
 public:
  EoDbBitmapFile() {}
  EoDbBitmapFile(const CString& fileName);
  EoDbBitmapFile(const EoDbBitmapFile&) = delete;
  EoDbBitmapFile& operator=(const EoDbBitmapFile&) = delete;

  ~EoDbBitmapFile() {}
  bool Load(const CString& fileName, CBitmap& loadedBitmap, CPalette& loadedPalette);
};
