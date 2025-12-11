#include "stdafx.h"

#include "Vax.h"

// Vax: the excess 128 exponent .. range is -128 (0x00 - 0x80) to 127 (0xff - 0x80)
// MS: the excess 127 exponent .. range is -127 (0x00 - 0x7f) to 128 (0xff - 0x7f)

double CVaxFloat::Convert()
{
	float fMS = 0.f;

	BYTE* pvax = (BYTE*) &m_f;
	BYTE* pms = (BYTE*) &fMS;
	
	BYTE bSign = BYTE(pvax[1] & 0x80);
	BYTE bExp = BYTE((pvax[1] << 1) & 0xff);
	bExp |= pvax[0] >> 7;
	
	if (bExp == 0)
	{
		if (bSign != 0)
		{	// floating-reserved operand (error condition)
			throw L"CVaxFloat: Conversion to MS - Reserve operand fault";
		}
	}	
	else if (bExp == 1)
	{	// this is a valid vax exponent but because the vax places the hidden 
		// leading 1 to the right of the binary point we have a problem .. 
		// the possible values are 2.94e-39 to 5.88e-39 .. just call it 0.
	}
	else
	{	// - 128 + 127 - 1 (to get hidden 1 to the left of the binary point)
		bExp -= 2; 
	
		pms[3] = BYTE(bExp >> 1);
		pms[3] |= bSign;

		pms[2] = BYTE((bExp << 7) & 0xff);
		pms[2] |= pvax[0] & 0x7f;
	
		pms[1] = pvax[3];
		pms[0] = pvax[2];
	}
	return (double(fMS));
}
void CVaxFloat::Convert(const double& dMS)
{
	float fMS = float(dMS);
	float fVax = 0.f;

	if (fMS != 0.f)
	{
		BYTE* pMS = (BYTE*) &fMS;
		BYTE* pVax = (BYTE*) &fVax;

		BYTE bSign = BYTE(pMS[3] & 0x80);
		BYTE bExp = BYTE((pMS[3] << 1) & 0xff);
		bExp |= pMS[2] >> 7;

		if (bExp > 0xfd)
			bExp = 0xfd;

		// - 127 + 128 + 1 (to get hidden 1 to the right of the binary point)
		bExp += 2;	
		
		pVax[1] = BYTE(bExp >> 1);
		pVax[1] |= bSign;

		pVax[0] = BYTE((bExp << 7) & 0xff);
		pVax[0] |= pMS[2] & 0x7f;

		pVax[3] = pMS[1];
		pVax[2] = pMS[0];
	}
	m_f = fVax;
}

void CVaxPnt::Convert(EoGePoint3d& pt)
{
	x.Convert(pt.x);
	y.Convert(pt.y);
	z.Convert(pt.z);
}
EoGePoint3d CVaxPnt::Convert()
{
	return EoGePoint3d(x.Convert(), y.Convert(), z.Convert());
}

void CVaxVec::Convert(EoGeVector3d v)
{
	x.Convert(v.x);
	y.Convert(v.y);
	z.Convert(v.z);
}
EoGeVector3d CVaxVec::Convert()
{
	return EoGeVector3d(x.Convert(), y.Convert(), z.Convert());
}
