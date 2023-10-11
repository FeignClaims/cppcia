from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy, rmdir
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os


required_conan_version = ">=1.52.0"


class UtConan(ConanFile):
    name = "boost-ext-ut"
    description = "short description"
    license = "BSL-1.0"
    url = "https://github.com/boost-ext/ut/"
    homepage = "https://boost-ext.github.io/ut/"
    topics = ("testing", "unit-testing", "benchmark", "tdd", "bdd", "boost",
              "header-only", "testing-framework", "cpp20", "single-module")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    options = {
        "enable_module": [True, False]
    }
    default_options = {
        "enable_module": False
    }

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {"msvc": "193",
                "gcc": "11",
                "clang": "13",
                "apple-clang": "14"}

    def export_sources(self):
        export_conandata_patches(self)

    def layout(self):
        basic_layout(self, src_folder="src")

    def package_id(self):
        self.info.clear()

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
            )
        if is_msvc(self):
            if self.options.get_safe("enable_module", False):
                raise ConanInvalidConfiguration(
                    f"{self.ref} when using msvc requires enable_module=False"
                )

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["BOOST_UT_BUILD_EXAMPLES"] = False
        toolchain.cache_variables["BOOST_UT_BUILD_TESTS"] = not self.conf.get(
            "tools.build:skip_test", default=True, check_type=bool)
        # TODO: cmake.install() and support module
        toolchain.cache_variables["BOOST_UT_DISABLE_MODULE"] = not self.options.get_safe("enable_module", False)
        toolchain.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, pattern="LICENSE",
             src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

        self.cpp_info.set_property("cmake_file_name", "ut")
        self.cpp_info.set_property("cmake_target_name", "boost-ext-ut::ut")

        self.cpp_info.components["ut"].includedirs = [os.path.join("include", "ut-1.1.9", "include")]

        if not self.options.get_safe("enable_module", False):
            self.cpp_info.components["ut"].defines = ["BOOST_UT_DISABLE_MODULE=1"]
