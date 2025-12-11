#pragma once

namespace dlgproceditops
{
	EoGeVector3d GetMirrorScale();
	EoGeVector3d GetInvertedScale();
	EoGeVector3d GetRotAng();
	CTMat GetInvertedRotTrnMat();
	void GetRotOrd(int* order);
	CTMat GetRotTrnMat();
	EoGeVector3d GetScaleFactors();
	void SetMirrorScale(double = - 1., double = 1., double = 1.);
	void SetScale(double = 1., double = 1., double = 1.);
	void SetRotAng(double = 0., double = 0., double = 45.);
	void SetRotOrd(int = 0, int = 1, int = 2);
}
