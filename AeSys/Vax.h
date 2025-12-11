#pragma once

class CVaxFloat
{
public:
	CVaxFloat() {m_f = 0.f;}

	void		Convert(const double&);
	double		Convert();
	
private:	
	float		m_f;
};

class CVaxPnt
{
public:
	CVaxPnt() 
	{
	}
	void		Convert(EoGePoint3d&);
	EoGePoint3d		Convert();

private:	
	CVaxFloat	x;
	CVaxFloat	y;
	CVaxFloat	z;
};

class CVaxVec
{
public:
	CVaxVec() 
	{
	}
	void		Convert(EoGeVector3d);
	EoGeVector3d		Convert();
	
private:	
	CVaxFloat	x;
	CVaxFloat	y;
	CVaxFloat	z;
};
