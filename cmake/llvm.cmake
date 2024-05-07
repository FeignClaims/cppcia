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

  function(install_conan_llvm configuration)
    string(TOUPPER "${configuration}" upper_configuration)
    set(llvm_package_dir "${llvm_PACKAGE_FOLDER_${upper_configuration}}")

    if(llvm_package_dir)
      # install llvm shared libraries
      install(DIRECTORY
        "${llvm_package_dir}/lib/" # workaround, see https://github.com/conan-io/conan/issues/12654
        TYPE LIB
        CONFIGURATIONS ${configuration}
        FILES_MATCHING
        PATTERN "*.dylib"
        PATTERN "*.so"
        PATTERN "*.lld"
      )

      # install clang headers
      install(DIRECTORY
        "${llvm_package_dir}/lib/"
        TYPE LIB
        CONFIGURATIONS ${configuration}
        FILES_MATCHING
        PATTERN "*.h"
      )
    endif()
  endfunction()

  if(CMAKE_CONFIGURATION_TYPES)
    foreach(configuration IN LISTS CMAKE_CONFIGURATION_TYPES)
      install_conan_llvm(${configuration})
    endforeach()
  else()
    install_conan_llvm(${CMAKE_BUILD_TYPE})
  endif()
endif()