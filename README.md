# AlcubierreEngine
The Alcubierre Engine is a modular game engine designed for perfomant 2D processing and rendering within the Vulkan framework.
The engine is modular in the sense that it makes reference to external components such as GLFW and Vulkan agnostically. Rewriting to use an alternative renderer or window system has been intentionally made as simple as possible.

Module structure notes:
All modules should register themselves with ModuleRegistry via a static function. This implementation avoids a hardcoded list of modules in either AlcubierreEngine or ModuleRegistry.
Modules should be structured thusly:
ModuleUniqueName/
    Wrapper.h # Holds static registry function, includes relevant service templates and maps pointers to public header methods
    PublicHeader.h # Declares public methods, properties and so on. Seperate from Wrapper because this must be included in Source and Wrapper brings the added weight of ModuleRegistry
    PrivateHeader.h # Technically optional if you don't mind managing declaration scope within Source, but I strongly recommend using this to declare all private objects relevant to the module before defining them in Source for readbility and simplicity.
    Source.cpp # Definition of all declarations and includes any needed libraries

The reason for this structure is to minimise header inclusion leakage as much as possible and to extensibly support the modular framework. If in future I choose to implement staged compilation (likely) then this folder structure should include the Makefile and log, and the parent directory should hold all compiled static libraries. Note this will necessitate some kind of checksum generation to recompile on changes and a loop in the top make.