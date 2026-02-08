#pragma once

class EoGeVector3d;

class EoDbCharacterCellDefinition {
  double m_height{0.1};           // height of character cell
  double m_expansionFactor{1.0};  // expansion factor applied to character cell width
  double m_rotationAngle{};       // rotation applied to the character cell
  double m_slantAngle{};          // rotation applied to the vertical component of the character cell

 public:
  EoDbCharacterCellDefinition() = default;
  EoDbCharacterCellDefinition(double rotationAngle, double slantAngle, double expansionFactor, double height);

  EoDbCharacterCellDefinition(const EoDbCharacterCellDefinition&) = default;
  EoDbCharacterCellDefinition(EoDbCharacterCellDefinition&&) noexcept = default;

  EoDbCharacterCellDefinition& operator=(const EoDbCharacterCellDefinition&) = default;
  EoDbCharacterCellDefinition& operator=(EoDbCharacterCellDefinition&&) noexcept = default;

  [[nodiscard]] double ExpansionFactor() const noexcept { return m_expansionFactor; }
  void SetExpansionFactor(double expansionFactor) noexcept { m_expansionFactor = expansionFactor; }
  [[nodiscard]] double Height() const noexcept { return m_height; }
  void SetHeight(double height) noexcept { m_height = height; }
  [[nodiscard]] double SlantAngle() const noexcept { return m_slantAngle; }
  void SetSlantAngle(double slantAngle) noexcept { m_slantAngle = slantAngle; }
  [[nodiscard]] double RotationAngle() const noexcept { return m_rotationAngle; }
  void SetRotationAngle(double rotationAngle) noexcept { m_rotationAngle = rotationAngle; }
};
