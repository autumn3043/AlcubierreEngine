#CMAKE SETUP
project(Precompiler)
add_executable(modelPrecompiler.bin)

#DEPENDENCIES
    set(DEPENDENCIES)
    
    list(APPEND DEPENDENCIES xxhash)

    FetchContent_Declare(tinyobjloader GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git GIT_TAG d56555b)
    FetchContent_MakeAvailable(tinyobjloader)
    list(APPEND DEPENDENCIES tinyobjloader)

#BUILD
    set_target_properties(modelPrecompiler.bin 
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    target_include_directories(modelPrecompiler.bin
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src   
    )

    target_link_libraries(modelPrecompiler.bin
        PRIVATE
            ${DEPENDENCIES}
    )

    target_sources(modelPrecompiler.bin
        PRIVATE
            main.cpp
    )