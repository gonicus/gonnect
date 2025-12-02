from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get
import os

class QtWebDAV(ConanFile):
    name = "qtwebdav"
    description = "Qt Library to access WebDAV servers"
    license = "LGPL"
    homepage = "https://github.com/QtWebDAV/QtWebDAV"        
    package_type = "library"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_conan_qt": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_conan_qt": False,
    }

    def export_sources(self):
        export_conandata_patches(self)

    def requirements(self):
        if self.options.with_conan_qt:
            self.requires("qt/6.10.1")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)
        apply_conandata_patches(self)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables['BUILD_WITH_QT6'] = True
        tc.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "QtWebDAV")
        self.cpp_info.set_property("cmake_target_name", "QtWebDAV")
        self.cpp_info.libs = ["QtWebDAV"]
        self.cpp_info.includedirs.append(os.path.join("include"))
