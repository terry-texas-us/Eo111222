#pragma once

class EoDbLayer : public EoDbGroupList {
 private:
  CString m_Name;  // layer name. If layer is externally referenced this is the full specification of the file name.
  EoUInt16 m_StateFlags;   // layer state flag values
                           // b0 set - layer is displayed using bitmap when possible
                           // b1 set - layer is externally referenced. This means it is not necessary to write disk.
                           // b2 set - layer is off
  EoUInt16 m_TracingFlgs;  // Tracing flag values
                           // b2 set - tracing is mapped
  EoInt16 m_ColorIndex;    // color index, negative if layer is off)
  EoDbLineType* m_LineType;

 public:  // Constructors and destructor
  enum LayerStateFlags {
    kIsResident = 0x0001,  // entry in table list is saved
    kIsInternal = 0x0002,  // group list saved within drawing
    kIsWork = 0x0004,      // may have groups added (0 or 1), displayed using hot color set
    kIsActive = 0x0008,    // may have groups modified (0 or more), displayed using warm color set
    kIsStatic =
        0x0010,  // tracing which is viewed or layer which is static (no additions or modifications), displayed using warm color set
    kIsOff = 0x0020
  };
  enum TracingStateFlags {
    kTracingIsOpened = 0x0004,
    kTracingIsMapped = 0x0008,
    kTracingIsViewed = 0x0010,
    kTracingIsCloaked = 0x0020
  };
  EoDbLayer(const CString& name, EoUInt16 flags);
  EoDbLayer(const CString& name, EoUInt16 flags, EoDbLineType* lineType);
  EoDbLayer(const EoDbLayer& layer) = delete;
  EoDbLayer& operator=(const EoDbLayer&) = delete;

  ~EoDbLayer() override {}
  void Display(AeSysView* view, CDC* deviceContext);
  void Display(AeSysView* view, CDC* deviceContext, bool identifyTrap);

  COLORREF Color() const;
  EoInt16 ColorIndex() const { return m_ColorIndex; }
  void SetColorIndex(EoInt16 colorIndex) { m_ColorIndex = colorIndex; }

  EoDbLineType* LineType() const;
  EoInt16 LineTypeIndex();
  CString LineTypeName() { return m_LineType->Name(); }
  void SetLineType(EoDbLineType* lineType);
  void PenTranslation(EoUInt16, EoInt16*, EoInt16*);
  CString Name() const { return m_Name; }
  void SetName(const CString& name) { m_Name = name; }

  void ClrTracingFlg(EoUInt16 w = 0xffff) { m_TracingFlgs &= ~w; }
  bool IsOpened() { return ((m_TracingFlgs & kTracingIsOpened) == kTracingIsOpened); }
  bool IsMapped() { return ((m_TracingFlgs & kTracingIsMapped) == kTracingIsMapped); }
  bool IsViewed() { return ((m_TracingFlgs & kTracingIsViewed) == kTracingIsViewed); }
  EoUInt16 GetTracingFlgs() { return m_TracingFlgs; }
  void SetTracingFlg(EoUInt16 w) { m_TracingFlgs = w; }

  void ClearStateFlag(EoUInt16 w = 0xffff) { m_StateFlags &= ~w; }
  EoUInt16 LayerStateFlags() const { return m_StateFlags; }
  bool IsActive() { return ((m_StateFlags & kIsActive) == kIsActive); }
  bool IsInternal() { return ((m_StateFlags & kIsInternal) == kIsInternal); }
  bool IsOff() { return ((m_StateFlags & kIsOff) == kIsOff); }
  bool IsResident() { return ((m_StateFlags & kIsResident) == kIsResident); }
  bool IsStatic() { return ((m_StateFlags & kIsStatic) == kIsStatic); }
  bool IsWork() { return ((m_StateFlags & kIsWork) == kIsWork); }
  void MakeInternal() { m_StateFlags |= kIsInternal; }
  void MakeResident() { m_StateFlags |= kIsResident; }
  void SetStateWork() {
    m_StateFlags &= ~(kIsActive | kIsStatic | kIsOff);
    m_StateFlags |= kIsWork;
  }
  void MakeStateActive() {
    m_StateFlags &= ~(kIsWork | kIsStatic | kIsOff);
    m_StateFlags |= kIsActive;
  }
  void SetStateStatic() {
    m_StateFlags &= ~(kIsWork | kIsActive | kIsOff);
    m_StateFlags |= kIsStatic;
  }
  void SetStateOff() {
    m_StateFlags &= ~(kIsWork | kIsActive | kIsStatic);
    m_StateFlags |= kIsOff;
  }
};

typedef CTypedPtrArray<CObArray, EoDbLayer*> CLayers;
