#pragma once

HTREEITEM tvAddItem(HWND tree, HTREEITEM parent, LPWSTR pszText, LPCVOID object);

class AeSysView;
class EoDbGroupList;
class EoDbGroup;

#if defined(USING_ODA)
class OdDbEntity;
class OdDbObjectId;
#endif  // USING_ODA

class EoDbPrimitive : public CObject {
 public:
  static const EoUInt16 BUFFER_SIZE = 2048;

  static const EoInt16 PENCOLOR_BYBLOCK = 0x0000;
  static const EoInt16 PENCOLOR_BYLAYER = 256;
  static const EoInt16 LINETYPE_BYBLOCK = 32766;
  static const EoInt16 LINETYPE_BYLAYER = 32767;

 protected:
  EoInt16 m_PenColor;
  EoInt16 m_LineType;

  static EoInt16 sm_LayerPenColor;
  static EoInt16 sm_LayerLineType;
  static EoInt16 sm_SpecialPenColorIndex;
  static EoInt16 sm_SpecialLineTypeIndex;
  static EoUInt16 sm_ControlPointIndex;
  static double sm_RelationshipOfPoint;
  static double sm_SelectApertureSize;

 public:  // Constructors and destructor
  EoDbPrimitive();
  EoDbPrimitive(EoInt16 penColor, EoInt16 lineType);
  virtual ~EoDbPrimitive();

 public:  // Methods - absolute virtuals
  virtual void AddToTreeViewControl(HWND, HTREEITEM) = 0;
  virtual void Assign(EoDbPrimitive* primitive) = 0;
#if defined(USING_ODA)
  virtual OdDbEntity* Convert(const OdDbObjectId& blockTableRecord) = 0;
#endif  // USING_ODA
  virtual EoDbPrimitive*& Copy(EoDbPrimitive*&) = 0;
  virtual void Display(AeSysView* view, CDC* deviceContext) = 0;
  virtual void AddReportToMessageList(EoGePoint3d) = 0;
  virtual void FormatExtra(CString& str) = 0;
  virtual void FormatGeometry(CString& str) = 0;
  virtual void GetAllPts(EoGePoint3dArray& pts) = 0;
  virtual EoGePoint3d GetCtrlPt() = 0;
  virtual void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) = 0;
  virtual EoGePoint3d GoToNxtCtrlPt() = 0;
  virtual bool Identical(EoDbPrimitive*) = 0;
  virtual bool Is(EoUInt16 type) = 0;
  virtual bool IsInView(AeSysView* view) = 0;
  virtual bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) = 0;
  virtual EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) = 0;
  virtual bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt) = 0;
  virtual bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) = 0;
  virtual bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) = 0;
  virtual void Transform(EoGeTransformMatrix&) = 0;
  virtual void Translate(EoGeVector3d translate) = 0;
  virtual void TranslateUsingMask(EoGeVector3d, const DWORD) = 0;
  virtual bool Write(CFile& file) = 0;
  virtual void Write(CFile& file, EoByte* buffer) = 0;

 public:  // Methods - virtuals
  virtual void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*);
  virtual void CutAtPt(EoGePoint3d&, EoDbGroup*);
  virtual int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*);
  virtual void ModifyState();
  virtual bool PvtOnCtrlPt(AeSysView*, const EoGePoint4d&);

 public:  // Methods
  CString FormatPenColor();
  CString FormatLineType();
  EoInt16 LogicalPenColor();
  EoInt16 LogicalLineType();
  EoInt16 PenColor() const;
  EoInt16 LineType() const;
  void PenColor(EoInt16 penColor);
  void LineType(EoInt16 lineType);

 public:  // Methods - static
  static EoUInt16 ControlPointIndex();
  static bool IsSupportedTyp(int iTyp);
  static EoInt16 LayerPenColorIndex();
  static void SetLayerPenColorIndex(EoInt16 colorIndex);
  static EoInt16 LayerLineTypeIndex();
  static void SetLayerLineTypeIndex(EoInt16 lineTypeIndex);
  static double& Rel();
  static EoInt16 SpecialLineTypeIndex();
  static EoInt16 SpecialPenColorIndex();
  static void SetSpecialPenColorIndex(EoInt16 colorIndex);
};
