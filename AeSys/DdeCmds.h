#pragma once

#if defined(USING_DDE)
#include "dde.h"

// prototypes for handling procedures

namespace dde {
	bool ExecFileGet(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecGotoPoint(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecLine(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecNote(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecPen(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecSend(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecSetPoint(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecDA(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecDL(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecScale(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecFill(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecNoteHT(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecTracingBlank(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecTracingMap(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecTracingOpen(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
	bool ExecTracingView(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *);
}
#endif // USING_DDE