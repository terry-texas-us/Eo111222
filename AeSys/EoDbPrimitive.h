#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <string>
#include <utility>

#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"
#include "drw_entities.h"

HTREEITEM tvAddItem(HWND tree, HTREEITEM parent, LPWSTR pszText, LPCVOID object);

class AeSysDoc;
class AeSysView;
class EoDbGroupList;
class EoDbGroup;
class EoGeTransformMatrix;

class EoDbPrimitive : public CObject {
 public:
  static constexpr EoUInt16 BUFFER_SIZE{2048};

  static constexpr EoInt16 PENCOLOR_BYBLOCK{0};
  static constexpr EoInt16 PENCOLOR_BYLAYER{256};
  static constexpr EoInt16 LINETYPE_BYBLOCK{32766};
  static constexpr EoInt16 LINETYPE_BYLAYER{32767};

 protected:
  EoInt16 m_PenColor{PENCOLOR_BYLAYER};
  EoInt16 m_LineType{LINETYPE_BYLAYER};
  std::wstring m_linetypeName{};
  std::wstring m_layerName{};

  static EoInt16 sm_LayerPenColor;
  static EoInt16 sm_LayerLineType;
  static EoInt16 sm_SpecialPenColorIndex;
  static EoUInt16 sm_ControlPointIndex;
  static double sm_RelationshipOfPoint;
  static double sm_SelectApertureSize;

 public:  // Constructors and destructor
  EoDbPrimitive();
  EoDbPrimitive(EoInt16 penColor, EoInt16 lineType);
  EoDbPrimitive(const EoDbPrimitive&) = delete;
  EoDbPrimitive& operator=(const EoDbPrimitive&) = delete;

  ~EoDbPrimitive() override;

 public:  // Methods - absolute virtuals
  virtual void AddToTreeViewControl(HWND, HTREEITEM) = 0;
  virtual void Assign(EoDbPrimitive* primitive) = 0;
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
  void SetBaseProperties(const DRW_Entity* entity, AeSysDoc* document);

  CString FormatPenColor() const;
  CString FormatLineType() const;
  EoInt16 LogicalPenColor() const;
  EoInt16 LogicalLineType() const;

  EoInt16 PenColor() const noexcept { return m_PenColor; }
  void SetPenColor(EoInt16 penColor) { m_PenColor = penColor; }
  EoInt16 LineTypeIndex() const noexcept { return m_LineType; }
  void SetLineTypeIndex(EoInt16 lineType) { m_LineType = lineType; }

  const std::wstring& LinetypeName() const noexcept { return m_linetypeName; }
  void SetLinetypeName(std::wstring name) { m_linetypeName = std::move(name); }
  
  const std::wstring& LayerName() const noexcept { return m_layerName; }
  void SetLayerName(std::wstring name) { m_layerName = std::move(name); }

 public:  // Methods - static
  static EoUInt16 ControlPointIndex();
  static bool IsSupportedTyp(int iTyp);
  static EoInt16 LayerPenColorIndex();
  static void SetLayerPenColorIndex(EoInt16 colorIndex);
  static EoInt16 LayerLineTypeIndex();
  static void SetLayerLineTypeIndex(EoInt16 lineTypeIndex);
  static double& Rel();
  static EoInt16 SpecialPenColorIndex();
  static void SetSpecialPenColorIndex(EoInt16 colorIndex);
};
