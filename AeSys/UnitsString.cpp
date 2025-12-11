#include "stdafx.h"

#include "Lex.h"


double UnitsString_ParseLength(LPTSTR aszLen)
{
	LPTSTR	szEndPtr;
	
	double dRetVal = _tcstod(aszLen, &szEndPtr);
	
	switch (toupper((int) szEndPtr[0]))
	{
		case '\'':												// Feet and maybe inches
			dRetVal *= 12.; 										// Reduce to inches
			dRetVal += _tcstod(&szEndPtr[1], &szEndPtr); 			// Begin scan for inches at character following foot delimeter
			break;
	
		case 'M': 
			if (toupper((int) szEndPtr[1]) == 'M')
				dRetVal *= .03937007874015748;
			else
				dRetVal *= 39.37007874015748;
			break;
	
		case 'C':
			dRetVal *= .3937007874015748;
			break;
			 
		case 'D':
			dRetVal *= 3.937007874015748;
			break;
		
		case 'K':
			dRetVal *= 39370.07874015748;
	
	}
	return (dRetVal / app.GetScale());
}

double UnitsString_ParseLength(EoDb::Units units, LPTSTR aszLen)
{	// Convert length expression to double value.
	try
	{
		int iTokId = 0;
		long lDef;
		int iTyp;
		double dVal[32];
		
		lex::Parse(aszLen);
		lex::EvalTokenStream(&iTokId, &lDef, &iTyp, (void*) dVal);
		
		if (iTyp == lex::TOK_LENGTH_OPERAND)
			return (dVal[0]);
		else
		{
			lex::ConvertValTyp(iTyp, lex::TOK_REAL, &lDef, dVal);
			
			switch (units)
			{
				case EoDb::kArchitectural:
				case EoDb::kEngineering:
				case EoDb::kFeet:
					dVal[0] *= 12.;
					break;

				case EoDb::kMeters:
					dVal[0] *= 39.37007874015748;
					break;

				case EoDb::kMillimeters:
					dVal[0] *= .03937007874015748;
					break;

				case EoDb::kCentimeters:
					dVal[0] *= .3937007874015748;
					break;
				
				case EoDb::kDecimeters:
					dVal[0] *= 3.937007874015748;
					break;

				case EoDb::kKilometers:
					dVal[0] *= 39370.07874015748;
			}				
			dVal[0] /= app.GetScale();
		}
		return (dVal[0]);
	}
	catch(LPTSTR szMessage)
	{
		::MessageBox(0, szMessage, 0, MB_ICONWARNING | MB_OK);
		return (0.);
	}
}

