#pragma once

class EoDbLineType : public CObject {
  EoUInt16 m_Index;
  CString m_Name;
  CString m_Description;
  EoUInt16 m_NumberOfDashElements;
  double* m_DashElements;

 public:
  EoDbLineType();
  EoDbLineType(EoUInt16 index, const CString& name, const CString& comment, EoUInt16 numberOfDashLengths,
               double* dashLengths);

  EoDbLineType(const EoDbLineType& lineType);

  ~EoDbLineType();

  EoDbLineType& operator=(const EoDbLineType& lineType);

  EoUInt16 Index() const { return m_Index; }
  EoUInt16 GetNumberOfDashes() { return m_NumberOfDashElements; }
  void GetDashLen(double* dDash) {
    for (int Index = 0; Index < m_NumberOfDashElements; Index++) { dDash[Index] = m_DashElements[Index]; }
  }
  CString Description() const { return m_Description; }
  CString Name() const { return m_Name; }
  double GetPatternLen();
};
