#pragma once

class EoGeMatrixRow {
	double m_d[4];

public: // Constructors and destructor

public: // Operators

	EoGeMatrixRow& operator+=(const EoGeMatrixRow& v);
	EoGeMatrixRow& operator-=(const EoGeMatrixRow& row);
	EoGeMatrixRow& operator*=(const double d);
	EoGeMatrixRow& operator/=(const double d);

	//bool operator==(EoGeMatrixRow& row);
	//bool operator!=(EoGeMatrixRow& row);
	EoGeMatrixRow& operator+(EoGeMatrixRow& row);
	EoGeMatrixRow operator-(const EoGeMatrixRow& row);
	EoGeMatrixRow operator*(const double d) const;

	double& operator[](int i);
	const double& operator[](int i) const;
	void operator()(const double c0, const double c1, const double c2, const double c3);

	EoGeMatrixRow operator-();
	//bool Identical(const EoGeMatrixRow& row, double tolerance);

	static void Exchange(EoGeMatrixRow& rowA, EoGeMatrixRow& rowB);
};
class EoGeMatrix {
protected:
	union {
		double m_4X4[4][4];
		EoGeMatrixRow m_row[4];
	};

public: // Constructors and destructor
	EoGeMatrix() {
	}
	EoGeMatrix(const EoGeMatrixRow& row0, const EoGeMatrixRow& row1, const EoGeMatrixRow& row2, const EoGeMatrixRow& row3);

public: // Operators
	EoGeMatrix& operator*=(const EoGeMatrix& m);
	EoGeMatrix& operator*=(double scaleFactor);
	EoGeMatrix& operator/=(double scaleFactor);

	EoGeMatrix operator*(const EoGeMatrix& mB);

	//bool operator==(EoGeMatrix& mB);
	//bool operator!=(EoGeMatrix& mB);

	EoGeMatrixRow& operator[](int i);
	const EoGeMatrixRow& operator[](int i) const;

public: // Methods
	//bool Identical(EoGeMatrix& m);
	EoGeMatrix&	Identity();
	EoGeMatrix&	Inverse();
	EoGeMatrix Transpose();

	static EoGeMatrix Multiply(const EoGeMatrix& M1, const EoGeMatrix& M2);
};
