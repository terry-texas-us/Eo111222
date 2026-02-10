#include "Stdafx.h"

#include "AeSysDoc.h"
#include "EoDbGroup.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDlgTrapModify.h"
#include "EoGsRenderState.h"
#include "Hatch.h"
#include "Resource.h"

EoDlgTrapModify::EoDlgTrapModify(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgTrapModify::IDD, pParent) {}
EoDlgTrapModify::EoDlgTrapModify(AeSysDoc* document, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgTrapModify::IDD, pParent), m_Document(document) {}
EoDlgTrapModify::~EoDlgTrapModify() {}
void EoDlgTrapModify::DoDataExchange(CDataExchange* dataExchange) { CDialog::DoDataExchange(dataExchange); }

void EoDlgTrapModify::OnOK() {
  if (IsDlgButtonChecked(IDC_MOD_PEN)) { m_Document->ModifyTrappedGroupsColor(renderState.Color()); }
  if (IsDlgButtonChecked(IDC_MOD_LINE)) { m_Document->ModifyTrappedGroupsLineType(renderState.LineTypeIndex()); }
  if (IsDlgButtonChecked(IDC_MOD_FILL)) { ModifyPolygons(); }
  auto characterCellDefinition = renderState.CharacterCellDefinition();
  const auto& fontDefinition = renderState.FontDefinition();

  if (IsDlgButtonChecked(IDC_MOD_NOTE)) {
    m_Document->ModifyTrappedGroupsNoteAttributes(fontDefinition, characterCellDefinition, TM_TEXT_ALL);
  } else if (IsDlgButtonChecked(IDC_FONT)) {
    m_Document->ModifyTrappedGroupsNoteAttributes(fontDefinition, characterCellDefinition, TM_TEXT_FONT);
  } else if (IsDlgButtonChecked(IDC_HEIGHT)) {
    m_Document->ModifyTrappedGroupsNoteAttributes(fontDefinition, characterCellDefinition, TM_TEXT_HEIGHT);
  }
  CDialog::OnOK();
}

void EoDlgTrapModify::ModifyPolygons() {
  auto position = m_Document->GetFirstTrappedGroupPosition();
  while (position != nullptr) {
    auto* group = m_Document->GetNextTrappedGroup(position);

    auto PrimitivePosition = group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = group->GetNext(PrimitivePosition);

      if (primitive->Is(EoDb::kPolygonPrimitive)) {
        auto* polygon = static_cast<EoDbPolygon*>(primitive);
        polygon->SetPolygonStyle(renderState.PolygonIntStyle());
        polygon->SetFillStyleIndex(renderState.PolygonIntStyleId());
        polygon->SetHatRefVecs(hatch::dOffAng, hatch::dXAxRefVecScal, hatch::dYAxRefVecScal);
      }
    }
  }
}
