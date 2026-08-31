import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, rmdir


class QCoro(ConanFile):
    name = "qcoro"
    description = "The QCoro library provides set of tools to make use of C++20 coroutines with Qt."
    license = "MIT"
    homepage = "https://github.com/qcoro/qcoro"
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

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        if self.options.with_conan_qt:
            self.requires("qt/6.10.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables['QCORO_BUILD_EXAMPLES'] = False
        tc.variables['QCORO_BUILD_TESTING'] = False
        tc.variables['QCORO_ENABLE_ASAN'] = False
        tc.variables['QCORO_DISABLE_DEPRECATED_TASK_H'] = True
        tc.variables['QCORO_WITH_QTDBUS'] = self.settings.os == "Linux"
        tc.variables['QCORO_WITH_QTNETWORK'] = True
        tc.variables['QCORO_WITH_QTWEBSOCKETS'] = True
        tc.variables['QCORO_WITH_QTQUICK'] = False
        tc.variables['QCORO_WITH_QML'] = False
        tc.variables['QCORO_WITH_QTTEST'] = True
        tc.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        cmake_base_dir = os.path.join(self.package_folder, 'lib', 'cmake', 'QCoro6Coro')

        with open(os.path.join(cmake_base_dir, 'QCoroMacros.cmake'), 'r') as file:
            data = file.read()

        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

        os.makedirs(cmake_base_dir)
        f = open(os.path.join(cmake_base_dir, 'QCoroMacros.cmake'), 'w')
        f.write(data)
        f.close()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "QCoro6")
        self.cpp_info.set_property("cmake_find_mode", "both")

        self.cpp_info.set_property("cmake_build_modules", [
            os.path.join("lib", "cmake", "QCoro6Coro", "QCoroMacros.cmake")
        ])

        self.cpp_info.components["core"].set_property("cmake_file_name", "QCoro6Core")
        self.cpp_info.components["core"].set_property("cmake_target_name", "QCoro6::Core")
        self.cpp_info.components["core"].libs = ["QCoro6Core"]
        self.cpp_info.components["core"].includedirs = [
            os.path.join("include", "qcoro6"),
            os.path.join("include", "qcoro6", "qcoro"),
            os.path.join("include", "qcoro6", "QCoro")
        ]

        if self.settings.os == "Linux":
            self.cpp_info.components["dbus"].set_property("cmake_file_name", "QCoro6DBus")
            self.cpp_info.components["dbus"].set_property("cmake_target_name", "QCoro6::DBus")
            self.cpp_info.components["dbus"].libs = ["QCoro6DBus"]
            self.cpp_info.components["dbus"].requires = ["core"]

        self.cpp_info.components["network"].set_property("cmake_file_name", "QCoro6Network")
        self.cpp_info.components["network"].set_property("cmake_target_name", "QCoro6::Network")
        self.cpp_info.components["network"].libs = ["QCoro6Network"]
        self.cpp_info.components["network"].requires = ["core"]

        self.cpp_info.components["websockets"].set_property("cmake_file_name", "QCoro6WebSockets")
        self.cpp_info.components["websockets"].set_property("cmake_target_name", "QCoro6::WebSockets")
        self.cpp_info.components["websockets"].libs = ["QCoro6WebSockets"]
        self.cpp_info.components["websockets"].requires = ["core"]
