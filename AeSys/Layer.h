#pragma once

namespace EoDb
{
	enum LayerStateFlags
	{
		kIsResident = 0x0001,		// entry in table list is saved
		kIsInternal = 0x0002,		// group list saved within drawing
		kIsWork = 0x0004,			// may have groups added (0 or 1), displayed using hot color set
		kIsActive = 0x0008,			// may have groups modified (0 or more), displayed using warm color set
		kIsStatic = 0x0010,			// tracing which is viewed or layer which is static (no additions or modifications), displayed using warm color set
		kIsOff = 0x0020
	};
	enum TracingStateFlags
	{
		kTracingIsOpened = 0x0004,
		kTracingIsMapped = 0x0008,
		kTracingIsViewed = 0x0010,
		kTracingIsCloaked = 0x0020
	};
}
class CLayer : public EoDbGroupList
{
private:
	CString m_strName;// layer name. If layer is externally referenced this is the full specification of the file name.
	EoUInt16 m_wTracingFlgs;// Tracing flag values
	// b2 set - tracing is mapped
	EoUInt16 m_wStateFlgs;// layer state flag values
	// b0 set - layer is displayed using bitmap when possible
	// b1 set - layer is externally referenced. This means it is not necessary to write disk.
	// b2 set - layer is off
	EoInt16 m_PenColor; // color number, negative if layer is off)
	CString m_strLineTypeName;

public: // Constructors and destructor
	CLayer(const CString& name, EoUInt16 flags);

	~CLayer()
	{
	}
	void ClrStateFlg(EoUInt16 w = 0xffff) 
	{
		m_wStateFlgs &= ~w;
	}
	void ClrTracingFlg(EoUInt16 w = 0xffff) 
	{
		m_wTracingFlgs &= ~w;
	}
	void Display(AeSysView* view, CDC* deviceContext);
	void Display(AeSysView* view, CDC* deviceContext, bool bIdentifyTrap);
	EoUInt16 GetStateFlgs() 
	{
		return m_wStateFlgs;
	}
	CString GetLineTypeName() 
	{
		return m_strLineTypeName;
	}
	CString GetName() 
	{
		return m_strName;
	}
	EoUInt16 GetTracingFlgs() 
	{
		return m_wTracingFlgs;
	}
	bool IsStatic() 
	{
		return ((m_wStateFlgs & EoDb::kIsStatic) == EoDb::kIsStatic);
	}
	bool IsWork() 
	{
		return ((m_wStateFlgs & EoDb::kIsWork) == EoDb::kIsWork);
	}
	bool IsInternal() 
	{
		return ((m_wStateFlgs & EoDb::kIsInternal) == EoDb::kIsInternal);
	}
	bool IsMapped() 
	{
		return ((m_wTracingFlgs & EoDb::kTracingIsMapped) == EoDb::kTracingIsMapped);
	}
	bool IsOff() 
	{
		return ((m_wStateFlgs & EoDb::kIsOff) == EoDb::kIsOff);
	}
	bool IsOn() 
	{
		return !IsOff();
	}
	bool IsOpened() 
	{
		return ((m_wTracingFlgs & EoDb::kTracingIsOpened) == EoDb::kTracingIsOpened);
	}
	bool IsResident() 
	{
		return ((m_wStateFlgs & EoDb::kIsResident) == EoDb::kIsResident);
	}
	bool IsViewed() 
	{
		return ((m_wTracingFlgs & EoDb::kTracingIsViewed) == EoDb::kTracingIsViewed);
	}
	bool IsActive() 
	{
		return ((m_wStateFlgs & EoDb::kIsActive) == EoDb::kIsActive);
	}
	EoInt16 PenColor() 
	{
		return m_PenColor;
	}
	EoInt16 LineType();
	void PenTranslation(EoUInt16, EoInt16*, EoInt16*);
	void SetPenColor(EoInt16 penColor) 
	{
		m_PenColor = penColor;
	}
	void SetStateFlg(EoUInt16 w) 
	{
		m_wStateFlgs |= w;
	}
	void SetStateCold();
	void SetStateWork();
	void SetStateOff();
	void SetStateActive();
	void SetLineTypeName(const CString& strName) 
	{
		m_strLineTypeName = strName;
	}
	void SetName(const CString& strName) 
	{
		m_strName = strName;
	}
	void SetTracingFlg(EoUInt16 w) 
	{
		m_wTracingFlgs = w;
	}
};

typedef CTypedPtrArray<CObArray, CLayer*> CLayers;
