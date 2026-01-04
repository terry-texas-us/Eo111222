#include "stdafx.h"

#include "EoDbBitmapFile.h"
#include <afx.h>
#include <afxstr.h>
#include <atltrace.h>
#include <memory>
#include <new>

EoDbBitmapFile::EoDbBitmapFile(const CString& fileName) {
  CFileException e;
  if (CFile::Open(fileName, modeRead | shareDenyNone, &e)) {}
}

/**
 * Loads a bitmap and its associated palette from a file.
 * @param fileName The path to the bitmap file.
 * @param[out] loadedBitmap A reference to a CBitmap object that will receive the loaded bitmap.
 * @param[out] loadedPalette A reference to a CPalette object that will receive the loaded palette.
 * @return true if the bitmap and palette were successfully loaded; false otherwise.
 */
bool EoDbBitmapFile::Load(const CString& fileName, CBitmap& loadedBitmap, CPalette& loadedPalette) {
  HBITMAP bitmap =
      static_cast<HBITMAP>(LoadImageW(0, fileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE));
  if (bitmap == nullptr) { return false; }
  loadedBitmap.Attach(bitmap);

  CClientDC clientContext(nullptr);
  if ((clientContext.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) == 0) {
    Close();
    return true;
  }
  DIBSECTION deviceIndependentBitmapStruct{};
  if (loadedBitmap.GetObject(sizeof(DIBSECTION), &deviceIndependentBitmapStruct) == 0) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to get DIBSECTION");
    Close();
    return false;
  }
  size_t colors{0};
  if (deviceIndependentBitmapStruct.dsBmih.biClrUsed != 0) {
    colors = static_cast<size_t>(deviceIndependentBitmapStruct.dsBmih.biClrUsed);
  } else {
    colors = static_cast<size_t>(1) << deviceIndependentBitmapStruct.dsBmih.biBitCount;
  }
  if (colors > 256) {
    if (!loadedPalette.CreateHalftonePalette(&clientContext)) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create halftone palette");
      Close();
      return false;
    }
  } else {
    std::unique_ptr<RGBQUAD[]> rgbQuad(new (std::nothrow) RGBQUAD[colors]);
    if (!rgbQuad) {
      Close();
      return false;
    }
    CDC memoryContext;
    if (!memoryContext.CreateCompatibleDC(&clientContext)) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create compatible DC");
      Close();
      return false;
    }
    CBitmap* customBitmap = memoryContext.SelectObject(&loadedBitmap);
    if (!customBitmap) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to select bitmap into DC");
      Close();
      return false;
    }
    if (GetDIBColorTable((HDC)memoryContext, 0U, static_cast<UINT>(colors), rgbQuad.get()) == 0) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to get DIB color table");
      memoryContext.SelectObject(customBitmap);
      Close();
      return false;
    }
    memoryContext.SelectObject(customBitmap);

    size_t paletteSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (colors - 1));
    std::unique_ptr<BYTE[]> paletteBuffer(new (std::nothrow) BYTE[paletteSize]);
    if (!paletteBuffer) {
      Close();
      return false;
    }
    LOGPALETTE* logicalPalette = reinterpret_cast<LOGPALETTE*>(paletteBuffer.get());
    logicalPalette->palVersion = 0x300;
    logicalPalette->palNumEntries = EoUInt16(colors);

    for (size_t i = 0; i < colors; i++) {
      logicalPalette->palPalEntry[i].peRed = rgbQuad[i].rgbRed;
      logicalPalette->palPalEntry[i].peGreen = rgbQuad[i].rgbGreen;
      logicalPalette->palPalEntry[i].peBlue = rgbQuad[i].rgbBlue;
      logicalPalette->palPalEntry[i].peFlags = 0;
    }
    if (!loadedPalette.CreatePalette(logicalPalette)) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Failed to create palette");
      Close();
      return false;
    }
  }
  Close();
  return true;
}
