from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class starterRecipe(ConanFile):
    name = "starter"
    settings = "os", "compiler", "build_type", "arch"

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
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self._strict_options_requirements()

    def layout(self):
        self.folders.build_folder_vars = ["settings.compiler"]
        cmake_layout(self)

    def requirements(self):
        self.requires("boost/1.83.0")
        self.requires("fmt/10.1.1", force=True)
        if self.options.with_llvm:
            self.requires("llvm/16.0.6")
        self.requires("ms-gsl/4.0.0")
        self.requires("nlohmann_json/3.11.2")
        self.requires("range-v3/0.12.0")
        self.requires("spdlog/1.12.0")
        self.requires("zlib/1.2.13", override=True)

    @property
    def _required_options(self):
        options = []
        if self.options.with_llvm:
            options.append(("llvm",
                            [("with_project_clang", True)]))
        options.append(("boost",
                       [("without_graph", False)]))
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
        self.tool_requires("cmake/[>=3.25 <4.0.0]")
        self.test_requires("boost-ext-ut/cci.20230709")

    def generate(self):
        CMakeDeps(self).generate()
        toolchain = CMakeToolchain(self)
        toolchain.presets_prefix = ""
        toolchain.cache_variables["BUILDING_LLVM_BY_CONAN"] = self.options.get_safe(
            "with_llvm", True)
        toolchain.generate()

    def package(self):
        cmake = CMake(self)
        cmake.install()
