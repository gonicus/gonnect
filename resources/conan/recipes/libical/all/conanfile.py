import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, rm, collect_libs


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

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)

        tc.variables["LIBICAL_STATIC"] = not self.options.shared
        tc.variables["LIBICAL_JAVA_BINDINGS"] = False
        tc.variables["LIBICAL_GLIB"] = False
        tc.variables["LIBICAL_GLIB_VAPI"] = False
        tc.variables["LIBICAL_BUILD_DOCS"] = False
        tc.variables["LIBICAL_GLIB_BUILD_DOCS"] = False
        tc.variables["LIBICAL_GOBJECT_INTROSPECTION"] = False
        tc.variables["LIBICAL_BUILD_TESTING"] = False
        tc.variables["LIBICAL_BUILD_EXAMPLES"] = False

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

        rm(self, "*.cmake", self.package_folder, recursive=True)
        rm(self, "*.pc", self.package_folder, recursive=True)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "libical")
        self.cpp_info.set_property("cmake_target_name", "libical::libical")
        self.cpp_info.libs = collect_libs(self)

        if not self.options.shared:
            self.cpp_info.cxxflags = ['-DLIBICAL_ICAL_STATIC_DEFINE', '-DLIBICAL_ICALSS_STATIC_DEFINE', '-DLIBICAL_VCAL_STATIC_DEFINE']
