import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, rm, collect_libs


class KSyntaxHighlightingConan(ConanFile):
    name = "ksyntaxhighlighting"
    description = "Syntax highlighting Engine for Structured Text and Code."
    license = "MIT"
    homepage = "https://invent.kde.org/frameworks/syntax-highlighting"
    url = "https://invent.kde.org/frameworks/syntax-highlighting"
    topics = ("ast", "markdown")
    package_type = "library"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.generate()

    def build_requirements(self):
        self.tool_requires("extra-cmake-modules/[>=6.26]")

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        rm(self, "*.cmake", self.package_folder, recursive=True)
        rm(self, "*.pc", self.package_folder, recursive=True)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "KF6SyntaxHighlighting")
        self.cpp_info.set_property("cmake_target_name", "KF6::SyntaxHighlighting")
        self.cpp_info.libs = collect_libs(self)
        self.cpp_info.includedirs.append(os.path.join("include", "KF6", "KSyntaxHighlighting"))
