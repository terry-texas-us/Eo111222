#include "Stdafx.h"
#include "AeSysDoc.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbGroup.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDlgTrapModify.h"
#include "Hatch.h"
#include "PrimState.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoDlgTrapModify, CDialog)

EoDlgTrapModify::EoDlgTrapModify(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgTrapModify::IDD, pParent) {}
EoDlgTrapModify::EoDlgTrapModify(AeSysDoc* document, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgTrapModify::IDD, pParent), m_Document(document) {}
EoDlgTrapModify::~EoDlgTrapModify() {}
void EoDlgTrapModify::DoDataExchange(CDataExchange* dataExchange) { CDialog::DoDataExchange(dataExchange); }
void EoDlgTrapModify::OnOK() {
  if (IsDlgButtonChecked(IDC_MOD_PEN)) { m_Document->ModifyTrappedGroupsColor(pstate.PenColor()); }
  if (IsDlgButtonChecked(IDC_MOD_LINE)) { m_Document->ModifyTrappedGroupsLineType(pstate.LineType()); }
  if (IsDlgButtonChecked(IDC_MOD_FILL)) { ModifyPolygons(); }
  EoDbCharacterCellDefinition ccd;
  pstate.GetCharCellDef(ccd);

  EoDbFontDefinition fd;
  pstate.GetFontDef(fd);

  if (IsDlgButtonChecked(IDC_MOD_NOTE)) {
    m_Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_ALL);
  } else if (IsDlgButtonChecked(IDC_FONT)) {
    m_Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_FONT);
  } else if (IsDlgButtonChecked(IDC_HEIGHT)) {
    m_Document->ModifyTrappedGroupsNoteAttributes(fd, ccd, TM_TEXT_HEIGHT);
  }

  CDialog::OnOK();
}
void EoDlgTrapModify::ModifyPolygons() {
  auto position = m_Document->GetFirstTrappedGroupPosition();
  while (position != nullptr) {
    auto* Group = m_Document->GetNextTrappedGroup(position);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* Primitive = Group->GetNext(PrimitivePosition);

      if (Primitive->Is(EoDb::kPolygonPrimitive)) {
        EoDbPolygon* pPolygon = static_cast<EoDbPolygon*>(Primitive);
        pPolygon->SetIntStyle(pstate.PolygonIntStyle());
        pPolygon->SetIntStyleId(pstate.PolygonIntStyleId());
        pPolygon->SetHatRefVecs(hatch::dOffAng, hatch::dXAxRefVecScal, hatch::dYAxRefVecScal);
      }
    }
  }
}
