include_guard(GLOBAL)

# Adding LLVM dependencies using wrapper.cmake or by conan?
option(BUILDING_LLVM_BY_CONAN "LLVM dependency is installed by conan instead of llvm wrapper" ON)

if(BUILDING_LLVM_BY_CONAN)
  find_package(llvm CONFIG REQUIRED)

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
else()
  include(FetchContent)
  FetchContent_Declare(_llvm_wrapper
    GIT_REPOSITORY https://github.com/LLVMParty/LLVMCMakeTemplate.git
    GIT_TAG master
    GIT_SHALLOW true
  )
  FetchContent_Populate(_llvm_wrapper)
  include(${_llvm_wrapper_SOURCE_DIR}/cmake/LLVM.cmake)
  include(${_llvm_wrapper_SOURCE_DIR}/cmake/Clang.cmake)

  add_library(llvm::llvm INTERFACE IMPORTED)
  set_property(TARGET llvm::llvm PROPERTY INTERFACE_LINK_LIBRARIES LLVM-Wrapper APPEND)
  set_property(TARGET llvm::llvm PROPERTY INTERFACE_LINK_LIBRARIES Clang-Wrapper APPEND)

  install(
    DIRECTORY "${LLVM_LIBRARY_DIR}/clang"
    TYPE LIB
    FILES_MATCHING
    PATTERN "*.h"
  )
endif()