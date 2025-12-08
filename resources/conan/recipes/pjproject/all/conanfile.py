import os
import shutil
from conan import ConanFile
from conan.tools.files import get, replace_in_file, rmdir, copy, apply_conandata_patches, export_conandata_patches, collect_libs
from conan.tools.build import cross_building
from conan.tools.layout import basic_layout
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.env import Environment, VirtualBuildEnv, VirtualRunEnv
from conan.tools.gnu import AutotoolsToolchain, AutotoolsDeps, Autotools
from conan.errors import ConanInvalidConfiguration

required_conan_version = ">=2"


class PjSIPConan(ConanFile):
    name = "pjproject"
    description = "A set of functions for use by applications that allow users to edit command lines as they are typed in"
    topics = ("sip", "communication")
    license = "GPL-2.0"
    homepage = "http://www.pjsip.org"
    url = "https://github.com/pjsip/pjproject"

    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_uuid": [True, False],
        "with_opus": [True, False],
        "with_samplerate": [True, False],
        "with_ext_sound": [True, False],
        "with_video": [True, False],
        "with_floatingpoint": [True, False],
        "endianness": ["big", "little"],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_uuid": True,
        "with_opus": True,
        "with_samplerate": False,
        "with_ext_sound": True,
        "with_video": False,
        "with_floatingpoint": True,
        "endianness": "little",
    }

    languages = "C"

    def requirements(self):
        if self.settings.os != "Macos":
            self.requires("openssl/3.5.4")

        if self.options.with_uuid:
            self.requires("libuuid/1.0.3")
        if self.options.with_samplerate:
            self.requires("libsamplerate/0.2.2")
        if self.options.with_opus:
            self.requires("opus/1.5.2")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        if self.settings.os == "Windows":
            cmake_layout(self)
        else:
            basic_layout(self, src_folder="src")

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        if self.settings.os == "Windows":
            vbe = VirtualBuildEnv(self)
            vbe.generate()
            if not cross_building(self):
                vre = VirtualRunEnv(self)
                vre.generate(scope="build")

            env = Environment()
            env.unset("VCPKG_ROOT")
            env.prepend_path("PKG_CONFIG_PATH", self.generators_folder)
            env.vars(self).save_script("conanbuildenv_pkg_config_path")

            deps = CMakeDeps(self)
            deps.generate()

            tc = CMakeToolchain(self)
    
            tc.variables['PJLIB_WITH_LIBUUID'] = self.options.with_uuid
            tc.variables['PJLIB_WITH_OPUS'] = self.options.with_opus
            tc.variables['PJLIB_WITH_FLOATING_POINT'] = self.options.with_floatingpoint
            tc.variables['PJMEDIA_HAS_VIDEO'] = "1" if self.options.with_video else "0"
            tc.variables['PJMEDIA_WITH_AUDIODEV'] = "0"
            tc.variables['BUILD_TESTING'] = False

            tc.set_property("opus", "cmake_target_name", "OPUS::OPUS")
            tc.set_property("Opus::opus", "cmake_target_name", "OPUS::OPUS")

            tc.extra_cflags.append("-DPJ_HAS_SSL_SOCK=1")
            tc.extra_cflags.append("-DPJ_HAS_IPV6=1")
    
            tc.generate()
            return

        if not cross_building(self):
            # Expose LD_LIBRARY_PATH when there are shared dependencies,
            # as configure tries to run a test executable (when not cross-building)
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        tc = AutotoolsToolchain(self)
#        if self.options.shared:
#            tc.configure_args.append("--enable-shared")
        if not self.options.with_uuid:
            tc.configure_args.append("--disable-uuid")
        if not self.options.with_opus:
            tc.configure_args.append("--disable-opus")
        else:
            tc.configure_args.append("--with-opus=%s" % self.dependencies["opus"].package_folder)
        if self.options.with_samplerate:
            tc.configure_args.append("--enable-libsamplerate")
        if not self.options.with_video:
            tc.configure_args.append("--disable-video")
        if not self.options.with_floatingpoint:
            tc.configure_args.append("--disable-floating-point")
        if self.options.with_ext_sound:
            tc.configure_args.append("--enable-ext-sound")
        if self.settings.os == "Macos":
            tc.extra_cflags.append("-DPJ_HAS_SSL_SOCK=1")
            tc.extra_cflags.append("-DPJ_SSL_SOCK_IMP=PJ_SSL_SOCK_IMP_APPLE")
            tc.extra_ldflags.append("-Wl,-framework,Security")
            tc.extra_ldflags.append("-Wl,-framework,Network")
        else:
            tc.configure_args.append("--with-ssl=%s" % self.dependencies["openssl"].package_folder)

        if cross_building(self):
            tc.configure_args.append("bash_cv_wcwidth_broken=yes")

        tc.configure_args.append("--disable-install-examples")
        tc.extra_cflags.append("-DPJ_HAS_IPV6=1")

        tc.generate()

        deps = AutotoolsDeps(self)
        deps.generate()

    def buildWindows(self):
        if self.options.shared:
            raise ConanInvalidConfiguration("Shared libraries not supported for Windows")

        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def buildAutotools(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    def build(self):
        apply_conandata_patches(self)
        shutil.copytree(self.source_folder, self.build_folder, dirs_exist_ok=True)

        if self.settings.os == "Windows":
            self.buildWindows()
        else:
            self.buildAutotools()

    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

        if self.settings.os == "Windows":            
            cmake = CMake(self)
            cmake.install()

            rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
            rmdir(self, os.path.join(self.package_folder, "share"))
            rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
            return

        autotools = Autotools(self)
        autotools.install()

        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        fix_apple_shared_install_name(self)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "pjproject")
        self.cpp_info.set_property("pkg_config_name", "libpjproject")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread", "stdc++", "rt", "m"]

        if self.options.get_safe("endianness") == "big":
            self.cpp_info.cxxflags = ['-DPJ_AUTOCONF=1', '-DPJ_IS_BIG_ENDIAN=1', '-DPJ_IS_LITTLE_ENDIAN=0', '-DPJMEDIA_HAS_RTCP_XR=1', '-DPJMEDIA_STREAM_ENABLE_XR=1']
        else:
            self.cpp_info.cxxflags = ['-DPJ_AUTOCONF=1', '-DPJ_IS_BIG_ENDIAN=0', '-DPJ_IS_LITTLE_ENDIAN=1', '-DPJMEDIA_HAS_RTCP_XR=1', '-DPJMEDIA_STREAM_ENABLE_XR=1']

        if self.settings.os == "Windows":
            self.cpp_info.libs = collect_libs(self)

        else:
            libs = []
            installed_libs = collect_libs(self)
            installed_libs.sort(key=len)

            lib_basenames = ["pjsua2", "pjsua", "pjsip-ua", "pjsip-simple", "pjsip", "pjmedia-codec", "pjmedia-videodev", "pjmedia-audiodev", "pjmedia", "ilbccodec", "srtp", "resample", "gsmcodec", "speex", "bccodec", "g7221codec", "webrtc", "pjnath", "pjlib-util", "pj"]

            for basename in lib_basenames:
                for installed in installed_libs:
                    if installed.startswith(basename + "-"):
                        libs.append(installed)
                        break

            self.cpp_info.libs = libs
