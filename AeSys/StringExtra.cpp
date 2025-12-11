#include "stdafx.h"

LPTSTR string_TrimLeadingSpace(LPTSTR szString)
{
	LPTSTR p = szString;
	
	while (p && *p && isspace(*p))
	{
		p++;
	}
	return p;
}		
