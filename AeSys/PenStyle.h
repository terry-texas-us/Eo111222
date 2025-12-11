#pragma once

class CLineType : public CObject
{
private:
	CString	m_strName;		// linetype name
	CString	m_strDesc; 		// descriptive text for linetype
	EoUInt16 m_wLens;		// number of dash length items
	double* m_dLen; 		// pointer to zero or more dash lengths

public:
	CLineType();
	CLineType(const CString& strName, const CString& strDesc, EoUInt16, double*);

	CLineType(const CLineType& lineType);
	
	~CLineType();
	
	CLineType& operator=(const CLineType& lineType);
	
	EoUInt16 GetNumberOfDashes() {return m_wLens;}
	void GetDashLen(double* dDash) {for (int i = 0; i < m_wLens; i++) dDash[i] = m_dLen[i];}
	CString GetDescription() {return m_strDesc;}
	CString GetName() {return m_strName;}
	double GetPatternLen();
};

typedef CTypedPtrArray<CObArray, CLineType*> CLineTypes;
