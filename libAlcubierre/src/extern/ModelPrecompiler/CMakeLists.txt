#CMAKE SETUP
cmake_minimum_required(VERSION 4.2.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

project(ModelPrecompiler)
add_executable(modelPrecompiler.bin)

#DEPENDENCIES
    set(DEPENDENCIES)
    
    FetchContent_Declare(xxhash GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git GIT_TAG e626a72)
    FetchContent_MakeAvailable(xxhash)
    list(APPEND DEPENDENCIES xxhash)

    FetchContent_Declare(tinyobjloader GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git GIT_TAG d56555b)
    FetchContent_MakeAvailable(tinyobjloader)
    list(APPEND DEPENDENCIES tinyobjloader)

    FetchContent_Declare(CLI11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG 37bb6ed)
    FetchContent_MakeAvailable(CLI11)
    list(APPEND DEPENDENCIES CLI11)

    set(PNG_SHARED OFF CACHE BOOL "")
    set(PNG_STATIC ON CACHE BOOL "")
    set(PNG_TESTS OFF CACHE BOOL "")
    set(PNG_EXECUTABLES OFF CACHE BOOL "")

    FetchContent_Declare(libpng GIT_REPOSITORY https://github.com/pnggroup/libpng.git GIT_TAG c3e3049)
    FetchContent_MakeAvailable(libpng)
    
    set(KTX_FEATURE_TOOLS OFF CACHE BOOL "")
    set(KTX_FEATURE_TESTS OFF CACHE BOOL "")
    set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "")
    set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "")

    FetchContent_Declare(ktx GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git GIT_TAG 4d6fc70)
    FetchContent_MakeAvailable(ktx)
    list(APPEND DEPENDENCIES ktx)

#BUILD
    target_include_directories(modelPrecompiler.bin
        PRIVATE
            /home/autumn/development/Alcubierre/AlcubierreEngine/libAlcubierre/src
    )

    target_link_libraries(modelPrecompiler.bin
        PRIVATE
            ${DEPENDENCIES}
    )

    target_sources(modelPrecompiler.bin
        PRIVATE
            source.cpp
    )