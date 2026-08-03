import os
from conan import ConanFile
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, load, save, rmdir, copy, rm, chdir
from conan.tools.scm import Version
from conan.tools.layout import basic_layout

required_conan_version = ">=1.57.0"


class SnowballStemConan(ConanFile):
    name = "snowballstem"
    license = "BSD-3"
    description = "Snowball compiler and stemming algorithms"
    homepage = "https://github.com/tsyrogit/zxcvbn-c"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    def export_sources(self):
        export_conandata_patches(self)

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        basic_layout(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version],
            destination=self.source_folder, strip_root=True)

    def build(self):
        apply_conandata_patches(self)

        with chdir(self, self.source_folder):
            self.run('CXX=g++ CFLAGS="%s -fPIC" CPPFLAGS="" CXXFLAGS="" LDFLAGS="" make libstemmer.a' % (os.environ.get("CFLAGS") or ""))

    def package(self):
        copy(self, "libstemmer.h", src=os.path.join(self.source_folder, "include"), dst=os.path.join(self.package_folder, "include"), keep_path=False)
        if self.options.shared:
            copy(self, "*.so", src=self.source_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        else:
            copy(self, "*.a", src=self.source_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "SnowballStem")
        self.cpp_info.set_property("cmake_target_name", "SnowballStem::libstemmer")

        self.cpp_info.libs = ["stemmer"]
