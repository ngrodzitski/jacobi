from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import rmdir
from conan.tools.scm import Version
import os, sys, re


class JacobiConan(ConanFile):

    def set_version(self):
        version_file_path = os.path.join(
            self.recipe_folder,
            "jacobi/include/jacobi/version.hpp"
        )
        with open(version_file_path, 'r') as file:
            content = file.read()
            major_match = re.search(r'VERSION_MAJOR\s+(\d+)ull', content)
            minor_match = re.search(r'VERSION_MINOR\s+(\d+)ull', content)
            patch_match = re.search(r'VERSION_PATCH\s+(\d+)ull', content)

            if major_match and minor_match and patch_match:
                major = int(major_match.group(1))
                minor = int(minor_match.group(1))
                patch = int(patch_match.group(1))
                self.version = f"{major}.{minor}.{patch}"
            else:
                raise ValueError(f"cannot detect version from {version_file_path}")

    options = {
        "fPIC": [True, False],
    }
    default_options = {
        "fPIC": True,
    }

    name = "jacobi"

    license = "BSL-1.0"
    author = "Nicolai Grodzitski"
    url = "https://github.com/ngrodzitski/jacobi"
    homepage = "https://github.com/ngrodzitski/jacobi"
    description = "Just Another Collection of Order Book Implementations"

    topics = ("jacobi", "limit order book", "order book")

    settings = "os", "compiler", "build_type", "arch"

    exports_sources = [
        "CMakeLists.txt",
        "jacobi/*",
        "book/*",
        "cmake-scripts/*"
    ]
    no_copy_source = False
    build_policy = "missing"

    def _compiler_support_lut(self):
        return {
            "gcc": "11",
            "clang": "12",
            "msvc": "193"
        }

    # This hint tells that this conanfile acts as
    # a conanfile for a package, which implies
    # it is responsible only for library itself.
    # Used to eliminate tests-related stuff (gtest, building tests)
    ACT_AS_PACKAGE_ONLY_CONANFILE = False

    def _is_package_only(self):
        return (
            self.ACT_AS_PACKAGE_ONLY_CONANFILE
            # The environment variable below can be used
            # to run conan create localy (used for debugging issues).
            or os.environ.get("JACOBI_CONAN_PACKAGING") == "ON"
        )

    def requirements(self):
        # TODO: add your libraries here
        self.requires("fmt/12.1.0")
        self.requires("boost/1.89.0")
        self.requires("type_safe/0.2.4")

        # Currently use a custom fork:
        # self.requires("plf_list/2.70")

        self.requires("range-v3/0.12.0")
        self.requires("tsl-robin-map/1.4.0")
        self.requires("abseil/20240116.1")

    def build_requirements(self):
        if not self._is_package_only():
            self.test_requires("gtest/1.17.0")
            self.test_requires("benchmark/1.9.4")
            self.test_requires("cli11/2.6.0")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        # Force Boost to be header-only
        self.options["boost"].header_only = True

    def validate(self):
        minimal_cpp_standard = "20"
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, minimal_cpp_standard)
        minimal_version = self._compiler_support_lut()

        compiler = str(self.settings.compiler)
        if compiler not in minimal_version:
            self.output.warning(
                "%s recipe lacks information about the %s compiler standard version support" % (self.name, compiler))
            self.output.warning(
                "%s requires a compiler that supports at least C++%s" % (self.name, minimal_cpp_standard))
            return

        version = Version(self.settings.compiler.version)
        if version < minimal_version[compiler]:
            raise ConanInvalidConfiguration("%s requires a compiler that supports at least C++%s" % (self.name, minimal_cpp_standard))

    def layout(self):
        cmake_layout(self, src_folder=".", build_folder=".")
        self.folders.generators = ""

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["JACOBI_INSTALL"] = True
        tc.variables[
            "JACOBI_BUILD_TESTS"
        ] = not self._is_package_only()

        tc.generate()

        cmake_deps = CMakeDeps(self)
        cmake_deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder=self.source_folder)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "jacobi")
        self.cpp_info.set_property("cmake_target_name", "jacobi::jacobi")

        self.cpp_info.names["cmake_find_package"] = "MyTestLib"
        self.cpp_info.names["cmake_find_package_multi"] = "MyTestLib"

        component_name = "_jacobi"

        self.cpp_info.components[component_name].set_property("cmake_target_name", "jacobi::jacobi")

        self.cpp_info.components[component_name].names["cmake_find_package"] = "jacobi"
        self.cpp_info.components[component_name].names["cmake_find_package_multi"] = "jacobi"
        self.cpp_info.components[component_name].set_property("cmake_target_name", "jacobi::jacobi")
        self.cpp_info.components[component_name].set_property("pkg_config_name", "jacobi::jacobi")


        # TODO: consider adding alloaces
        # self.cpp_info.components[component_name].set_property("cmake_target_aliases", [f"{self.name}::{self.name}"])
        self.cpp_info.components[component_name].libs = ["jacobi"]

        self.cpp_info.components[component_name].requires = [
            # TODO: add dependencies here.
            "fmt::fmt"
        ]

        # OS dependent settings
        # Here is an example:
        # if self.settings.os in ["Linux", "FreeBSD"]:
        #     self.cpp_info.components[component_name].system_libs.append("m")
        #     self.cpp_info.components[component_name].system_libs.append("pthread")
