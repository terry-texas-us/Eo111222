#include "stdafx.h"

#include <io.h>
/// <summary>
/// the default file examine -- if the file isn't pathed to the right place,
/// then search the ACAD environment variable for the oldFile
/// </summary>
void Directory_ExamineFile(LPWSTR oldFile, const int oldFileBufferSize, LPTSTR newFile, const int newFileBufferSize)
{
	const int kExistenceOnly = 0;

	static WCHAR pathchar = '\\';
	static WCHAR oldpathchar = '/';

	// replace path characters, if present, with proper ones for platform
	for (int i = 0; i < int(wcslen(oldFile)); i++)
	{
		if (oldFile[i] == oldpathchar) oldFile[i] = pathchar;
	}
	if (wcschr(oldFile, pathchar) != 0) 
	{	// file has some degree of path designation
		if (_waccess_s(oldFile, kExistenceOnly) == 0) 
		{
			wcscpy_s(newFile, newFileBufferSize, oldFile);
			return;
		}
		// strip the path
		LPWSTR pLast = _tcsrchr(oldFile, pathchar);
		wcscpy_s(oldFile, oldFileBufferSize, ++pLast);
	}
	wcscpy_s(newFile, newFileBufferSize, oldFile);

	if (_waccess_s(oldFile, kExistenceOnly) != 0)
	{	// file is not in current directory
		if (wcschr(oldFile, ':') != 0) 
		{	// path with drive id
			return;
		}
		size_t RequiredSize;
		_wgetenv_s(&RequiredSize, NULL, 0, L"ACAD");

		if (RequiredSize == 0) 
		{	// no ACAD environment to search
			return;
		}
		LPWSTR envptr = new WCHAR[RequiredSize];

		_wgetenv_s(&RequiredSize, envptr, RequiredSize, L"ACAD");

		LPWSTR cptr = envptr;
		WCHAR testpath[256];
		WCHAR holdch;
		do 
		{
			while (*cptr != ';' && *cptr != 0) 
			{
				cptr++;
			}
			holdch = *cptr; // grab terminating character
			*cptr = 0;      // null it out for wcscpy_s
			wcscpy_s(testpath, 256, envptr);
			*cptr = holdch;
			int iTest = (int) wcslen(testpath);
			if (testpath[iTest - 1] != pathchar) 
			{  // append path character
				testpath[iTest] = pathchar;
				testpath[iTest + 1] = 0;
			}
			wcscat_s(testpath, 256, oldFile);

			if (_waccess_s(testpath, kExistenceOnly) == 0) 
			{
				wcscpy_s(newFile, newFileBufferSize, testpath);
				return;
			}
			cptr++;
			envptr = cptr;
		} 
		while (holdch == ';');        /* terminator is 0 for end of env string */

		delete [] envptr;
	}
}
