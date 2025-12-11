#include "stdafx.h"

CModelTransform::CModelTransform()
{
	m_wDepth = 0;
	m_p_tmCompositeModeling = new CTMat;
}
CModelTransform::~CModelTransform()
{
	delete m_p_tmCompositeModeling;
}
void CModelTransform::InvokeNew()
{
	m_wDepth++;
	m_TMList.AddTail(m_p_tmCompositeModeling);
	m_p_tmCompositeModeling = new CTMat(*m_p_tmCompositeModeling);
}
void CModelTransform::Return()
{
	delete m_p_tmCompositeModeling;
	
	m_p_tmCompositeModeling = (CTMat*) m_TMList.RemoveTail();
	
	m_wDepth--;
}
void CModelTransform::SetLocalTM(CTMat& tm)
{
	*m_p_tmCompositeModeling = (Matrix) tm * *m_p_tmCompositeModeling;
}
void CModelTransform::Transform(EoGePoint3d& pt) 
{
	if (m_wDepth == 0) return;
	pt = *m_p_tmCompositeModeling * pt;
}
void CModelTransform::Transform(CPnt4& pt) 
{
	if (m_wDepth == 0) return;
	pt = *m_p_tmCompositeModeling * pt;
}
void CModelTransform::Transform(EoGeVector3d& v) 
{
	if (m_wDepth == 0) return;
	v = *m_p_tmCompositeModeling * v;
}
