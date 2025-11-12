import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get


class HidAPIConan(ConanFile):
    name = "hidapi"
    description = "Multi-platform library which allows to interface with USB and Bluetooth HID-Class devices"
    license = "BSD-3-Clause"
    homepage = "https://github.com/libusb/hidapi"
    url = "https://github.com/conan-io/conan-center-index"
    topics = ("usb", "device")
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

    def requirements(self):
        if self.settings.os != "Windows":
            self.requires("libusb/1.0.26")

        if self.settings.os == "Linux":
            self.requires("libudev/system")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
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
        if self.settings.os != "Windows":
            self.cpp_info.components["libusb"].set_property("cmake_target_name", "hidapi::libusb")
            self.cpp_info.components["libusb"].set_property("pkg_config_name", "hidapi-lisbusb")
            self.cpp_info.components["libusb"].libs = ["hidapi-libusb"]
            self.cpp_info.components["libusb"].requires = ["libusb::libusb"]
            self.cpp_info.components["libusb"].includedirs.append(os.path.join("include", "hidapi"))

            self.cpp_info.components["hidraw"].set_property("cmake_target_name", "hidapi::hidraw")
            self.cpp_info.components["hidraw"].set_property("pkg_config_name", "hidapi-hidraw")
            if self.settings.os == "Linux":
                self.cpp_info.components["hidraw"].libs = ["hidapi-hidraw"]
            else:
                self.cpp_info.components["hidraw"].libs = ["hidapi"]

            self.cpp_info.components["hidraw"].includedirs.append(os.path.join("include", "hidapi"))

            if self.settings.os == "Linux":
                self.cpp_info.components["hidraw"].requires = ["libudev::libudev"]
        else:
            self.cpp_info.libs = ["hidapi"]
            self.cpp_info.includedirs.append(os.path.join("include", "hidapi"))
