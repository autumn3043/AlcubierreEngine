# AlcubierreEngine
The Alcubierre Engine is a modular game engine designed for perfomant 2D processing and rendering within the Vulkan framework.
The engine is modular in the sense that it makes reference to external components such as GLFW and Vulkan agnostically. Rewriting to use an alternative renderer or window system has been intentionally made as simple as possible.

Module structure notes:
All modules should register themselves with Registry via a static function via the Dependency Injection design principle. This implementation avoids a hardcoded list of modules.
Modules should be structured thusly:
ModuleUniqueName/
    Public.h # Handles module registration and service declaration, maps public module functions to service functions.
    Private.h # Seperates weighty headers for speed and simplicity. Also seperates service declaration from service provision, so that a service which itself depends on a service does not cause a segfault.
    Source.cpp # Definition of all declarations and includes any needed libraries.

The reason for this structure is to minimise header inclusion leakage as much as possible and to extensibly support the modular framework. If in future I choose to implement staged compilation (likely) then this folder structure should include the Makefile and log, and the parent directory should hold all compiled static libraries. Note this will necessitate some kind of checksum generation to recompile on changes and a loop in the top make.