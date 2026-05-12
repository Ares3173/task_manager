cmake_minimum_required(VERSION 3.25)

project(task_manager
    VERSION 0.1.0
    DESCRIPTION "Windows process inspection library"
    LANGUAGES CXX)

# ─── Project-wide settings ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if user didn't specify
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ─── Options ────────────────────────────────────────────────────────────
option(TASK_MANAGER_BUILD_TESTS    "Build tests"    ON)
option(TASK_MANAGER_BUILD_EXAMPLES "Build examples" ON)

# Only build tests/examples when this is the top-level project,
# not when consumed via FetchContent
if(NOT PROJECT_IS_TOP_LEVEL)
    set(TASK_MANAGER_BUILD_TESTS    OFF)
    set(TASK_MANAGER_BUILD_EXAMPLES OFF)
endif()

# ─── Platform guard ─────────────────────────────────────────────────────
if(NOT WIN32)
    message(FATAL_ERROR "task_manager is Windows-only")
endif()

# ─── The library target ─────────────────────────────────────────────────
add_library(task_manager STATIC)
add_library(task_manager::task_manager ALIAS task_manager)

target_sources(task_manager PRIVATE
    src/error.cpp
    src/handle.cpp
    src/process/process.cpp
    src/process/memory.cpp
    src/process/modules.cpp
    src/thread/thread.cpp
    src/thread/context.cpp
    src/system/privileges.cpp
    src/system/token.cpp
    src/detail/nt_api.cpp
    src/detail/unique_handle.cpp)

target_include_directories(task_manager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(task_manager PUBLIC cxx_std_23)

target_link_libraries(task_manager
    PRIVATE
        ntdll)  # for NT API functions

# ─── Compiler warnings ──────────────────────────────────────────────────
if(MSVC)
    target_compile_options(task_manager PRIVATE
        /W4 /WX /permissive- /Zc:__cplusplus)
    target_compile_definitions(task_manager PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS)
endif()

# ─── Subdirectories ─────────────────────────────────────────────────────
if(TASK_MANAGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(TASK_MANAGER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ─── Install / package config ──────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 1. Install the library binary + tell CMake to remember its target info
install(TARGETS task_manager
    EXPORT  task_manager-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 2. Install the public headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 3. Generate task_manager-targets.cmake from the EXPORT we declared above
install(EXPORT task_manager-targets
    FILE        task_manager-targets.cmake
    NAMESPACE   task_manager::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/task_manager)

# 4. Process the .in template into a real config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/task_manager-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/task_manager-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/task_manager)

# 5. Generate a version file so find_package(task_manager 1.0) works
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/task_manager-config-version.cmake
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

# 6. Install the two generated files alongside the targets file
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/task_manager-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/task_manager-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/task_manager)
