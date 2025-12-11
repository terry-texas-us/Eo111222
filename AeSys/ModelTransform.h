#pragma once

class CModelTransform
{

private:

	EoUInt16	m_wDepth;
	CTMat*	m_p_tmCompositeModeling;

	CObList m_TMList;	

public:
	
	CModelTransform();
	
	~CModelTransform();

	void	InvokeNew();
	void	Return();
	void	Transform(EoGePoint3d& pt);
	void	Transform(CPnt4& pt);
	void	Transform(EoGeVector3d& v);
	void	SetLocalTM(CTMat& tm);

};

extern CModelTransform mspace;
