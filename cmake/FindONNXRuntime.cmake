# Locates an ONNX Runtime distribution and exposes it as ONNXRuntime::ONNXRuntime.
#
# Neither the official release archives nor the NuGet package ship a CMake config package,
# so a find module is required rather than a convenience. It expects the layout the
# official archives use:
#
#   <root>/include/onnxruntime_cxx_api.h
#   <root>/lib/onnxruntime.{lib,dll}          on Windows
#   <root>/lib/libonnxruntime.so              on Linux
#
# No path in this repository names a machine. The distribution is found through the
# standard ONNXRuntime_ROOT CMake variable, the ONNXRUNTIME_ROOT environment variable, or
# the repository-local staging directory that scripts/install-onnxruntime.ps1 writes, in
# that order. This mirrors how OpenCV is found through OpenCV_DIR.

set(_nvision_ort_hints)
if(ONNXRuntime_ROOT)
    list(APPEND _nvision_ort_hints "${ONNXRuntime_ROOT}")
endif()
if(DEFINED ENV{ONNXRUNTIME_ROOT})
    list(APPEND _nvision_ort_hints "$ENV{ONNXRUNTIME_ROOT}")
endif()
list(APPEND _nvision_ort_hints "${PROJECT_SOURCE_DIR}/artifacts/onnxruntime")

find_path(ONNXRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS ${_nvision_ort_hints}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH)

find_library(ONNXRuntime_LIBRARY
    NAMES onnxruntime
    HINTS ${_nvision_ort_hints}
    PATH_SUFFIXES lib lib64
    NO_DEFAULT_PATH)

if(WIN32)
    # The import library is a stub: the C API exports one entry point and hands back a
    # struct of function pointers. The DLL beside it is what actually has to be deployed.
    find_file(ONNXRuntime_RUNTIME_LIBRARY
        NAMES onnxruntime.dll
        HINTS ${_nvision_ort_hints}
        PATH_SUFFIXES lib bin
        NO_DEFAULT_PATH)
else()
    set(ONNXRuntime_RUNTIME_LIBRARY "${ONNXRuntime_LIBRARY}")
endif()

# The archives record their version in a plain text file rather than in a header, so the
# version is read where it is written rather than inferred.
if(ONNXRuntime_INCLUDE_DIR)
    get_filename_component(_nvision_ort_root "${ONNXRuntime_INCLUDE_DIR}" DIRECTORY)
    if(EXISTS "${_nvision_ort_root}/VERSION_NUMBER")
        file(READ "${_nvision_ort_root}/VERSION_NUMBER" _nvision_ort_version)
        string(STRIP "${_nvision_ort_version}" ONNXRuntime_VERSION)
    endif()
    unset(_nvision_ort_root)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRuntime
    REQUIRED_VARS ONNXRuntime_LIBRARY ONNXRuntime_INCLUDE_DIR ONNXRuntime_RUNTIME_LIBRARY
    VERSION_VAR ONNXRuntime_VERSION
    FAIL_MESSAGE
        "ONNX Runtime was not found. Run scripts/install-onnxruntime.ps1, or set the \
ONNXRUNTIME_ROOT environment variable to a distribution containing include/ and lib/.")

if(ONNXRuntime_FOUND AND NOT TARGET ONNXRuntime::ONNXRuntime)
    add_library(ONNXRuntime::ONNXRuntime SHARED IMPORTED)
    set_target_properties(ONNXRuntime::ONNXRuntime PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRuntime_INCLUDE_DIR}"
        IMPORTED_LOCATION "${ONNXRuntime_RUNTIME_LIBRARY}")
    if(WIN32)
        set_target_properties(ONNXRuntime::ONNXRuntime PROPERTIES
            IMPORTED_IMPLIB "${ONNXRuntime_LIBRARY}")
    endif()
endif()

mark_as_advanced(
    ONNXRuntime_INCLUDE_DIR
    ONNXRuntime_LIBRARY
    ONNXRuntime_RUNTIME_LIBRARY)

unset(_nvision_ort_hints)
