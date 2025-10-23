import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, rm


class LibICALConan(ConanFile):
    name = "libical"
    description = "An implementation of iCalendar protocols and data formats"
    license = "LGPL-2.1-or-later"
    homepage = "https://github.com/libical/libical"
    url = "https://github.com/libical/libical"
    topics = ("communication", "calendar")
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

    #def requirements(self):
    #    self.requires("glib/2.78.3")
    #    self.requires("gobject-introspection/1.78.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)

        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["GOBJECT_INTROSPECTION"] = False
        tc.variables["ICAL_GLIB"] = False
        tc.variables["ICAL_GLIB_VAPI"] = False
        tc.variables["ICAL_BUILD_DOCS"] = False

        rm(self, "FindGLib.cmake", os.path.join(self.source_folder, "cmake"))
        rm(self, "FindGObjectIntrospection.cmake", os.path.join(self.source_folder, "cmake", "modules"))

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
        self.cpp_info.libs = ["ical"]
