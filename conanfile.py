from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, rm, rmdir
from conan.tools.scm import Version
import os


class CppciaRecipe(ConanFile):
    name = "cppcia"
    description = "Change Impact Analysis tool for C++"
    license = "Unlicense"
    url = "https://github.com/FeignClaims/cppcia"
    homepage = "https://github.com/FeignClaims/cppcia"
    topics = ("tool", "change-impact-analysis")
    package_type = "application"
    settings = "os", "arch", "compiler", "build_type"
    version = "0.0.1"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_llvm": [True, False]
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "with_llvm": False
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self._strict_options_requirements()

    def layout(self):
        self.folders.build_folder_vars = ["settings.compiler"]
        cmake_layout(self)

    def requirements(self):
        if self.options.with_llvm:
            self.requires("clangd_headers/18.1.4")
        else:
            self.requires("clangd_headers/system")
        self.requires("ctre/3.8.1")
        self.requires("fmt/10.2.1")
        self.requires("graaf/1.1.1")
        self.requires("magic_enum/0.9.5")
        self.requires("ms-gsl/4.0.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("range-v3/0.12.0")
        self.requires("zlib/1.3.1", override=True)

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {"msvc": "193",
                "gcc": "11",
                "clang": "13",
                "apple-clang": "14"}

    def _validate_cppstd(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
            )

    @property
    def _required_options(self):
        options = []
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
        check_min_cppstd(self, "20")
        self._validate_options_requirements()

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.28 <4.0.0]")
        self.test_requires("catch2/3.5.4")

    def generate(self):
        CMakeDeps(self).generate()
        toolchain = CMakeToolchain(self)
        toolchain.presets_prefix = ""
        toolchain.cache_variables["USE_SYSTEM_LLVM"] = not self.options.get_safe("with_llvm", False)
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if not self.conf.get("tools.build:skip_test", default=False):
            cmake.ctest(cli_args=["--rerun-failed", "--output-on-failure"])

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package(self):
        copy(self, pattern="LICENSE", dst=os.path.join(self.package_folder, "licenses"), src=self.source_folder)
        cmake = CMake(self)
        cmake.install()

        # some files extensions and folders are not allowed. Please, read the FAQs to get informed.
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.la", os.path.join(self.package_folder, "lib"))
        rm(self, "*.pdb", os.path.join(self.package_folder, "lib"))
        rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))

    def package_info(self):
        self.cpp_info.libs = ["cppcia"]

        self.cpp_info.set_property("cmake_file_name", "cppcia")
        self.cpp_info.set_property("cmake_target_name", "cppcia::cppcia")
