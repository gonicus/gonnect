from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get


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

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "qcoro")
        self.cpp_info.set_property("cmake_target_name", "qcoro")
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.libs = ["qcorod" if self.settings.os == "Windows" and self.settings.build_type == "Debug" else "qcoro"]
