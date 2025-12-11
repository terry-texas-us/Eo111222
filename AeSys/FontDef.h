#pragma once

namespace EoDb
{
	enum Path
	{
		kPathRight,
		kPathLeft,
		kPathUp,
		kPathDown
	};
	enum HorizontalAlignment
	{
		kAlignLeft = 1,
		kAlignCenter,
		kAlignRight
	};
	enum VerticalAlignment
	{
		kAlignTop = 2,
		kAlignMiddle,
		kAlignBottom
	};
	enum Precision
	{
		kEoTrueType = 1,
		kStrokeType
	};
}

class CFontDef
{
private:
	double m_CharacterSpacing;
	EoUInt16 m_Precision;
	CString m_FontName;
	EoUInt16 m_Path;
	EoUInt16 m_HorizontalAlignment;
	EoUInt16 m_VerticalAlignment;

public:
	CFontDef();
	CFontDef(EoUInt16 precision, const CString& fontName, EoUInt16 path, EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment, double spacing);
	CFontDef(const CFontDef& fd);

	CFontDef& operator=(const CFontDef& fd);

	void Set(EoUInt16 precision, const CString& fontName, EoUInt16 path, EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment, double spacing);

	CString FormatHorizonatlAlignment();
	CString FormatPath();
	CString FormatPrecision();
	CString FormatVerticalAlignment();

public: // get and set methods
	double CharacterSpacing() 
	{
		return (m_CharacterSpacing);
	}
	void CharacterSpacing(double spacing)
	{
		m_CharacterSpacing = spacing;
	}
	EoUInt16 HorizontalAlignment() 
	{
		return (m_HorizontalAlignment);
	}
	void HorizontalAlignment(EoUInt16 horizontalAlignment)
	{
		m_HorizontalAlignment = horizontalAlignment;
	}
	CString FontName() 
	{
		return (m_FontName);
	}
	void FontName(const CString& fontName) 
	{
		m_FontName = fontName;
	}
	EoUInt16 Precision() 
	{
		return (m_Precision);
	}
	void Precision(EoUInt16 precision) 
	{
		m_Precision = precision;
	}
	EoUInt16 Path() 
	{
		return (m_Path);
	}
	void Path(EoUInt16 path) 
	{
		m_Path = path;
	}
	EoUInt16 VerticalAlignment() 
	{
		return (m_VerticalAlignment);
	}
	void VerticalAlignment(EoUInt16 verticalAlignment)
	{
		m_VerticalAlignment = verticalAlignment;
	}

	void Read(CFile& file);
	void Write(CFile& file);
};
