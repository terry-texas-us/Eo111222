#pragma once

class EoDbCharacterCellDefinition;

class EoGeReferenceSystem {
	EoGePoint3d	m_Origin;
	EoGeVector3d m_XDirection;
	EoGeVector3d m_YDirection;

public: // Constructors and destructor
	EoGeReferenceSystem();
	EoGeReferenceSystem(const EoGePoint3d& origin, EoDbCharacterCellDefinition& ccd);
	EoGeReferenceSystem(const EoGePoint3d& origin, EoGeVector3d xDirection, EoGeVector3d yDirection);

	EoGeReferenceSystem(const EoGeReferenceSystem& cs);

	~EoGeReferenceSystem() {
	}
public: // Operators
	EoGeReferenceSystem& operator=(const EoGeReferenceSystem&);

public: // Methods
	void GetUnitNormal(EoGeVector3d& normal);
	EoGePoint3d Origin() const;
	void Read(CFile& file);
	/// <summary>Takes the current reference directions and rescales using passed character cell state.</summary>
	void Rescale(EoDbCharacterCellDefinition& ccd);
	void Set(const EoGePoint3d& origin, const EoGeVector3d xDirection, const EoGeVector3d yDirection);
	void SetOrigin(const EoGePoint3d& origin);
	void SetXDirection(const EoGeVector3d& xDirection);
	void SetYDirection(const EoGeVector3d& yDirection);
	void Transform(EoGeTransformMatrix& tm);
	EoGeTransformMatrix TransformMatrix() const;
	void Write(CFile& file);
	EoGeVector3d XDirection() const;
	EoGeVector3d YDirection() const;
};
