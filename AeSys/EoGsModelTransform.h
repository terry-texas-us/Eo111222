#pragma once

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

/** @class A class that manages a stack of 3D transformation matrices for model transformations.
 *  Used when rendering nested geometry (like a block reference containing other block references).
 *
 *  This class allows pushing and popping transformation matrices onto a stack, applying transformations 
 *  to points and vectors, and maintaining a composite transformation matrix that represents the 
 *  cumulative effect of all transformations in the stack.
 */
class EoGsModelTransform {
 public:
  EoGsModelTransform() : m_depth{0} {}

  EoGsModelTransform(const EoGsModelTransform&) = delete;
  EoGsModelTransform& operator=(const EoGsModelTransform&) = delete;

  ~EoGsModelTransform() = default;

  /** Returns the current composite transformation matrix.
   *  Useful for caching or external calculations.
   */
  [[nodiscard]] const EoGeTransformMatrix& GetCompositeMatrix() const noexcept { return m_compositeTransformMatrix; }

  /** Returns the inverse of the composite matrix.
   *  Essential for pick/selection: transforms screen coords back to model space.
   */
  [[nodiscard]] EoGeTransformMatrix GetInverseCompositeMatrix() const;

  [[nodiscard]] std::uint16_t Depth() const noexcept { return m_depth; }

  /** Transforms a point from world space to current model space (inverse transform).
   *  Used for hit-testing in nested blocks.
   */
  void InverseTransformPoint(EoGePoint3d& point) const;

  [[nodiscard]] bool IsActive() const noexcept { return m_depth > 0; }

  /** Returns true if the composite matrix is identity (no effective transform).
   */
  [[nodiscard]] bool IsIdentity() const noexcept { return m_depth == 0 || m_compositeTransformMatrix.IsIdentity(); }

  /** Removes the top transformation off the current transformation stack.
   */
  bool Pop();

  /** Places an identity transform on the top of the current transformation stack.
   */
  void Push();

  /** Clears all transforms and resets depth to zero.
   *  Safety method for error recovery or view reset.
   */
  void Reset() noexcept;

  /** The specified transformation is concatenated to the current model transformation (which is initially the identity transform).
   */
  void SetLocalTM(const EoGeTransformMatrix& transformation);

  void TransformPoint(EoGePoint3d& point) const noexcept;
  void TransformPoint(EoGePoint4d& point) const noexcept;

  void TransformPoints(EoGePoint4dArray& pointsArray) const noexcept;
  void TransformPoints(int numberOfPoints, EoGePoint4d* points) const noexcept;

  void TransformVector(EoGeVector3d& vector) const noexcept;

 private:
  std::uint16_t m_depth;                                // Transformation stack depth (0 means inactive)
  EoGeTransformMatrix m_compositeTransformMatrix;  // Current composite transformation matrix
  EoGeTransformMatrixList m_transformMatrixList;   // Stack of transformation matrices
};
