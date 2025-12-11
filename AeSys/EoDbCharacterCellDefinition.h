#pragma once

class EoDbCharacterCellDefinition {
private:
	double		m_dChrHgt;		// height of character cell
	double		m_dChrExpFac;	// expansion factor applied to character cell width
	double		m_dTextRotAng;	// rotation applied to the character cell
	double		m_dChrSlantAng;	// rotation applied to the vertical component of the character cell

public:

	EoDbCharacterCellDefinition();
	EoDbCharacterCellDefinition(double dTextRotAng, double dChrSlantAng, double dChrExpFac, double dChrHgt);

	EoDbCharacterCellDefinition(const EoDbCharacterCellDefinition& ccd);

	EoDbCharacterCellDefinition& operator=(const EoDbCharacterCellDefinition& ccd);

	double		ChrExpFacGet() {return (m_dChrExpFac);}
	void		ChrExpFacSet(double d) {m_dChrExpFac = d;}
	double		ChrHgtGet() {return (m_dChrHgt);}
	void		ChrHgtSet(double d) {m_dChrHgt = d;}
	double		ChrSlantAngGet() {return (m_dChrSlantAng);}
	void		ChrSlantAngSet(double d) {m_dChrSlantAng = d;}
	double		TextRotAngGet() {return (m_dTextRotAng);}
	void		TextRotAngSet(double d) {m_dTextRotAng = d;}
};
/// <summary>Produces the reference system vectors for a single charater cell.</summary>
void CharCellDef_EncdRefSys(const EoGeVector3d& normal, EoDbCharacterCellDefinition& ccd, EoGeVector3d& xAxisReference, EoGeVector3d& yAxisReference);
