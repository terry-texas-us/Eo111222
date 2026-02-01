#include "Stdafx.h"

#include <cmath>

#include "Eo.h"
#include "EoGeLine.h"
#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

/** @brief Constructs transformation matrix required to transform points about a reference point and axis.
 * @param referencePoint reference point
 * @param referenceAxis reference axis vector (unit)
 * @param angle angle (radians)
 * @note Assumes reference vector is a unit vector. Uses right handed convention. Based on the following equation:
 *  [ti] * [r] * [t], where [ti] translation of reference point to origin
 *  and [r] rotation about origin matrix defined as:
 *  | ax*ax+(1-ax*ax)*ca | ax*ay*(1-ca)+az*sa | ax*az*(1-ca)-ay*sa |
 *  | ax*ay*(1-ca)-az*sa | ay*ay+(1-ay*ay)*ca | ay*az*(1-ca)+ax*sa |
 *  | ax*az*(1-ca)+ay*sa | ay*az*(1-ca)-ax*sa | az*az+(1-az*az)*ca | and
 *  [t] translation of reference point back to initial position
 */
EoGeTransformMatrix::EoGeTransformMatrix(const EoGePoint3d& referencePoint, EoGeVector3d referenceAxis, const double angle) {
  double SinAng = sin(angle);
  double CosAng = cos(angle);

  double xSquared = referenceAxis.x * referenceAxis.x;
  double ySquared = referenceAxis.y * referenceAxis.y;
  double zSquared = referenceAxis.z * referenceAxis.z;

  m_4X4[0][0] = (xSquared + (1.0 - xSquared) * CosAng);
  m_4X4[0][1] = (referenceAxis.x * referenceAxis.y * (1.0 - CosAng) - referenceAxis.z * SinAng);
  m_4X4[0][2] = (referenceAxis.x * referenceAxis.z * (1.0 - CosAng) + referenceAxis.y * SinAng);
  m_4X4[0][3] = -m_4X4[0][0] * referencePoint.x - m_4X4[0][1] * referencePoint.y - m_4X4[0][2] * referencePoint.z + referencePoint.x;

  m_4X4[1][0] = (referenceAxis.x * referenceAxis.y * (1.0 - CosAng) + referenceAxis.z * SinAng);
  m_4X4[1][1] = (ySquared + (1.0 - ySquared) * CosAng);
  m_4X4[1][2] = (referenceAxis.y * referenceAxis.z * (1.0 - CosAng) - referenceAxis.x * SinAng);
  m_4X4[1][3] = -m_4X4[1][0] * referencePoint.x - m_4X4[1][1] * referencePoint.y - m_4X4[1][2] * referencePoint.z + referencePoint.y;

  m_4X4[2][0] = (referenceAxis.x * referenceAxis.z * (1.0 - CosAng) - referenceAxis.y * SinAng);
  m_4X4[2][1] = (referenceAxis.y * referenceAxis.z * (1.0 - CosAng) + referenceAxis.x * SinAng);
  m_4X4[2][2] = (zSquared + (1.0 - zSquared) * CosAng);
  m_4X4[2][3] = -m_4X4[2][0] * referencePoint.x - m_4X4[2][1] * referencePoint.y - m_4X4[2][2] * referencePoint.z + referencePoint.z;

  m_4X4[3][0] = 0.0;
  m_4X4[3][1] = 0.0;
  m_4X4[3][2] = 0.0;
  m_4X4[3][3] = 1.0;
}

/** @brief Constructs a transformation matrix to align an arbitrary coordinate system defined by a point and two reference vectors.
 * 
 * This constructor initializes a transformation matrix that aligns an arbitrary coordinate system defined by a reference point and two reference vectors (x and y axes).
 * The z-axis is derived from the cross product of the x and y reference vectors. The resulting transformation matrix maps the specified coordinate system to the standard Cartesian coordinate system.
 * 
 * @param referencePoint The origin point of the reference coordinate system.
 * @param xAxis The reference vector defining the x-axis of the coordinate system.
 * @param yAxis The reference vector defining the y-axis of the coordinate system.
 * 
 * @note The x and y reference axis vectors do not need to be normalized; appropriate scaling is applied as needed.
 */
EoGeTransformMatrix::EoGeTransformMatrix(EoGePoint3d referencePoint, EoGeVector3d xAxis, EoGeVector3d yAxis) {
  
  auto normal = CrossProduct(xAxis, yAxis);
  normal.Normalize();

  ConstructUsingReferencePointAndNormal(referencePoint, normal);

  // Transform x-axis reference vector onto z=0 plane
  EoGeVector3d xAxisTransformed = xAxis;
  xAxisTransformed = *this * xAxisTransformed;

  EoGeVector3d scaleVector(1.0 / sqrt(xAxisTransformed.x * xAxisTransformed.x + xAxisTransformed.y * xAxisTransformed.y), 1.0, 1.0);

  xAxisTransformed.Normalize();

  // To get x-axis reference vector as x-axis
  EoGeTransformMatrix transformMatrix;
  *this *= transformMatrix.ZAxisRotation(-xAxisTransformed.y, xAxisTransformed.x);

  // Transform y-axis reference vector onto z=0 plane
  EoGeVector3d yAxisTransformed = yAxis;
  yAxisTransformed = *this * yAxisTransformed;

  if (fabs(yAxisTransformed.y) < Eo::geometricTolerance) { return; }

  scaleVector.y = 1.0 / yAxisTransformed.y;
  scaleVector.z = 1.0;

  // Add shear to matrix which gets positive y-axis reference vector as y-axis
  if (fabs(yAxisTransformed.x) > Eo::geometricTolerance) {
    double shearFactor = -yAxisTransformed.x / yAxisTransformed.y;
    for (int i = 0; i < 4; i++) { m_4X4[0][i] += m_4X4[1][i] * shearFactor; }
  }
  Scale(scaleVector);
}

EoGeTransformMatrix::EoGeTransformMatrix(EoGePoint3d referencePoint, EoGeVector3d normal) { 
  ConstructUsingReferencePointAndNormal(referencePoint, normal); 
}

/** @brief Builds rotation transformation matrices.
 * @param rotationAngles Angles (in degrees) for each axis
 * @note Rotations are applied in X, Y, Z order. Angles (in degrees) for each axis
 * @return The resulting rotation transformation matrix.
 */
EoGeTransformMatrix EoGeTransformMatrix::BuildRotationTransformMatrix(const EoGeVector3d& rotationAngles) const {
  EoGeTransformMatrix matrix;

  matrix.Identity();
  matrix.AppendXAxisRotation(rotationAngles.x);
  matrix.AppendYAxisRotation(rotationAngles.y);
  matrix.AppendZAxisRotation(rotationAngles.z);

  return matrix;
}

void EoGeTransformMatrix::AppendXAxisRotation(double xAxisAngle) {
  if (fabs(xAxisAngle) > Eo::geometricTolerance) {
    double angleInRadians = Eo::DegreeToRadian(xAxisAngle);
    EoGeTransformMatrix matrix;
    *this *= matrix.XAxisRotation(sin(angleInRadians), cos(angleInRadians));
  }
}

void EoGeTransformMatrix::AppendYAxisRotation(double yAxisAngle) {
  if (fabs(yAxisAngle) > Eo::geometricTolerance) {
    double angleInRadians = Eo::DegreeToRadian(yAxisAngle);
    EoGeTransformMatrix matrix;
    *this *= matrix.YAxisRotation(sin(angleInRadians), cos(angleInRadians));
  }
}

void EoGeTransformMatrix::AppendZAxisRotation(double zAxisAngle) {
  if (fabs(zAxisAngle) > Eo::geometricTolerance) {
    double angleInRadians = Eo::DegreeToRadian(zAxisAngle);
    EoGeTransformMatrix matrix;
    *this *= matrix.ZAxisRotation(sin(angleInRadians), cos(angleInRadians));
  }
}

/** @brief Constructs transformation matrix required to transform points about a reference point and normal.
 *  @param referencePoint reference point on plane which defines origin
 *  @param normal unit vector defining plane normal
 *  @note Assumes plane normal is a unit vector. Uses right handed convention. See Rodgers, 3-9 Rotation about an arbitrary axis in space.
*/
void EoGeTransformMatrix::ConstructUsingReferencePointAndNormal(EoGePoint3d referencePoint, EoGeVector3d normal) {
  Identity();
  Translate(EoGeVector3d(referencePoint, EoGePoint3d::kOrigin));

  double yNormalAbs = fabs(normal.y);
  double zNormalAbs = fabs(normal.z);

  double d = 0.0;
  if (zNormalAbs < Eo::geometricTolerance) {
    d = yNormalAbs;
  } else if (yNormalAbs < Eo::geometricTolerance) {
    d = zNormalAbs;
  } else {
    d = sqrt(yNormalAbs * yNormalAbs + zNormalAbs * zNormalAbs);
  }

  EoGeTransformMatrix transformMatrix;
  if (d > Eo::geometricTolerance) {
    transformMatrix.XAxisRotation(normal.y / d, normal.z / d);
    *this *= transformMatrix;
  }
  if (fabs(normal.x) > Eo::geometricTolerance) {
    transformMatrix.YAxisRotation(-normal.x, d);
    *this *= transformMatrix;
  }
}

/** @brief Initializes a matrix for rotation about the x-axis, the y-axis rotates to the z-axis
 *  @param sinAngle sine of rotation angle
 *  @param cosAngle cosine of rotation angle
 */
EoGeTransformMatrix EoGeTransformMatrix::XAxisRotation(const double sinAngle, const double cosAngle) {
  m_4X4[0][0] = 1.0;
  m_4X4[0][1] = 0.0;
  m_4X4[0][2] = 0.0;
  m_4X4[0][3] = 0.0;

  m_4X4[1][0] = 0.0;
  m_4X4[1][1] = cosAngle;
  m_4X4[1][2] = -sinAngle;
  m_4X4[1][3] = 0.0;

  m_4X4[2][0] = 0.0;
  m_4X4[2][1] = sinAngle;
  m_4X4[2][2] = cosAngle;
  m_4X4[2][3] = 0.0;

  m_4X4[3][0] = 0.0;
  m_4X4[3][1] = 0.0;
  m_4X4[3][2] = 0.0;
  m_4X4[3][3] = 1.0;

  return (*this);
}

/** @brief Initializes a matrix for rotation about the y-axis, the z-axis rotates to the x-axis
 *  @param sinAngle sine of rotation angle
 *  @param cosAngle cosine of rotation angle
 */
EoGeTransformMatrix EoGeTransformMatrix::YAxisRotation(const double sinAngle, const double cosAngle) {
  m_4X4[0][0] = cosAngle;
  m_4X4[0][1] = 0.0;
  m_4X4[0][2] = sinAngle;
  m_4X4[0][3] = 0.0;

  m_4X4[1][0] = 0.0;
  m_4X4[1][1] = 1.0;
  m_4X4[1][2] = 0.0;
  m_4X4[1][3] = 0.0;

  m_4X4[2][0] = -sinAngle;
  m_4X4[2][1] = 0.0;
  m_4X4[2][2] = cosAngle;
  m_4X4[2][3] = 0.0;

  m_4X4[3][0] = 0.0;
  m_4X4[3][1] = 0.0;
  m_4X4[3][2] = 0.0;
  m_4X4[3][3] = 1.0;

  return (*this);
}

/** @brief Initializes a matrix for rotation about the z-axis, the x-axis rotates to the y-axis
 *  @param sinAngle sine of rotation angle
 *  @param cosAngle cosine of rotation angle
 */
EoGeTransformMatrix EoGeTransformMatrix::ZAxisRotation(const double sinAngle, const double cosAngle) {
  m_4X4[0][0] = cosAngle;
  m_4X4[0][1] = -sinAngle;
  m_4X4[0][2] = 0.0;
  m_4X4[0][3] = 0.0;

  m_4X4[1][0] = sinAngle;
  m_4X4[1][1] = cosAngle;
  m_4X4[1][2] = 0.0;
  m_4X4[1][3] = 0.0;

  m_4X4[2][0] = 0.0;
  m_4X4[2][1] = 0.0;
  m_4X4[2][2] = 1.0;
  m_4X4[2][3] = 0.0;

  m_4X4[3][0] = 0.0;
  m_4X4[3][1] = 0.0;
  m_4X4[3][2] = 0.0;
  m_4X4[3][3] = 1.0;

  return (*this);
}

void EoGeTransformMatrix::Scale(EoGeVector3d scaleVector) {
  for (int i = 0; i < 4; i++) {
    m_4X4[0][i] *= scaleVector.x;
    m_4X4[1][i] *= scaleVector.y;
    m_4X4[2][i] *= scaleVector.z;
  }
}

EoGeLine EoGeTransformMatrix::operator*(const EoGeLine& line) {
  EoGeLine transformedLine;

  transformedLine.begin = *this * line.begin;
  transformedLine.end = *this * line.end;

  return transformedLine;
}

EoGePoint3d EoGeTransformMatrix::operator*(const EoGePoint3d& point) {
  EoGePoint3d transformedPoint;

  transformedPoint.x = point.x * m_4X4[0][0] + point.y * m_4X4[0][1] + point.z * m_4X4[0][2] + m_4X4[0][3];
  transformedPoint.y = point.x * m_4X4[1][0] + point.y * m_4X4[1][1] + point.z * m_4X4[1][2] + m_4X4[1][3];
  transformedPoint.z = point.x * m_4X4[2][0] + point.y * m_4X4[2][1] + point.z * m_4X4[2][2] + m_4X4[2][3];

  return transformedPoint;
}

EoGePoint4d EoGeTransformMatrix::operator*(const EoGePoint4d& point) {
  EoGePoint4d transformedPoint;

  transformedPoint.x = point.x * m_4X4[0][0] + point.y * m_4X4[0][1] + point.z * m_4X4[0][2] + point.w * m_4X4[0][3];
  transformedPoint.y = point.x * m_4X4[1][0] + point.y * m_4X4[1][1] + point.z * m_4X4[1][2] + point.w * m_4X4[1][3];
  transformedPoint.z = point.x * m_4X4[2][0] + point.y * m_4X4[2][1] + point.z * m_4X4[2][2] + point.w * m_4X4[2][3];
  transformedPoint.w = point.x * m_4X4[3][0] + point.y * m_4X4[3][1] + point.z * m_4X4[3][2] + point.w * m_4X4[3][3];

  return transformedPoint;
}

EoGeVector3d EoGeTransformMatrix::operator*(const EoGeVector3d& vector) {
  EoGeVector3d transformedVector;

  transformedVector.x = vector.x * m_4X4[0][0] + vector.y * m_4X4[0][1] + vector.z * m_4X4[0][2];
  transformedVector.y = vector.x * m_4X4[1][0] + vector.y * m_4X4[1][1] + vector.z * m_4X4[1][2];
  transformedVector.z = vector.x * m_4X4[2][0] + vector.y * m_4X4[2][1] + vector.z * m_4X4[2][2];

  return transformedVector;
}
