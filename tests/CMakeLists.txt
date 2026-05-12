# ─── Catch2 via FetchContent ────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

# ─── Test executable ────────────────────────────────────────────────────
add_executable(task_manager_tests
    unit/test_error.cpp
    unit/test_types.cpp
)

target_link_libraries(task_manager_tests
    PRIVATE
        task_manager::task_manager
        Catch2::Catch2WithMain
)

target_compile_features(task_manager_tests PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(task_manager_tests PRIVATE
        /W4 /WX /permissive- /Zc:__cplusplus)
    target_compile_definitions(task_manager_tests PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS)
endif()

# Register each TEST_CASE with CTest. Catch2 tags become CTest labels,
# so `ctest -L unit` runs only the fast tier.
catch_discover_tests(task_manager_tests)
