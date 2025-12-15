#include "stdafx.h"
#include "AeSysDoc.h"

#include "EoDbCharacterCellDefinition.h"
#include "EoDbGroup.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDlgTrapModify.h"
#include "Hatch.h"
#include "PrimState.h"

// EoDlgTrapModify dialog

IMPLEMENT_DYNAMIC(EoDlgTrapModify, CDialog)

EoDlgTrapModify::EoDlgTrapModify(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgTrapModify::IDD, pParent) {}
EoDlgTrapModify::EoDlgTrapModify(AeSysDoc* document, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgTrapModify::IDD, pParent), m_Document(document) {}
EoDlgTrapModify::~EoDlgTrapModify() {}
void EoDlgTrapModify::DoDataExchange(CDataExchange* dataExchange) { CDialog::DoDataExchange(dataExchange); }
void EoDlgTrapModify::OnOK() {
  if (IsDlgButtonChecked(IDC_MOD_PEN)) { m_Document->ModifyTrappedGroupsPenColor(pstate.PenColor()); }
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
  POSITION Position = m_Document->GetFirstTrappedGroupPosition();
  while (Position != 0) {
    EoDbGroup* Group = m_Document->GetNextTrappedGroup(Position);

    POSITION PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

      if (Primitive->Is(EoDb::kPolygonPrimitive)) {
        EoDbPolygon* pPolygon = static_cast<EoDbPolygon*>(Primitive);
        pPolygon->SetIntStyle(pstate.PolygonIntStyle());
        pPolygon->SetIntStyleId(pstate.PolygonIntStyleId());
        pPolygon->SetHatRefVecs(hatch::dOffAng, hatch::dXAxRefVecScal, hatch::dYAxRefVecScal);
      }
    }
  }
}
