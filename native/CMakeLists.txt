add_library(nvision_core STATIC
    acquisition/src/file_camera.cpp
    acquisition/src/frame.cpp
    acquisition/src/image.cpp
    acquisition/src/image_view.cpp
    core/src/engine.cpp
    core/src/execution_result.cpp
    core/src/geometry.cpp
    core/src/identifier.cpp
    core/src/inspection_outcome.cpp
    core/src/resource_reference.cpp
    core/src/semantic_version.cpp)

target_include_directories(nvision_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/acquisition/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>)

target_compile_features(nvision_core PUBLIC cxx_std_20)

# Workflow owns the tool catalog, the persisted graph contracts, and execution. Core
# keeps the data vocabulary those contracts are expressed in: identifiers, versions,
# port data kinds, geometry, resources, status, and results.
add_library(nvision_workflow STATIC
    workflow/src/builtin_catalog.cpp
    workflow/src/parameter_schema.cpp
    workflow/src/port_value.cpp
    workflow/src/subworkflow.cpp
    workflow/src/tool_definition.cpp
    workflow/src/tool_execution.cpp
    workflow/src/tool_implementation_registry.cpp
    workflow/src/tool_registry.cpp
    workflow/src/workflow_graph.cpp
    workflow/src/workflow_node.cpp)

target_include_directories(nvision_workflow
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/workflow/include>)

target_compile_features(nvision_workflow PUBLIC cxx_std_20)
target_link_libraries(nvision_workflow PUBLIC nvision_core)

# Vision owns the generic image tool implementations. OpenCV is a required dependency of
# this library and stays PRIVATE, so it can never reach a public NVision header. It is
# located through the standard CMake search, which honours the OpenCV_DIR environment
# variable, so no build file in this repository contains a machine-specific path.
find_package(OpenCV REQUIRED CONFIG)
message(STATUS "NVision vision tools build against OpenCV ${OpenCV_VERSION}")

add_library(nvision_vision STATIC
    vision/src/builtin_tool_implementations.cpp
    vision/src/crop_tool.cpp
    vision/src/opencv_interop.cpp
    vision/src/threshold_tool.cpp)

target_include_directories(nvision_vision
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vision/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vision/src
        ${OpenCV_INCLUDE_DIRS})

target_compile_features(nvision_vision PUBLIC cxx_std_20)
target_link_libraries(nvision_vision PUBLIC nvision_workflow)
target_link_libraries(nvision_vision PRIVATE ${OpenCV_LIBS})

add_library(nvision_engine SHARED
    abi/src/nvision_c_api.cpp)

target_include_directories(nvision_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/abi/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/acquisition/include
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
    target_compile_options(nvision_workflow PRIVATE /W4 /WX /permissive- /EHsc)
    target_compile_options(nvision_vision PRIVATE /W4 /WX /permissive- /EHsc)
    target_compile_options(nvision_engine PRIVATE /W4 /WX /permissive- /EHsc)
else()
    target_compile_options(nvision_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(nvision_workflow PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(nvision_vision PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(nvision_engine PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(NVISION_BUILD_TESTS)
    enable_testing()

    add_executable(nvision_native_tests
        tests/native_engine_tests.cpp)

    target_link_libraries(nvision_native_tests PRIVATE nvision_engine)
    target_compile_features(nvision_native_tests PRIVATE cxx_std_20)
    target_compile_definitions(nvision_native_tests PRIVATE
        NVISION_TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/samples/wire-marker/fixtures")

    if(MSVC)
        target_compile_options(nvision_native_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(nvision_native_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_native_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_native_tests COMMAND nvision_native_tests)

    add_executable(nvision_acquisition_tests
        tests/acquisition_tests.cpp)

    target_link_libraries(nvision_acquisition_tests PRIVATE nvision_core)
    target_compile_features(nvision_acquisition_tests PRIVATE cxx_std_20)
    target_compile_definitions(nvision_acquisition_tests PRIVATE
        NVISION_TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/samples/wire-marker/fixtures")

    if(MSVC)
        target_compile_options(nvision_acquisition_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(nvision_acquisition_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_acquisition_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_acquisition_tests COMMAND nvision_acquisition_tests)

    add_executable(nvision_foundational_contracts_tests
        tests/foundational_contracts_tests.cpp)

    target_link_libraries(nvision_foundational_contracts_tests PRIVATE nvision_core)
    target_compile_features(nvision_foundational_contracts_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_foundational_contracts_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(nvision_foundational_contracts_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_foundational_contracts_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(
        NAME nvision_foundational_contracts_tests
        COMMAND nvision_foundational_contracts_tests)

    add_executable(nvision_execution_result_tests
        tests/execution_result_tests.cpp)

    target_link_libraries(nvision_execution_result_tests PRIVATE nvision_core)
    target_compile_features(nvision_execution_result_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_execution_result_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(nvision_execution_result_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_execution_result_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_execution_result_tests COMMAND nvision_execution_result_tests)

    add_executable(nvision_tool_catalog_tests
        tests/tool_catalog_tests.cpp)

    target_link_libraries(nvision_tool_catalog_tests PRIVATE nvision_workflow)
    target_compile_features(nvision_tool_catalog_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_tool_catalog_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(nvision_tool_catalog_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_tool_catalog_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_tool_catalog_tests COMMAND nvision_tool_catalog_tests)

    add_executable(nvision_parameter_schema_tests
        tests/parameter_schema_tests.cpp)

    target_link_libraries(nvision_parameter_schema_tests PRIVATE nvision_workflow)
    target_compile_features(nvision_parameter_schema_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_parameter_schema_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(
            nvision_parameter_schema_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_parameter_schema_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_parameter_schema_tests COMMAND nvision_parameter_schema_tests)

    add_executable(nvision_workflow_contract_tests
        tests/workflow_contract_tests.cpp)

    target_link_libraries(nvision_workflow_contract_tests PRIVATE nvision_workflow)
    target_compile_features(nvision_workflow_contract_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_workflow_contract_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(
            nvision_workflow_contract_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_workflow_contract_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_workflow_contract_tests COMMAND nvision_workflow_contract_tests)

    add_executable(nvision_builtin_catalog_tests
        tests/builtin_catalog_tests.cpp)

    target_link_libraries(nvision_builtin_catalog_tests PRIVATE nvision_workflow)
    target_compile_features(nvision_builtin_catalog_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_builtin_catalog_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(
            nvision_builtin_catalog_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_builtin_catalog_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_builtin_catalog_tests COMMAND nvision_builtin_catalog_tests)

    add_executable(nvision_reference_graph_tests
        tests/reference_graph_tests.cpp)

    target_link_libraries(nvision_reference_graph_tests PRIVATE nvision_workflow)
    target_compile_features(nvision_reference_graph_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_reference_graph_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(
            nvision_reference_graph_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_reference_graph_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_reference_graph_tests COMMAND nvision_reference_graph_tests)

    add_executable(nvision_tool_execution_tests
        tests/tool_execution_tests.cpp)

    target_link_libraries(nvision_tool_execution_tests PRIVATE nvision_vision)
    target_compile_features(nvision_tool_execution_tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(nvision_tool_execution_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(
            nvision_tool_execution_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_tool_execution_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    # A test executable that links the vision library needs the OpenCV runtime beside it,
    # otherwise the suite fails to start rather than reporting a real assertion.
    if(WIN32)
        add_custom_command(TARGET nvision_tool_execution_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:nvision_tool_execution_tests>
                $<TARGET_FILE_DIR:nvision_tool_execution_tests>
            COMMAND_EXPAND_LISTS)
    endif()

    add_test(NAME nvision_tool_execution_tests COMMAND nvision_tool_execution_tests)

    add_executable(nvision_public_contract_tests
        tests/public_contract_tests.cpp)

    target_link_libraries(nvision_public_contract_tests PRIVATE nvision_vision)
    target_compile_features(nvision_public_contract_tests PRIVATE cxx_std_20)
    target_compile_definitions(nvision_public_contract_tests PRIVATE
        NVISION_NATIVE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

    if(MSVC)
        target_compile_options(nvision_public_contract_tests PRIVATE /W4 /WX /permissive- /EHsc)
    else()
        target_compile_options(
            nvision_public_contract_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    set_target_properties(nvision_public_contract_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    add_test(NAME nvision_public_contract_tests COMMAND nvision_public_contract_tests)
endif()
