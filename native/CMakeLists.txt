add_library(nvision_core STATIC
    acquisition/src/file_camera.cpp
    acquisition/src/frame.cpp
    acquisition/src/image_view.cpp
    core/src/engine.cpp
    core/src/execution_result.cpp
    core/src/geometry.cpp
    core/src/identifier.cpp
    core/src/inspection_outcome.cpp
    core/src/parameter_schema.cpp
    core/src/resource_reference.cpp
    core/src/semantic_version.cpp
    core/src/subworkflow.cpp
    core/src/tool_definition.cpp
    core/src/tool_registry.cpp
    core/src/workflow_graph.cpp
    core/src/workflow_node.cpp)

target_include_directories(nvision_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/acquisition/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>)

target_compile_features(nvision_core PUBLIC cxx_std_20)

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

    target_link_libraries(nvision_tool_catalog_tests PRIVATE nvision_core)
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

    target_link_libraries(nvision_parameter_schema_tests PRIVATE nvision_core)
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

    target_link_libraries(nvision_workflow_contract_tests PRIVATE nvision_core)
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
endif()
