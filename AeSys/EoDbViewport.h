#pragma once

class EoDbViewport : public EoDbPrimitive
{
private:
	EoDbAbstractView m_AbstractView;

public:

	EoDbViewport(void)
	{
	}

	virtual ~EoDbViewport(void)
	{
	}

void EoDbViewport::Display(AeSysView* view, CDC* deviceContext);
};
