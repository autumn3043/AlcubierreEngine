# AlcubierreEngine
The Alcubierre Engine is a 3D graphics rendering API intended to be a performant and reliable backend for modern rendering applications development.

Rendering APIs available to modern developers are generally old and time-tested, providing a degree of reliability, but are also outdated and therefore less ideal than they could otherwise be. Projects such as OpenGL are products of fundamental design decisions which are not necessarily incorrect, but have in some regards aged poorly or no longer represent most optimal solutions given the utilities available on modern graphics hardware. This project aims to provide a reliable and transparent graphics rendering framework which allows developers to access new capabilities available on modern hardware in order to benefit their applications and workflow.

## Key Features
- Transparent and reliable API which allows developers fine-tuned control according to their discretion in order to make the most effective use of available resources for specialised applications where standard implementations may be a limiting factor
- Utilises parallelism on both the Host and Device in user environments where such capabilities are available
- Reacts to resource-availability in the user environment in order to take reasonable advantage of available hardware
- Supports most modern 3D graphics formats, including a utility to reduce filesize by converting meshes, materials and textures into GPU-ready formats
- Targets single-executable output, with no requirement for additional files other than built program and the option to include assets directly in the binary if the developer prefers
    - (Alternate distribution methods of course supported)
- Compatible with any end-user environment which is also compatible with Vulkan
- Provides robust additional utilities, such as window management, input handling, audio management, configuration/settings parsing and safe runtime-modification, automated shader descriptor generation and configurable debugging/logging

## Dependencies
### Build Requirements
- CMake
- g++ (>= C++23) (Recommended)
- Vulkan (>= 3.0): Hardware abstraction layer
- GLFW: Window and input management
- Nlohmann Json: Configuration database parsing and management
- SPIRV-Reflect: Shader descriptor generation

### Optional Tool Requirements
- **Model Precompiler**:
    - xxHash: Asset hashing for mesh-independent material/texture tracking
    - CLI11: Command line argument parsing utility
    - tinyobjloader: Obj file parsing
    - KTX-Software: Texture file parsing
 
### End User Requirements
- Vulkan (>=3.0)
- GLFW

## Development
This API is a solo project to which I have been the only contributor on-and-off for several months, according to my own availability. What is present is functional, but the API as a whole is not complete or in a usable state, and awaits both feature completion and optimisation passes. Furthermore, some older components of the project deserve full refactors. It is published as a supplementary article to my resume, in order to demonstrate my capacity for software architecture, optimisation and robust/reliable/transparent design. This note will be removed when I consider it to be suitable for use.

## License
MIT License

Copyright (c) 2026 Autumn Phelps

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
