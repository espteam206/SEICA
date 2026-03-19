cmake_minimum_required(VERSION 3.17)

project(concrete)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link statically when building on mingw
# to avoid errors trying to find the correct dlls
if (MINGW)
    message("Platform Mingw")
    add_link_options(--static)
endif()

#######################
# Download dependencies
#######################

include(FetchContent)

set(IMGUI_VERSION "1.92.6")
set(IMPLOT_VERSION "0.17")
set(GLFW_VERSION "3.4")

FetchContent_Declare(
    imgui
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG 934c6a5f5ef2355d6df25395d555cb71f790c4e9
)

FetchContent_Declare(
    implot
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/epezent/implot/releases/download/v${IMPLOT_VERSION}/implot-v${IMPLOT_VERSION}.tar.gz
)

FetchContent_Declare(
    glfw
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/glfw/glfw/archive/refs/tags/${GLFW_VERSION}.tar.gz
)

FetchContent_MakeAvailable(imgui implot glfw)

set(IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/imconfig.h"
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui.h"
    "${imgui_SOURCE_DIR}/imgui_internal.h"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imstb_rectpack.h"
    "${imgui_SOURCE_DIR}/imstb_textedit.h"
    "${imgui_SOURCE_DIR}/imstb_truetype.h"

    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)

add_library(imgui ${IMGUI_SOURCES})

target_include_directories(
    imgui
    PUBLIC
    ${imgui_SOURCE_DIR}
)

target_link_libraries(imgui PRIVATE glfw)

add_compile_definitions(GLFW_INCLUDE_NONE)

set(IMPLOT_SOURCES
    "${implot_SOURCE_DIR}/implot.cpp"
    "${implot_SOURCE_DIR}/implot_demo.cpp"
    "${implot_SOURCE_DIR}/implot.h"
    "${implot_SOURCE_DIR}/implot_internal.h"
    "${implot_SOURCE_DIR}/implot_items.cpp"
)

add_library(implot ${IMPLOT_SOURCES})

target_include_directories(
    implot
    PUBLIC
    ${implot_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)

#############
# Find OpenGL
#############

find_package(OpenGL REQUIRED)
if (NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL not found")
endif()
message("OPENGL_FOUND:${OpenGL_FOUND}")
message("OPENGL_LIBRARIES:${OpenGL_LIBRARIES}")

######################
# Define sources files
######################

set(SOURCES
    "src/Main.cpp"
    "src/App.h"
    "src/App.cpp"
    "src/CSV.h"
    "src/Calc.cpp"
)

if (WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
elseif (UNIX)
    add_executable(${PROJECT_NAME} ${SOURCES})
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
)

########################
# Link with dependencies
########################

# glad
set(glad_SOURCE_DIR "lib/glad")

add_library(glad
    "${glad_SOURCE_DIR}/src/glad.c"
    "${glad_SOURCE_DIR}/include/glad/glad.h"
)

target_include_directories(
    glad
    PUBLIC
    "${glad_SOURCE_DIR}/include"
)

# GLFW
set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    imgui
    implot
    glfw
    glad
    ${OPENGL_LIBRARY}
    ${Python_LIBRARY}
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${implot_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)
