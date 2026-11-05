from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get
import os

class QCoro(ConanFile):
    name = "qcoro6"
    description = "C++ Coroutines for Qt"
    license = "MIT"
    homepage = "https://github.com/qcoro/qcoro"        
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
        tc.variables['QCORO_BUILD_EXAMPLES'] = False
        tc.variables['QCORO_BUILD_TESTING'] = False
        tc.variables['QCORO_WITH_QTTEST'] = False
        tc.variables['QCORO_BUILD_ASAN'] = False
        tc.variables['QCORO_DISABLE_DEPRECATED_TASK_H'] = True
        tc.variables['QCORO_WITH_QTDBUS'] = True
        tc.variables['QCORO_WITH_QTNETWORK'] = True
        tc.variables['QCORO_WITH_QTWEBSOCKETS'] = False
        tc.variables['QCORO_WITH_QTQUICK'] = False
        tc.variables['QCORO_WITH_QML'] = False
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
        self.cpp_info.set_property("cmake_file_name", "QCoro6")
        self.cpp_info.set_property("cmake_target_name", "QCoro6")

        self.cpp_info.components["core"].set_property("cmake_target_name", "QCoro6::Core")
        self.cpp_info.components["core"].libs = ["QCoro6Core"]
        self.cpp_info.components["core"].includedirs.append(os.path.join("include", "qcoro6"))
        self.cpp_info.components["core"].includedirs.append(os.path.join("include", "qcoro6", "qcoro"))
        self.cpp_info.components["core"].includedirs.append(os.path.join("include", "qcoro6", "QCoro"))

        self.cpp_info.components["network"].set_property("cmake_target_name", "QCoro6::Network")
        self.cpp_info.components["network"].libs = ["QCoro6Network"]
        self.cpp_info.components["network"].requires = ["core"]

        if self.settings.os == "Linux":
            self.cpp_info.components["dbus"].set_property("cmake_target_name", "QCoro6::DBus")
            self.cpp_info.components["dbus"].libs = ["QCoro6DBus"]
            self.cpp_info.components["dbus"].requires = ["core"]
