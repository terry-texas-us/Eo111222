You are working on a native MFC/C++ CAD/graphics application called AeSys (main project folder: Application/AeSys). 
We strictly follow the MFC document/view pattern (AeSys, AeSysDoc, AeSysView classes). 
All geometric primitives derive from EoDbPrimitive and implement virtuals for drawing, selection, transform, and serialization. 
Preserve the virtual contract and ABI at all times.

The codebase lives in D:\Projects\Eo111222 and is under local Git + GitHub at https://github.com/terry-texas-us/Eo111222.
Reference the public repo when needed, but always work on the local files.

Build system: 100% Visual Studio XML projects (.vcxproj, .sln, .vcxproj.filters, .props). 
Describe every file addition, include path change, library link, or project property modification exclusively in Visual Studio terms: 
“Add file via Solution Explorer → right-click project → Add → Existing Item”, 
“Edit project properties (Alt+F7) → C/C++ → General → Additional Include Directories”, etc. 
NEVER mention CMake, CMakeLists.txt, or any non-Visual-Studio build system.

We are modernizing the .peg file format to make linear DXF parsing easier. 
Use handle architecture (at minimum for header and table sections, including entities/.peg primitives). 
Only hard resource handles are from entities to header/tables. 
Extension dictionaries are optional but welcome for future-proofing.

When suggesting changes, always maintain compatibility with existing on-disk PEG file formats and MFC idioms.