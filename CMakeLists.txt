cmake_minimum_required(VERSION 3.10)
project(CornellRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =====================================
# Source Files
# =====================================
file(GLOB SRC_FILES
    src/*.cpp
    src/*.c       # for glad.c
)

# =====================================
# Executable
# =====================================
add_executable(renderer ${SRC_FILES})

# =====================================
# Include Paths
# =====================================
target_include_directories(renderer PRIVATE
    include
    include/renderer
    include/glad
    include/KHR
    include/stb
)

# =====================================
# OpenGL + GLFW
# =====================================
find_package(OpenGL REQUIRED)

# macOS vs Linux GLFW handling
if (APPLE)
    find_package(glfw3 REQUIRED)
    target_link_libraries(renderer
        OpenGL::GL
        glfw
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)

    target_link_libraries(renderer
        OpenGL::GL
        ${GLFW_LIBRARIES}
        dl pthread m
    )
endif()

# =====================================
# Copy Shaders to Build Directory
# =====================================
file(GLOB SHADER_FILES "shaders/*")
foreach(shader ${SHADER_FILES})
    configure_file(${shader} ${shader} COPYONLY)
endforeach()

message(STATUS "Shaders copied to build directory")
# Copy shaders folder to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy textures folder to build directory
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})
message(STATUS "Textures copied to build directory")    