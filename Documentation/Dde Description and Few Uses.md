
# MainFrm.cpp

  #include "Dde.h"						
  
  dde::Uninitialize();					

# PegAEsys.cpp

  #include "Dde.h"						
  
  dde::Setup(AeSys::GetInstance());				
  
  dde::PostAdvise(dde::ExtNumInfo);		
  dde::PostAdvise(dde::ExtStrInfo);		

# PegAEsysView.cpp

  #include "DdeGItms.h"						
  
  dde::PostAdvise(dde::ScaleInfo);		
  dde::PostAdvise(dde::DimLenInfo);		
  dde::PostAdvise(dde::DimAngZInfo);	
  dde::PostAdvise(dde::RelPosXInfo);	
  dde::PostAdvise(dde::RelPosYInfo);	
  dde::PostAdvise(dde::RelPosZInfo);	

# PrimDim.cpp

  #include "DdeGItms.h"	
  
  dde::PostAdvise(dde::EngLenInfo);		
  dde::PostAdvise(dde::EngAngZInfo);	
  
# PrimLine.cpp  

  #include "DdeGItms.h"	
  
  dde::PostAdvise(dde::EngLenInfo);		
  dde::PostAdvise(dde::EngAngZInfo);	
   
# PrimPolygon.cpp

  #include "DdeGItms.h"	
  
  dde::PostAdvise(dde::EngLenInfo);		
  dde::PostAdvise(dde::EngAngZInfo);	

# PrimPolyline.cpp

  #include "DdeGItms.h"						

  dde::PostAdvise(dde::EngLenInfo);		
  dde::PostAdvise(dde::EngAngZInfo);
  
  # Dde.cpp

  #include "PegAEsys.h"
  #include "Lex.h"
  #include "Messages.h"

  # DdeCmds.cpp

  #include "PegAEsys.h"
  #include "PegAEsysDoc.h"

  #include "CharCellDef.h"
  #include "FileJob.h"
  #include "FontDef.h"
  #include "Layer.h"
  #include "Pnt.h"
  #include "Prim.h"
  #include "PrimState.h"
  #include "PrimText.h"
  #include "RefSys.h"
  #include "Seg.h"
  #include "SegsDet.h"
  #include "SegsTrap.h"
  #include "Text.h"
  #include "UnitsString.h"
  #include "Vec.h"

  # DdeGItms.cpp

  #include "PegAEsys.h"
  #include "PegAEsysView.h"