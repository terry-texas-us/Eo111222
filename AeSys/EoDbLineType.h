#pragma once

#include <afx.h>
#include <afxstr.h>
#include <algorithm>
#include <vector>

class EoDbLineType : public CObject {
 public:

  EoDbLineType();
  EoDbLineType(EoUInt16 index, const CString& name, const CString& comment, EoUInt16 numberOfDashLengths,
               double* dashLengths);

  EoDbLineType(const EoDbLineType& other);
  EoDbLineType& operator=(const EoDbLineType& other);
 
  /** @brief Retrieves the index of the line type.
   *
   * @return The index of the line type as an EoUInt16.
   */
  EoUInt16 Index() const { return m_Index; }
  
  /** @brief Retrieves the number of dash elements in the line type.
   *
   * @return The number of dash elements as an EoUInt16.
   */
  EoUInt16 GetNumberOfDashes() const { return static_cast<EoUInt16>(m_DashElements.size()); }
  
  /** @brief Copies the dash lengths into the provided array.
   *
   * @param dDash A pointer to an array where the dash lengths will be copied.
   */
  void GetDashLen(double* dDash) const { std::copy(m_DashElements.begin(), m_DashElements.end(), dDash); }
  
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
  EoUInt16 m_Index;
  CString m_Name;
  CString m_Description;
  std::vector<double> m_DashElements;
};
