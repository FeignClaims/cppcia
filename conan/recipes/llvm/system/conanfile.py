from conan import ConanFile
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy
import os


required_conan_version = ">=2.0.0"


class SystemLlvmConan(ConanFile):
    name = 'llvm'
    description = 'The LLVM Project is a collection of modular and reusable compiler and toolchain technologies'
    url = "https://github.com/FeignClaims/customized_conan_recipes"
    homepage = 'https://github.com/llvm/llvm-project'
    license = 'Apache-2.0'
    topics = ('cpp', 'compiler', 'tooling', 'clang')
    settings = 'os', 'arch', 'compiler', 'build_type'
    no_copy_source = True

    def export_sources(self):
        export_conandata_patches(self)

    def package_id(self):
        self.info.clear()

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def build(self):
        apply_conandata_patches(self)

    def package(self):
        copy(self, pattern="LICENSE",
             src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))
        copy(self, "Clang.cmake", self.source_folder, self.package_folder)
        copy(self, "LLVM.cmake", self.source_folder, self.package_folder)
        copy(self, "Wrapper.cmake", self.source_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []

        self.cpp_info.set_property("cmake_file_name", "llvm_system")
        self.cpp_info.set_property("cmake_target_name", "llvm::llvm")

        self.cpp_info.set_property("cmake_build_modules", ["Wrapper.cmake"])
