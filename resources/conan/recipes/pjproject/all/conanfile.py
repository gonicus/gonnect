import os
import shutil
from conan import ConanFile
from conan.tools.files import get, replace_in_file, rmdir, copy, apply_conandata_patches, export_conandata_patches, collect_libs
from conan.tools.build import cross_building
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.env import VirtualRunEnv
from conan.tools.gnu import AutotoolsToolchain, AutotoolsDeps, Autotools
from conan.errors import ConanInvalidConfiguration
from conan.tools.microsoft import MSBuild
from conan.tools.microsoft import MSBuildToolchain
from conan.tools.microsoft import MSBuildDeps

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
        "with_samplerate": False,
        "with_ext_sound": True,
        "with_video": False,
        "with_floatingpoint": True,
        "endianness": "little",
    }

    implements = ["auto_shared_fpic"]
    languages = "C"

    def requirements(self):
        if self.settings.os != "Macos":
            self.requires("openssl/3.5.4")

        if self.options.with_uuid:
            self.requires("libuuid/1.0.3")
        if self.options.with_samplerate:
            self.requires("libsamplerate/0.2.2")

    def layout(self):
        basic_layout(self, src_folder="src")

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        if self.settings.os == "Windows":
            bd = MSBuildDeps(self)
            if self.settings.build_type == 'Release':
                bd.configuration  = 'Release-Dynamic'
            elif self.settings.build_type == 'Debug':
                bd.configuration  = 'Debug-Dynamic'

            bd.generate()

            tc = MSBuildToolchain(self)
            tc.generate()
            return

        if not cross_building(self):
            # Expose LD_LIBRARY_PATH when there are shared dependencies,
            # as configure tries to run a test executable (when not cross-building)
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        tc = AutotoolsToolchain(self)
        if not self.options.with_uuid:
            tc.configure_args.append("--disable-uuid")
        if self.options.with_samplerate:
            tc.configure_args.append("--enable-libsamplerate")
        if not self.options.with_video:
            tc.configure_args.append("--disable-video")
        if not self.options.with_floatingpoint:
            tc.configure_args.append("--disable-floating-point")
        if self.options.with_ext_sound:
            tc.configure_args.append("--enable-ext-sound")

        if cross_building(self):
            tc.configure_args.append("bash_cv_wcwidth_broken=yes")

        tc.configure_args.append("--disable-install-examples")
        tc.extra_cflags.append("-DPJ_HAS_IPV6=1")
        tc.generate()

        deps = AutotoolsDeps(self)
        deps.generate()

    def injectConanPropsFile(self):
        search = '<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
        prop = os.path.join(self.generators_folder, 'conan_openssl.props')
        replace_in_file(self,
                        os.path.join(self.build_folder, 'build/vs/pjproject-vs14-common-config.props'),
                        search,
                        search + '\r\n<Import Project="../../conan/conan_openssl.props"/>')


    def buildWindows(self):
        if self.options.shared:
            raise ConanInvalidConfiguration("Shared libraries not supported for Windows")

        self.injectConanPropsFile()

        shutil.copy(os.path.join(self.build_folder, 'pjlib/include/pj/config_site_sample.h'),
                    os.path.join(self.build_folder, 'pjlib/include/pj/config_site.h'))
        with open(os.path.join(self.build_folder, 'pjlib/include/pj/config_site.h'), 'a') as file:
            file.write('\n\n#define PJ_HAS_SSL_SOCK 1\n')

        path = os.path.join(self.build_folder, "pjproject-vs14.sln")

        # Upgrade sln to current build system
        self.output.info('converting visual studio project if necessary')
        self.run('devenv %s /upgrade' % path, ignore_errors=True, quiet=True)

        # build pjsua
        msbuild = MSBuild(self)
        if self.settings.build_type == 'Release':
            msbuild.build_type = 'Release-Dynamic'
        elif self.settings.build_type == 'Debug':
            msbuild.build_type = 'Debug-Dynamic'
        else:
            raise ConanInvalidConfiguration("Unsupported build_type %s" % self.settings.build_type)
        msbuild.build(path, targets=["pjsua", "libspeex", "libsrtp"])

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
            for lib in ['pjlib', 'pjsip', 'pjlib-util', 'pjmedia', 'pjnath', 'third_party']:
                copy(self, "*.h", os.path.join(self.build_folder, "%s/include" % lib), os.path.join(self.package_folder, "include"))
                copy(self, "*.hpp", os.path.join(self.build_folder, "%s/include" % lib), os.path.join(self.package_folder, "include"))
                copy(self, "*.lib", os.path.join(self.build_folder, "%s/lib" % lib), os.path.join(self.package_folder, "lib"))
            return

        autotools = Autotools(self)
        autotools.install()

        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        fix_apple_shared_install_name(self)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "pjproject")
        self.cpp_info.set_property("pkg_config_name", "libpjproject")

        if self.settings.os != "Windows":
            if self.options.get_safe("endianness") == "big":
                self.cpp_info.cxxflags = ['-DPJ_AUTOCONF=1', '-DPJ_IS_BIG_ENDIAN=1', '-DPJ_IS_LITTLE_ENDIAN=0', '-DPJMEDIA_HAS_RTCP_XR=1', '-DPJMEDIA_STREAM_ENABLE_XR=1']
            else:
                self.cpp_info.cxxflags = ['-DPJ_AUTOCONF=1', '-DPJ_IS_BIG_ENDIAN=0', '-DPJ_IS_LITTLE_ENDIAN=1', '-DPJMEDIA_HAS_RTCP_XR=1', '-DPJMEDIA_STREAM_ENABLE_XR=1']

        self.cpp_info.libs = collect_libs(self)
