cmake_minimum_required(VERSION 3.21)
project(framework LANGUAGES CXX OBJCXX)

# macOS only — use the Visual Studio .sln for Windows/PC builds
if(NOT APPLE)
    message(FATAL_ERROR "This CMakeLists.txt targets macOS. Use InternalTraining_Framework.sln for Windows builds.")
endif()
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Source files — portable (no platform APIs)
# ---------------------------------------------------------------------------
set(SOURCES_SHARED
    framework/audio/AudioResourceWave.cpp
    framework/render/BMfont.cpp
    framework/render/RenderMeshResource.cpp
    framework/render/RenderTextureResource.cpp
    framework/render/tga.cpp
    framework/render/tiny_obj_loader.cpp
    # framework/gameframework/InputManager/InputManager.cpp
    # framework/gameframework/InputManager/InputManagerAdapter.cpp
    # framework/gameframework/InputManager/InputMapper.cpp
    # framework/gameframework/log/LogSystem.cpp
    # framework/gameframework/log/LogSystemImGuiView.cpp
    # framework/gameframework/log/ImGuiManager.cpp
    # framework/gameframework/manager/AudioManager.cpp
    # framework/gameframework/manager/RendererManager.cpp
    framework/gameframework/assets/AssetCommon.cpp
)

file(GLOB_RECURSE HEADERS_PUBLIC CONFIGURE_DEPENDS
    framework/include/*
    framework/game/*
    framework/gameframework/*
)

set(HEADERS_SHARED
    framework/audio/AudioInterface.h
    framework/audio/AudioResourceWave.h
    framework/render/BMfont.h
    framework/render/RenderInterface.h
    framework/render/tga.h
    framework/render/tiny_obj_loader.h
    # framework/gameframework/InputManager/InputManager.h
    # framework/gameframework/InputManager/InputManagerAdapter.h
    # framework/gameframework/InputManager/InputMapper.h
    # framework/gameframework/InputManager/InputAction.h
    # framework/gameframework/InputManager/IInputSystem.h
    # framework/gameframework/log/LogSystem.h
    # framework/gameframework/log/LogSystemImGuiView.h
    # framework/gameframework/log/ImGuiManager.h
    # framework/gameframework/manager/AudioManager.h
    # framework/gameframework/manager/RendererManager.h
    framework/gameframework/assets/AssetCommon.h
)

set(IMGUI_SOURCES
    3rdlib/imgui/imgui.cpp
    3rdlib/imgui/imgui_draw.cpp
    3rdlib/imgui/imgui_widgets.cpp
    3rdlib/imgui/imgui_tables.cpp
    3rdlib/imgui/imgui_demo.cpp
    3rdlib/imgui/imgui_impl_metal.mm
    3rdlib/imgui/imgui_impl_osx.mm
)

set_source_files_properties(${SOURCES_OBJCXX_AS_CPP}
    PROPERTIES
    COMPILE_LANGUAGE OBJCXX
    framework/gameframework/InputManager/InputManager.cpp
    framework/gameframework/log/ImGuiManager.cpp
)

set(HEADERS_MAC
    framework/render/Mac/RenderMac.h
    framework/render/Mac/WindowMac.h
)

# ---------------------------------------------------------------------------
# Source files — Mac-specific (created in subsequent steps)
#   Step 2 : framework/main_mac.cpp
#   Step 3 : framework/render/Mac/RenderMac.mm
#             framework/render/Mac/RenderResourceMac.mm
#   Step 5 : framework/render/Mac/WindowMac.mm
#   Step 6 : framework/render/Mac/ImageIOMac.mm
#   Step 7 : framework/audio/Mac/AudioMac.mm
# ---------------------------------------------------------------------------
set(SOURCES_MAC
    framework/src/Mac/main_mac.cpp
    framework/render/Mac/RenderMac.mm
    framework/render/Mac/RenderResourceMac.mm
    framework/render/Mac/WindowMac.mm
    framework/render/Mac/ImageIOMac.mm
    framework/audio/Mac/AudioMac.mm
)

add_executable(framework
    ${SOURCES_SHARED}
    ${SOURCES_OBJCXX_AS_CPP}
    ${SOURCES_MAC}
    ${IMGUI_SOURCES}
    ${HEADERS_PUBLIC}
    ${HEADERS_SHARED}
    ${HEADERS_MAC}
)

source_group(TREE "${CMAKE_SOURCE_DIR}/framework" FILES
    ${SOURCES_SHARED}
    ${SOURCES_MAC}
    ${HEADERS_PUBLIC}
    ${HEADERS_SHARED}
    ${HEADERS_MAC}
)

# The vcxproj sets $(ProjectDir) as an include root, so all headers are
# resolved relative to the framework/ directory (e.g. "render/RenderInterface.h")
target_include_directories(framework PRIVATE
    framework
    framework/include
    framework/gameframework
    3rdlib/glm
    3rdlib/imgui
    3rdlib/doctest
)

target_compile_definitions(framework PRIVATE
    $<$<CONFIG:Debug>:_DEBUG DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(framework PRIVATE
    -Wall
    -Wextra
    -Werror
    -ffast-math
)

# CMake's Xcode generator auto-injects -lobjc for ObjC++ sources via
# -fobjc-link-runtime, which duplicates the libobjc pulled in by Cocoa/
# Foundation. Suppress the benign "Ignoring duplicate libraries: '-lobjc'"
# linker warning that results.
target_link_options(framework PRIVATE "LINKER:-no_warn_duplicate_libraries")

# macOS system frameworks required by the Mac backend
target_link_libraries(framework PRIVATE
    "-framework Metal"
    "-framework MetalKit"
    "-framework Cocoa"
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework AVFoundation"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework GameController"
)

# Set the Xcode scheme working directory so that relative asset paths
# (e.g. "assets/sound/...") resolve correctly when running from Xcode.
set_target_properties(framework PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.virtuos.training.3dtankrender"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.virtuos.training.3dtankrender"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/framework"
)

# ---------------------------------------------------------------------------
# Usage:
#   cmake -S . -B framework/build_xcode -G Xcode
#   open framework/build_xcode/framework.xcodeproj
# ---------------------------------------------------------------------------


#----------------------------------------------------------------------------
# UNIT TEST
#----------------------------------------------------------------------------

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    tests/MAC/*.cpp
    tests/MAC/*.mm
)

add_executable(framework_tests
    ${TEST_SOURCES}
)

target_include_directories(framework_tests PRIVATE
    framework
    framework/include
    3rdlib/glm
    3rdlib/doctest
    3rdlib/imgui
    # 后续添加要测试的代码文件
)

