# - A wrapper for LLVM
# Include this module in the main CMakeLists.txt before adding targets to make use
include_guard(GLOBAL)

# Adding LLVM dependencies using wrapper.cmake or by conan?
option(USE_SYSTEM_LLVM "Use LLVM in current system instead of building by conan" ON)

if(USE_SYSTEM_LLVM)
  find_package(llvm_system CONFIG REQUIRED)

  install(
    DIRECTORY "${LLVM_LIBRARY_DIR}/clang"
    TYPE LIB
    FILES_MATCHING
    PATTERN "*.h"
  )
else()
  find_package(LLVM CONFIG REQUIRED)

  # workaround: install LLVM shared libraries. See https://github.com/conan-io/conan/issues/12654
  install(DIRECTORY
    "$<$<CONFIG:Release>:${llvm_LIB_DIRS_RELEASE}/>"
    "$<$<CONFIG:Debug>:${llvm_LIB_DIRS_DEBUG}/>"
    "$<$<CONFIG:RelWithDebInfo>:${llvm_LIB_DIRS_RELWITHDEBINFO}/>"
    "$<$<CONFIG:MinSizeRel>:${llvm_LIB_DIRS_MINSIZEREL}/>"
    "$<$<CONFIG:Release>:${zstd_LIB_DIRS_RELEASE}/>"
    "$<$<CONFIG:Debug>:${zstd_LIB_DIRS_DEBUG}/>"
    "$<$<CONFIG:RelWithDebInfo>:${zstd_LIB_DIRS_RELWITHDEBINFO}/>"
    "$<$<CONFIG:MinSizeRel>:${zstd_LIB_DIRS_MINSIZEREL}/>"
    TYPE LIB
    FILES_MATCHING
    PATTERN "*.dylib"
    PATTERN "*.so"
    PATTERN "*.lld"
  )

  # install clang headers
  install(DIRECTORY
    "$<$<CONFIG:Release>:${llvm_LIB_DIRS_RELEASE}/clang>"
    "$<$<CONFIG:Debug>:${llvm_LIB_DIRS_DEBUG}/clang>"
    "$<$<CONFIG:RelWithDebInfo>:${llvm_LIB_DIRS_RELWITHDEBINFO}/clang>"
    "$<$<CONFIG:MinSizeRel>:${llvm_LIB_DIRS_MINSIZEREL}/clang>"
    TYPE LIB
    FILES_MATCHING
    PATTERN "*.h"
  )
endif()