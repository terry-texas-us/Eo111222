#pragma once

class EoDbMaskedPrimitive : public CObject {
 private:
  EoDbPrimitive* m_Primitive;
  DWORD m_Mask;

 public:
  EoDbMaskedPrimitive() : m_Primitive(nullptr), m_Mask(0) {}
  EoDbMaskedPrimitive(EoDbPrimitive* primitive, DWORD mask) : m_Primitive(primitive), m_Mask(mask) {}
  EoDbMaskedPrimitive(const EoDbMaskedPrimitive&) = delete;
  EoDbMaskedPrimitive& operator=(const EoDbMaskedPrimitive&) = delete;

  void ClearMaskBit(int bit) { m_Mask &= ~(1UL << bit); }
  inline DWORD GetMask() const { return m_Mask; }
  EoDbPrimitive*& GetPrimitive() { return m_Primitive; }
  void SetMaskBit(int bit) { m_Mask |= (1UL << bit); }
};
