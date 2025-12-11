#pragma once

#include "PixelMapRGB.h"

class CFileBitmap : public CFile
{

private:
	int		m_nRows;		// dimensions of the bitmap
	int		m_nCols;
	CRGB*	m_rgbPixel;

public:
	CFileBitmap() 
	{
	}
	CFileBitmap(const CString& strPathName);

	~CFileBitmap() 
	{
	}
	bool Load(const CString& strPathName, CBitmap& bm, CPalette& pal);
};
