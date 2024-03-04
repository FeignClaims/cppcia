from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy, replace_in_file
from conan.tools.scm import Git, Version
import os
import glob


required_conan_version = ">=2.0.0"


class ClangdHeadersConan(ConanFile):
    name = "clangd_headers"
    description = "headers for clangd"
    license = "Apache-2.0"
    url = "https://github.com/FeignClaims/customized_conan_recipes"
    homepage = 'https://github.com/llvm/llvm-project'
    topics = ("cpp", "compiler", "tooling", "clang")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    @property
    def _min_cppstd(self):
        ver = Version(self.version).major
        if ver < 16:
            return 14
        else:
            return 17

    def export_sources(self):
        export_conandata_patches(self)

    def configure(self):
        self._strict_options_requirements()

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires(f"llvm/{self.version}")

    def package_id(self):
        self.info.clear()

    @property
    def _required_options(self):
        options = []
        if self.version != "system":
            options.append(("llvm",
                            [("with_project_clang", True), ("with_project_clang-tools-extra", True)]))
        return options

    def _strict_options_requirements(self):
        for requirement, options in self._required_options:
            for option_name, value in options:
                setattr(self.options[requirement], f"{option_name}", value)

    def _validate_options_requirements(self):
        for requirement, options in self._required_options:
            is_missing_option = not all(self.dependencies[requirement].options.get_safe(
                f"{option_name}") == value for option_name, value in options)
            if is_missing_option:
                raise ConanInvalidConfiguration(
                    f"{self.ref} requires {requirement} with these options: {options}")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)
        self._validate_options_requirements()

    def build_requirements(self):
        self.build_requires("cmake/[>=3.21.3 <4.0.0]")
        self.build_requires("ninja/[>=1.10.0 <2.0.0]")

    def source(self):
        if self.version == "system":
            Git(self).fetch_commit(**self.conan_data["sources"][self.version])
        else:
            get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        toolchain = CMakeToolchain(self, "Ninja")
        toolchain.cache_variables["LLVM_ENABLE_PROJECTS"] = "clang;clang-tools-extra"
        toolchain.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure(build_script_folder=os.path.join(self.source_folder, 'llvm'))

    def _replace_clangtidy_include(self):
        for file in glob.glob(f'{os.path.join(self.package_folder, "include/clangd")}/**/*.h', recursive=True):
            replace_in_file(self, file, "../../clang-tidy/", "clang-tidy/", strict=False)
            replace_in_file(self, file, "../clang-tidy/", "clang-tidy/", strict=False)

    def package(self):
        copy(self, pattern="LICENSE.TXT",
             src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))
        copy(self, pattern="Features.inc",
             src=os.path.join(self.build_folder, "tools/clang/tools/extra/clangd/"),
             dst=os.path.join(self.package_folder, "include/clangd"))
        copy(self,
             pattern="*.h",
             src=os.path.join(self.source_folder, "clang-tools-extra/clangd"),
             dst=os.path.join(self.package_folder, "include/clangd"))
        copy(self,
             pattern="*.h",
             src=os.path.join(self.source_folder, "clang-tools-extra/include-cleaner/include/clang-include-cleaner"),
             dst=os.path.join(self.package_folder, "include/clang-include-cleaner"))
        self._replace_clangtidy_include()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = ['include', 'include/clangd']

        self.cpp_info.set_property("cmake_file_name", "clangd_headers")
        self.cpp_info.set_property("cmake_target_name", "clangd_headers::clangd_headers")

        self.cpp_info.requires = ["llvm::llvm"]
