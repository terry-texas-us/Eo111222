#pragma once

#include <algorithm>
#include <vector>

class EoDbLineType : public CObject {
 public:
  EoDbLineType();
  EoDbLineType(std::int16_t index, const CString& name, const CString& comment, EoUInt16 numberOfDashLengths, double* dashLengths);

  EoDbLineType(const EoDbLineType& other);
  EoDbLineType& operator=(const EoDbLineType& other);

  /** @brief Retrieves the index of the line type.
   *
   * @return The index of the line type as an std::int16_t.
   */
  std::int16_t Index() const { return m_Index; }

  CString IndexToString() const {
    CString indexAsString;
    indexAsString.Format(L"%03u", m_Index);
    return indexAsString;
  }

  /** @brief Retrieves the number of dash elements in the line type.
   *
   * @return The number of dash elements as an EoUInt16.
   */
  EoUInt16 GetNumberOfDashes() const { return static_cast<EoUInt16>(m_DashElements.size()); }

  /** @brief Copies the dash elements of the line type into the provided array.
   *
   * @param dashElements A pointer to an array where the dash elements will be copied.
   */
  void GetDashElements(double* dashElements) const { std::copy(m_DashElements.begin(), m_DashElements.end(), dashElements); }

  /** @brief Retrieves the dash elements of the line type.
   *
   * @return A constant reference to a vector containing the dash elements.
   */
  const std::vector<double>& DashElements() const { return m_DashElements; }

  /** @brief Retrieves the description of the line type.
   *
   * @return The description of the line type as a CString.
   */
  CString Description() const { return m_Description; }

  /** @brief Retrieves the name of the line type.
   *
   * @return The name of the line type as a CString.
   */
  CString Name() const { return m_Name; }

  /**
   * @brief Calculates the total length of the line type pattern.
   *
   * This method sums the absolute values of all dash elements in the line type
   * to determine the total length of the pattern.
   *
   * @return The total length of the line type pattern as a double.
   */
  double GetPatternLength() const;

 private:
  std::int16_t m_Index;
  CString m_Name;
  CString m_Description;
  std::vector<double> m_DashElements;
};
