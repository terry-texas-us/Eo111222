#pragma once

#if defined(USING_ODA)
#include "RxObject.h"

class EoDbConvertEntityToPrimitive : public OdRxObject {
public:
  ODRX_DECLARE_MEMBERS(EoDbConvertEntityToPrimitive);

  virtual void Convert(OdDbEntity* entity, EoDbGroup* group);
};
class Converters;

/// <summary> This class is the protocol extension class for all entity Converters</summary>
class ProtocolExtension_ConvertEntityToPegPrimitive {
	Converters* m_Converters;
public:
	static AeSysDoc* m_Document;

	ProtocolExtension_ConvertEntityToPegPrimitive(AeSysDoc* document);
	virtual ~ProtocolExtension_ConvertEntityToPegPrimitive();
	void Initialize();
	void Uninitialize();
};
#endif // USING_ODA
