#CMAKE SETUP
cmake_minimum_required(VERSION 4.2.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

project(AlcubierreEngine)

add_library(Alcubierre SHARED)

#DEPENDENCIES
    set(DEPENDENCIES)

    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 7b6aead)
    FetchContent_MakeAvailable(glfw)
    list(APPEND DEPENDENCIES glfw)

    FetchContent_Declare(spirv_reflect GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git GIT_TAG ef913b3)
    set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(spirv_reflect)
    list(APPEND DEPENDENCIES spirv-reflect-static)

    FetchContent_Declare(xxhash GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git GIT_TAG e626a72)
    FetchContent_MakeAvailable(xxhash)
    list(APPEND DEPENDENCIES xxhash)

    find_package(Vulkan REQUIRED)
    list(APPEND DEPENDENCIES "Vulkan::Vulkan")

#COLLECT SOURCES
    set(OBJECTS)

    set(SOURCES
        core/PublicInterface
        core/Registry
        core/DebugManager

        module/ConfigManager
        module/VulkanHandler
        module/GLFWHandler
        module/GLFWHandler/GLFWSurfaceBridge
        module/ResourceManager
        module/Hasher
        module/DiskInterface
        module/ShaderProcessor
        module/Director
    )

    foreach(source IN LISTS SOURCES)
        list(APPEND OBJECTS "src/${source}/source.cpp")
    endforeach()

#BUILD
    target_link_libraries(Alcubierre 
        PRIVATE 
            ${DEPENDENCIES}
    )

    target_include_directories(Alcubierre
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )

    target_sources(Alcubierre
        PRIVATE
            ${OBJECTS}
    )

#COPY HEADER
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/AlcubierreEngine.h
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/src/core/PublicInterface/public.h
                ${CMAKE_BINARY_DIR}/AlcubierreEngine.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/core/PublicInterface/public.h
    )

    add_custom_target(copy_public_header ALL
        DEPENDS ${CMAKE_BINARY_DIR}/AlcubierreEngine.h
    )

#TOOLS
    add_subdirectory(src/extern/ModelPrecompiler ${CMAKE_BINARY_DIR}/tools/ModelPrecompiler)