add_library(nvision_core STATIC
    core/src/engine.cpp)

target_include_directories(nvision_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>)

target_compile_features(nvision_core PUBLIC cxx_std_20)

add_library(nvision_engine SHARED
    abi/src/nvision_c_api.cpp)

target_include_directories(nvision_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/abi/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include)

target_compile_definitions(nvision_engine PRIVATE NVISION_ENGINE_BUILD)
target_link_libraries(nvision_engine PRIVATE nvision_core)
set_target_properties(nvision_engine PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")

if(MSVC)
    target_compile_options(nvision_core PRIVATE /W4 /WX /permissive- /EHsc)
    target_compile_options(nvision_engine PRIVATE /W4 /WX /permissive- /EHsc)
else()
    target_compile_options(nvision_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(nvision_engine PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(NVISION_BUILD_TESTS)
    enable_testing()

    add_executable(nvision_native_tests
        tests/native_engine_tests.cpp)

    target_link_libraries(nvision_native_tests PRIVATE nvision_engine)
    target_compile_features(nvision_native_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_native_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(nvision_native_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_native_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_native_tests COMMAND nvision_native_tests)
endif()
