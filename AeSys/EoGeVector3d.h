#pragma once

class EoGePoint3d;
class OdGeVector3d;
class OdGeScale3d;

class EoGeVector3d {
 public:
  double x;
  double y;
  double z;

 public:  // constructors and destructor
  EoGeVector3d();
  EoGeVector3d(double initialX, double initialY, double initialZ);
  EoGeVector3d(const EoGePoint3d& ptP, const EoGePoint3d& ptQ);
#if defined(USING_ODA)
  EoGeVector3d(const OdGeVector3d& initialVector);
  EoGeVector3d(const OdGeScale3d& initialScale);
#endif  // USING_ODA

 public:  // operators
  bool operator==(const EoGeVector3d& vector) const;
  bool operator!=(const EoGeVector3d& vector) const;
  void operator+=(const EoGeVector3d& vector);
  void operator-=(const EoGeVector3d& vector);
  void operator*=(double t);
  void operator/=(double t);

  void operator()(double xNew, double yNew, double zNew);
  EoGeVector3d operator-() const;
  EoGeVector3d operator-(const EoGeVector3d& vector);
  EoGeVector3d operator+(const EoGeVector3d& vector);

  EoGeVector3d operator*(double t) const;

 public:  // Methods
  bool IsNearNull();
  double Length() const;
  void Normalize();
  void RotAboutArbAx(const EoGeVector3d& axis, double angle);
  /** @brief Returns the square of the length of the vector.
   * @return The squared length.
   */
  double SquaredLength() const;
  CString ToString();
  void Read(CFile& file);
  void Write(CFile& file);

 public:
  static const EoGeVector3d kXAxis;
  static const EoGeVector3d kYAxis;
  static const EoGeVector3d kZAxis;
};
/// <summary>Compute a not so arbitrary axis for AutoCAD entities</summary>
EoGeVector3d ComputeArbitraryAxis(const EoGeVector3d& normal);
EoGeVector3d RotateVectorAboutZAxis(const EoGeVector3d& vector, double angle);
EoGeVector3d EoGeCrossProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2);
/// <summary>Computes the scalar product (= inner product) of the two vectors</summary>
/// <remarks>
///In the Euclidean space there is a strong relationship between the dot product and lengths and angles.
///For a vector a, a•a is the square of its length, and, more generally, if b is another vector
/// a•b = |a||b|cos(θ) where
///   |a| and |b| denote the length (magnitude) of a and b
///   θ is the angle between them.
///Since |a|cos(θ) is the scalar projection of a onto b, the dot product can be understood geometrically as the product of this projection with the length of b.
///As the cosine of 90° is zero, the dot product of two perpendicular vectors is always zero.
///If a and b have length one (they are unit vectors), the dot product simply gives the cosine of the angle between them.
///Thus, given two vectors, the angle between them can be found by rearranging the above formula:
///  θ = arccos(a•b/|a||b|)
/// </remarks>
double EoGeDotProduct(const EoGeVector3d& vector1, const EoGeVector3d& vector2);
bool EoGeNearEqual(const EoGeVector3d& vector1, const EoGeVector3d& vector2, double tolerance);
