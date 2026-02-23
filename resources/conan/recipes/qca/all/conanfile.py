import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, collect_libs
from conan.tools.env import Environment, VirtualBuildEnv, VirtualRunEnv
from conan.tools.build import cross_building

class QCA(ConanFile):
    name = "qca"    
    license = "GPL"
    homepage = "https://github.com/KDE/qca"        
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

        self.requires("openssl/3.5.4")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        vbe = VirtualBuildEnv(self)
        vbe.generate()
        if not cross_building(self):
            vre = VirtualRunEnv(self)
            vre.generate(scope="build")

        # TODO: to remove when properly handled by conan (see https://github.com/conan-io/conan/issues/11962)
        env = Environment()
        env.unset("VCPKG_ROOT")
        env.prepend_path("PKG_CONFIG_PATH", self.generators_folder)
        env.vars(self).save_script("conanbuildenv_pkg_config_path")

        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables['BUILD_WITH_QT6'] = True
        tc.variables['BUILD_TESTS'] = False
        tc.variables['BUILD_TOOLS'] = False
        tc.variables['BUILD_PLUGINS'] = "ossl"
        tc.variables['OSX_FRAMEWORK'] = False
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
        self.cpp_info.set_property("cmake_file_name", "Qca-qt6")
        self.cpp_info.set_property("cmake_target_name", "qca-qt6")
        self.cpp_info.libs = collect_libs(self)
        self.cpp_info.includedirs.append(os.path.join("include", "Qca-qt6"))
        self.cpp_info.includedirs.append(os.path.join("include", "Qca-qt6", "QtCrypto"))
