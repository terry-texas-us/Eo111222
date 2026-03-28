#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbAttrib.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDlgAttributePrompt.h"
#include "EoDlgBlockInsert.h"
#include "EoDxfAttributes.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"
#include "Resource.h"
#include "WndProcPreview.h"

/// @brief Creates an EoDbAttrib from an ATTDEF template with a user-supplied value.
///
/// Builds the font definition and reference system from the ATTDEF's text properties
/// (alignment, height, rotation, width, oblique angle, style) in block-definition space,
/// then applies the INSERT's transform matrix to position it in WCS.
///
/// @param attdef          The ATTDEF template from the block definition.
/// @param attributeValue  The user-entered attribute value string.
/// @param insertTransform The INSERT's combined transform (block space → WCS).
/// @return Newly allocated EoDbAttrib, or nullptr on degenerate input.
static EoDbAttrib* CreateAttribFromAttDef(const EoDxfAttDef& attdef, const std::wstring& attributeValue,
    const EoGeTransformMatrix& insertTransform) {
  if (attdef.m_textHeight < Eo::geometricTolerance || attributeValue.empty()) { return nullptr; }

  // --- Alignment mapping (same logic as ConvertAttribEntity) ---
  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(attdef.m_textStyleName);

  const auto horizontalAlignment = attdef.m_horizontalTextJustification;
  const auto verticalAlignment = attdef.m_verticalTextJustification;
  const bool isMiddleComposite = (horizontalAlignment == 4 && verticalAlignment == 0);

  switch (horizontalAlignment) {
    case 1:  // Center
    case 4:  // Middle (paired with Baseline)
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
      break;
    case 2:  // Right
    case 5:  // Fit
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Right);
      break;
    case 0:  // Left
    case 3:  // Aligned
    default:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Left);
      break;
  }

  if (isMiddleComposite) {
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
  } else {
    switch (verticalAlignment) {
      case 2:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
        break;
      case 3:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Top);
        break;
      case 0:
      case 1:
      default:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Bottom);
        break;
    }
  }

  // --- Reference system in block-definition space ---
  const bool isDefaultAlignment = (horizontalAlignment == 0 && verticalAlignment == 0);
  const bool isAlignedOrFit = (horizontalAlignment == 3 || horizontalAlignment == 5);

  auto firstPoint =
      EoGePoint3d{attdef.m_firstAlignmentPoint.x, attdef.m_firstAlignmentPoint.y, attdef.m_firstAlignmentPoint.z};
  auto secondPoint =
      EoGePoint3d{attdef.m_secondAlignmentPoint.x, attdef.m_secondAlignmentPoint.y, attdef.m_secondAlignmentPoint.z};

  EoGePoint3d referenceOrigin;
  if (isDefaultAlignment || isAlignedOrFit) {
    referenceOrigin = firstPoint;
  } else if (attdef.HasSecondAlignmentPoint()) {
    referenceOrigin = secondPoint;
  } else {
    referenceOrigin = firstPoint;
  }

  auto baselineDirection = EoGeVector3d::positiveUnitX;
  const auto textRotation = Eo::DegreeToRadian(attdef.m_textRotation);
  if (attdef.HasSecondAlignmentPoint() && isAlignedOrFit) {
    auto alignedDirection = secondPoint - firstPoint;
    if (!alignedDirection.IsNearNull()) {
      baselineDirection = alignedDirection;
      baselineDirection.Unitize();
    }
  } else if (Eo::IsGeometricallyNonZero(textRotation)) {
    baselineDirection.RotateAboutArbitraryAxis(EoGeVector3d::positiveUnitZ, textRotation);
  }

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(EoGeVector3d::positiveUnitZ, xAxisDirection);

  const auto obliqueAngle = Eo::DegreeToRadian(attdef.m_obliqueAngle);
  if (Eo::IsGeometricallyNonZero(obliqueAngle)) {
    yAxisDirection.RotateAboutArbitraryAxis(EoGeVector3d::positiveUnitZ, -obliqueAngle);
  }

  yAxisDirection *= attdef.m_textHeight;
  xAxisDirection *= attdef.m_relativeXScaleFactor * attdef.m_textHeight * Eo::defaultCharacterCellAspectRatio;

  EoGeReferenceSystem referenceSystem(referenceOrigin, xAxisDirection, yAxisDirection);

  auto* attrib = new EoDbAttrib(fontDefinition, referenceSystem, attributeValue, attdef.m_tagString, attdef.m_attributeFlags);
  attrib->SetTextGenerationFlags(attdef.m_textGenerationFlags);

  // Transform from block-definition space to WCS via the INSERT's transform matrix
  attrib->Transform(insertTransform);

  return attrib;
}

BEGIN_MESSAGE_MAP(EoDlgBlockInsert, CDialog)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgBlockInsert::OnLbnSelchangeBlocksList)
ON_BN_CLICKED(IDC_PURGE, &EoDlgBlockInsert::OnBnClickedPurge)
END_MESSAGE_MAP()

EoGePoint3d EoDlgBlockInsert::InsertionPoint;

EoDlgBlockInsert::EoDlgBlockInsert(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgBlockInsert::IDD, pParent) {}
EoDlgBlockInsert::EoDlgBlockInsert(AeSysDoc* document, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgBlockInsert::IDD, pParent), m_Document(document) {}
EoDlgBlockInsert::~EoDlgBlockInsert() {}
void EoDlgBlockInsert::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_BlocksListBoxControl);
}
BOOL EoDlgBlockInsert::OnInitDialog() {
  CDialog::OnInitDialog();

  InsertionPoint = app.GetCursorPosition();

  CString BlockName;
  EoDbBlock* Block{};

  auto BlockPosition = m_Document->GetFirstBlockPosition();
  while (BlockPosition != nullptr) {
    m_Document->GetNextBlock(BlockPosition, BlockName, Block);
    if (!Block->IsAnonymous()) { m_BlocksListBoxControl.AddString(BlockName); }
  }
  m_BlocksListBoxControl.SetCurSel(0);

  if (m_Document->BlockTableIsEmpty()) {
    WndProcPreviewClear(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd());
  } else {
    BlockPosition = m_Document->GetFirstBlockPosition();
    m_Document->GetNextBlock(BlockPosition, BlockName, Block);
    SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(Block->GetCount()), FALSE);
    SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
    WndProcPreviewUpdateBlock(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), Block);
  }
  return TRUE;
}
void EoDlgBlockInsert::OnOK() {
  int CurrentSelection = m_BlocksListBoxControl.GetCurSel();

  if (CurrentSelection != LB_ERR) {
    CString BlockName;
    m_BlocksListBoxControl.GetText(CurrentSelection, BlockName);

    auto* blockReference = new EoDbBlockReference(BlockName, InsertionPoint);
    auto* group = new EoDbGroup(blockReference);

    // Check if block has attribute definitions — prompt user for values
    EoDbBlock* block{};
    if (m_Document->LookupBlock(BlockName, block) && !block->AttributeDefinitions().empty()) {
      const auto basePoint = block->BasePoint();
      const auto transformMatrix = blockReference->BuildTransformMatrix(basePoint);

      for (const auto& attdef : block->AttributeDefinitions()) {
        // Skip constant attributes (flag 2) — they are block-level text, not per-insert
        if (attdef.m_attributeFlags & 2) { continue; }
        // Skip invisible attributes (flag 1) — not rendered
        if (attdef.m_attributeFlags & 1) { continue; }

        std::wstring attributeValue;

        if (attdef.m_attributeFlags & 8) {
          // Preset: use default value without prompting
          attributeValue = attdef.m_defaultValue;
        } else {
          EoDlgAttributePrompt promptDlg(this);
          promptDlg.m_blockName = BlockName;
          promptDlg.m_tagName = attdef.m_tagString.c_str();
          promptDlg.m_promptString = attdef.m_promptString.c_str();
          promptDlg.m_defaultValue = attdef.m_defaultValue.c_str();

          if (promptDlg.DoModal() != IDOK) { break; }
          attributeValue = promptDlg.m_enteredValue.GetString();
        }

        if (attributeValue.empty()) { continue; }

        auto* attrib = CreateAttribFromAttDef(attdef, attributeValue, transformMatrix);
        if (attrib == nullptr) { continue; }

        // Link ATTRIB ↔ INSERT via handles
        attrib->SetInsertHandle(blockReference->Handle());
        attrib->SetOwnerHandle(blockReference->Handle());
        blockReference->AddAttributeHandle(attrib->Handle());
        group->AddTail(attrib);
      }
    }

    m_Document->AddWorkLayerGroup(group);
    m_Document->UpdateAllViews(nullptr, EoDb::kGroup, group);
  }
  CDialog::OnOK();
}
void EoDlgBlockInsert::OnLbnSelchangeBlocksList() {
  int CurrentSelection = m_BlocksListBoxControl.GetCurSel();

  if (CurrentSelection != LB_ERR) {
    CString BlockName;
    m_BlocksListBoxControl.GetText(CurrentSelection, BlockName);

    EoDbBlock* Block{};
    m_Document->LookupBlock(BlockName, Block);
    SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(Block->GetCount()), FALSE);
    SetDlgItemInt(IDC_REFERENCES, static_cast<UINT>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
    WndProcPreviewUpdateBlock(GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd(), Block);
  }
}
void EoDlgBlockInsert::OnBnClickedPurge() {
  m_Document->RemoveUnusedBlocks();

  CDialog::OnOK();
}
