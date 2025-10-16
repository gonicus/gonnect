import os
import fileinput
import re
import glob
import shutil
import itertools
import configparser

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import get, apply_conandata_patches, export_conandata_patches, replace_in_file, copy, rename, rm
from conan.tools.scm import Version
from conan.tools.env import Environment, VirtualBuildEnv, VirtualRunEnv
from conan.tools.cmake import cmake_layout, CMakeDeps
from conan.tools.gnu import PkgConfigDeps
from conan.tools.build import cross_building
from conan.tools.microsoft import is_msvc, VCVars

class QtConan(ConanFile):
    name = "qt"
    description = "Qt is a cross-platform framework for graphical user interfaces."
    topics = ("conan", "qt", "ui")
    url = "https://www.gonicus.de"
    homepage = "https://www.qt.io"
    license = "LGPL-3.0"
    author = "Cajus Pollmeier <pollmeier@gonicus.de>"
    settings = "os", "arch", "compiler", "build_type"

    no_copy_source = True
    short_paths = True

    _replacer = re.compile(r"CONAN_LIB::[^ ;]+_([^_]+)_RELEASE")

    _submodules = ["qtsvg", "qtdeclarative", "qttools", "qttranslations", "qtdoc",
               "qtwayland","qtquickcontrols2", "qtquicktimeline", "qtquick3d", "qtshadertools", "qt5compat",
               "qtactiveqt", "qtcharts", "qtdatavis3d", "qtlottie", "qtscxml", "qtvirtualkeyboard",
               "qt3d", "qtimageformats", "qtnetworkauth", "qtcoap", "qtmqtt", "qtopcua",
               "qtmultimedia", "qtlocation", "qtsensors", "qtconnectivity", "qtserialbus",
               "qtserialport", "qtwebsockets", "qtwebchannel", "qtwebengine", "qtwebview",
               "qtremoteobjects", "qtpositioning", "qtlanguageserver", "qtgraphicaleffects",
               "qtspeech", "qthttpserver", "qtquick3dphysics", "qtgrpc", "qtquickeffectmaker",
               "qtquickcontrols", "qtrepotools", "qtqa", "qtgraphs"]
    _submodules_tree = None

    options = {
        "shared": [True, False],
        "commercial": [True, False],
        "opengl": ["no", "es2", "desktop", "dynamic"],
        "securetransport": [True, False],
        "openssl": [True, False],
        "with_tabletevent": [True, False],
        "with_pcre2": [True, False],
        "with_dbus": [True, False],
        "with_glib": [True, False],
        "with_doubleconversion": [True, False],
        "with_freetype": [True, False],
        "with_fontconfig": [True, False],
        "with_icu": [True, False],
        "with_harfbuzz": [True, False],
        "with_libjpeg": [True, False],
        "with_libpng": [True, False],
        "with_webp": [True, False],
        "with_jasper": [True, False],
        "with_mng": [True, False],
        "with_tiff": [True, False],
        "with_mysql": [True, False],
        "with_pq": [True, False],
        "with_odbc": [True, False],
        "with_sdl2": [True, False],
        "with_libalsa": [True, False],
        "with_openal": [True, False],
        "with_thread": [True, False],

        "GUI": [True, False],
        "widgets": [True, False],

        # "device": ["ANY"],
        "cross_compile": [True, False, None],
        "deprecated_up_to": ["ANY"],
        "sysroot": ["ANY"],
        "ltcg": [True, False],
        "reduce_exports": [True, False],
        "gc_binaries": [True, False],
        # "config": ["ANY"],
        "multiconfiguration": [True, False],
    }
    options.update({module: [True, False] for module in _submodules})

    default_options = {
        "shared": True,
        "commercial": False,
        "opengl": "desktop",
        "openssl": True,
        "securetransport": False,
        "with_pcre2": True,
        "with_glib": True,
        "with_tabletevent": True,
        "with_doubleconversion": True,
        "with_freetype": True,
        "with_fontconfig": True,
        "with_icu": True,
        "with_harfbuzz": True,
        "with_libjpeg": True,
        "with_libpng": True,
        "with_webp": True,
        "with_jasper": False,
        "with_mng": False,
        "with_dbus": False,
        "with_tiff": False,
        "with_mysql": True,
        "with_pq": True,
        "with_odbc": True,
        "with_sdl2": True,
        "with_libalsa": True,
        "with_openal": True,
        "with_thread": True,
        "cross_compile": False,
        "deprecated_up_to": "0x060800",

        "GUI": True,
        "widgets": True,

        # "device": None,
        "sysroot": None,
        "ltcg": False,
        "reduce_exports": False,
        "gc_binaries": False,
        # "config": None,
        "multiconfiguration": False,
    }
    default_options.update({module: False for module in _submodules})

    @property
    def _get_module_tree(self):
        if self._submodules_tree:
            return self._submodules_tree
        config = configparser.ConfigParser()
        config.read(os.path.join(self.recipe_folder, f"qtmodules-{self.version}.conf"))
        self._submodules_tree = {}
        assert config.sections(), f"no qtmodules.conf file for version {self.version}"
        for s in config.sections():
            section = str(s)
            assert section.startswith("submodule ")
            assert section.count('"') == 2
            modulename = section[section.find('"') + 1: section.rfind('"')]
            status = str(config.get(section, "status"))
            if status not in ["obsolete", "ignore", "additionalLibrary"]:
                self._submodules_tree[modulename] = {"status": status,
                                "path": str(config.get(section, "path")), "depends": []}
                if config.has_option(section, "depends"):
                    self._submodules_tree[modulename]["depends"] = [str(i) for i in config.get(section, "depends").split()]

        for m in self._submodules_tree:
            assert m in ["qtbase", "qtqa", "qtrepotools"] or m in self._submodules, f"module {m} not in self._submodules"

        return self._submodules_tree

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

        cmake = CMakeDeps(self)
        cmake.generate()

        pc = PkgConfigDeps(self)
        pc.generate()

        if self.settings.os == "Android":
            env = Environment()
            env.define("ANDROID_API_VERSION", "android-29")
            env.vars(self, scope="build").save_script("conanbuild_android")

        ms = VCVars(self)
        ms.generate()

    def export_sources(self):
        export_conandata_patches(self)

    def layout(self):
        cmake_layout(self, src_folder="src")

    def build_requirements(self):
        if is_msvc(self):
            self.build_requires("jom_installer/1.1.2")
        if self.settings.os == 'Linux':
            if not self.conf.get("tools.gnu:pkg_config", default=False, check_type=str):
                self.build_requires('pkgconf/2.2.0')
        if self.options.qtwayland:
            self.tool_requires("wayland/1.22.0")

    def config_options(self):
        if self.settings.os != "Linux":
            self.options.with_icu = False

        for m in self._submodules:
            if m not in self._get_module_tree:
                delattr(self.options, m)

        if self.settings.os != 'Linux':
            self.options.with_glib = False
            self.options.with_fontconfig = False
        if self.settings.compiler == "gcc" and Version(self.settings.compiler.version.value) < "5.3":
            self.options.with_mysql = False
        if self.settings.os == "Windows":
            self.options.with_mysql = False
            if not self.options.shared and self.options.with_icu:
                raise ConanInvalidConfiguration("icu option is not supported on windows in static build. see QTBUG-77120.")

        if self.options.widgets and not self.options.GUI:
            raise ConanInvalidConfiguration("using option qt:widgets without option qt:GUI is not possible. "
                                            "You can either disable qt:widgets or enable qt:GUI")
        if not self.options.GUI:
            self.options.opengl = "no"
            self.options.with_freetype = False
            self.options.with_fontconfig = False
            self.options.with_harfbuzz = False
            self.options.with_libjpeg = False
            self.options.with_libpng = False
            self.options.with_webp = False

        if not self.options.qtmultimedia:
            self.options.with_libalsa = False
            self.options.with_openal = False

        if self.settings.os != "Linux":
            self.options.with_libalsa = False

        def _enablemodule(mod):
            if mod != "qtbase":
                setattr(self.options, mod, True)
            for req in self._get_module_tree[mod]["depends"]:
                _enablemodule(req)

        for module in self._get_module_tree:
            if self.options.get_safe(module):
                _enablemodule(module)

    def configure(self):
        if self.settings.os == "Android" and self.options.opengl == "desktop":
            raise ConanInvalidConfiguration("OpenGL desktop is not supported on Android. Consider using OpenGL es2")

        if self.settings.os != "Windows" and self.options.opengl == "dynamic":
            raise ConanInvalidConfiguration("Dynamic OpenGL is supported only on Windows.")

        if (self.settings.os != "Macos" and self.settings.os != "iOS") and self.options.securetransport:
            raise ConanInvalidConfiguration("securetransport is only supported on iOS and Macos")

        if self.options.securetransport and self.options.openssl:
            raise ConanInvalidConfiguration("openssl cannot be used with securetransport")

        if self.settings.os == "Macos":
            del self.settings.os.version

        if self.options.multiconfiguration:
            del self.settings.build_type

        if not self.options.with_doubleconversion and str(self.settings.compiler.libcxx) != "libc++":
            raise ConanInvalidConfiguration('Qt without libc++ needs qt:with_doubleconversion. '
                                            'Either enable qt:with_doubleconversion or switch to libc++')

    def requirements(self):
        self.requires("zlib/1.3.1")

        if self.settings.os != "Emscripten":
            self.requires("libcurl/8.15.0") # Workaround for not having cmake build for the host platform
        if self.options.openssl:
            self.requires("openssl/3.5.4")
        if self.options.with_pcre2:
            self.requires("pcre2/10.42")
        if self.options.with_glib:
            self.requires("glib/2.85.3")
            self.dependencies["glib"].options.shared = True
        if self.options.with_doubleconversion and not self.options.multiconfiguration:
            self.requires("double-conversion/3.3.0")
        if self.options.with_freetype and not self.options.multiconfiguration:
            self.requires("freetype/2.13.2")
        if self.options.with_fontconfig:
            self.requires("fontconfig/2.15.0")
        if self.options.qtwayland:
            self.requires("wayland/1.22.0")
        if self.options.get_safe("qtwebengine"):
            self.requires("expat/[>=2.6.2 <3]")
            self.requires("opus/1.4")
            self.requires("xorg-proto/2022.2")
            self.requires("libxshmfence/1.3")
            self.requires("nss/3.93")
            if self.settings.os == "Linux":
                self.requires("libdrm/2.4.119")
        if self.options.with_dbus:
            self.requires("dbus/1.15.8")
        if self.options.with_icu:
            self.requires("icu/77.1")
        if self.options.with_harfbuzz and not self.options.multiconfiguration:
            self.requires("harfbuzz/11.4.1")
        if self.options.with_libjpeg and not self.options.multiconfiguration:
            self.requires("libjpeg-turbo/3.1.1")
        if self.options.with_webp and not self.options.multiconfiguration:
            self.requires("libwebp/1.5.0")
        if self.options.with_libpng and not self.options.multiconfiguration:
            self.requires("libpng/1.6.50")
        if self.options.with_mysql:
            self.requires("libmysqlclient/8.1.0")
        if self.options.with_pq:
            self.requires("libpq/17.5")
        if self.options.with_odbc:
            if self.settings.os != "Windows":
                self.requires("odbc/2.3.11")
        if self.options.with_sdl2:
            self.requires("sdl/3.2.20")
        if self.options.with_openal:
            self.requires("openal/1.22.2")
        if self.options.with_libalsa:
            self.requires("libalsa/1.2.7.2")
        if self.options.GUI:
            if self.settings.os == "Linux" and not cross_building(self, skip_x64_x86=True):
                self.requires("xorg/system")
                self.requires("egl/system")
                self.requires("xkbcommon/1.6.0")

    def export_sources(self):
        export_conandata_patches(self)

    def export(self):
        copy(self, f"qtmodules-{self.version}.conf", self.recipe_folder, self.export_folder)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def _xplatform(self):
        if self.settings.os == "Emscripten":
            return "wasm-emscripten"

        elif self.settings.os == "Linux":
            if 'YOCTO' in os.environ:
                return "linux-oe-g++"
            if self.settings.compiler == "gcc":
                return {"x86": "linux-g++-32",
                        "armv6": "linux-arm-gnueabi-g++",
                        "armv7": "linux-arm-gnueabi-g++",
                        "armv7hf": "linux-arm-gnueabi-g++",
                        "armv8": "linux-aarch64-gnu-g++"}.get(str(self.settings.arch), "linux-g++")
            elif self.settings.compiler == "clang":
                if self.settings.arch == "x86":
                    return "linux-clang-libc++-32" if self.settings.compiler.libcxx == "libc++" else "linux-clang-32"
                elif self.settings.arch == "x86_64":
                    return "linux-clang-libc++" if self.settings.compiler.libcxx == "libc++" else "linux-clang"

        elif self.settings.os == "Macos":
            return {"clang": "macx-clang",
                    "apple-clang": "macx-clang",
                    "gcc": "macx-g++"}.get(str(self.settings.compiler))

        elif self.settings.os == "iOS":
            if self.settings.compiler == "apple-clang":
                return "macx-ios-clang"

        elif self.settings.os == "watchOS":
            if self.settings.compiler == "apple-clang":
                return "macx-watchos-clang"

        elif self.settings.os == "tvOS":
            if self.settings.compiler == "apple-clang":
                return "macx-tvos-clang"

        elif self.settings.os == "Android":
            return {"clang": "android-clang",
                    "gcc": "android-g++"}.get(str(self.settings.compiler))

        elif self.settings.os == "Windows":
            return {"Visual Studio": "win32-msvc",
                    "gcc": "win32-g++",
                    "clang": "win32-clang-g++"}.get(str(self.settings.compiler))

        elif self.settings.os == "WindowsStore":
            if is_msvc(self):
                return {"14": {"armv7": "winrt-arm-msvc2015",
                               "x86": "winrt-x86-msvc2015",
                               "x86_64": "winrt-x64-msvc2015"},
                        "15": {"armv7": "winrt-arm-msvc2017",
                               "x86": "winrt-x86-msvc2017",
                               "x86_64": "winrt-x64-msvc2017"}
                        }.get(str(self.settings.compiler.version)).get(str(self.settings.arch))

        elif self.settings.os == "FreeBSD":
            return {"clang": "freebsd-clang",
                    "gcc": "freebsd-g++"}.get(str(self.settings.compiler))

        elif self.settings.os == "SunOS":
            if self.settings.compiler == "sun-cc":
                if self.settings.arch == "sparc":
                    return "solaris-cc-stlport" if self.settings.compiler.libcxx == "libstlport" else "solaris-cc"
                elif self.settings.arch == "sparcv9":
                    return "solaris-cc64-stlport" if self.settings.compiler.libcxx == "libstlport" else "solaris-cc64"
            elif self.settings.compiler == "gcc":
                return {"sparc": "solaris-g++",
                        "sparcv9": "solaris-g++-64"}.get(str(self.settings.arch))
        elif self.settings.os == "Neutrino" and self.settings.compiler == "qcc":
            return {"armv8": "qnx-aarch64le-qcc",
                    "armv8.3": "qnx-aarch64le-qcc",
                    "armv7": "qnx-armle-v7-qcc",
                    "armv7hf": "qnx-armle-v7-qcc",
                    "armv7s": "qnx-armle-v7-qcc",
                    "armv7k": "qnx-armle-v7-qcc",
                    "x86": "qnx-x86-qcc",
                    "x86_64": "qnx-x86-64-qcc"}.get(str(self.settings.arch))

        return None

    def build(self):
        apply_conandata_patches(self)

        for f in glob.glob("*.cmake"):
            replace_in_file(self, f,
                "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:>",
                "", strict=False)
            replace_in_file(self, f,
                "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:>",
                "", strict=False)
            replace_in_file(self, f,
                "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:>",
                "", strict=False)
            replace_in_file(self, f,
                "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:-Wl,--export-dynamic>",
                "", strict=False)
            replace_in_file(self, f,
                "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:-Wl,--export-dynamic>",
                "", strict=False)
            replace_in_file(self, f,
                " IMPORTED)\n",
                " IMPORTED GLOBAL)\n", strict=False)
        args = ["-confirm-license", "-silent", "-nomake examples", "-nomake tests", "-optimized-tools",
                "-prefix %s" % self.package_folder, "-extprefix %s" % self.package_folder ]

        if self.options.commercial:
            args.append("-commercial")
        else:
            args.append("-opensource")
        if not self.options.GUI:
            args.append("-no-gui")
        if not self.options.widgets:
            args.append("-no-widgets")
        if not self.options.with_tabletevent:
            args.append("-no-feature-tabletevent")
        if not self.options.shared:
            args.insert(0, "-static")
            if is_msvc(self):
                if self.settings.compiler.runtime == "MT" or self.settings.compiler.runtime == "MTd":
                    args.append("-static-runtime")
        else:
            args.insert(0, "-shared")
        if self.options.multiconfiguration:
            args.append("-debug-and-release")
        elif self.settings.build_type == "Debug":
            args.append("-debug")
        elif self.settings.build_type == "Release":
            args.append("-release")
        elif self.settings.build_type == "RelWithDebInfo":
            args.append("-release")
            args.append("-force-debug-info")
        elif self.settings.build_type == "MinSizeRel":
            args.append("-release")
            args.append("-optimize-size")

        if self.settings.os == "iOS":
            args.append("-sdk iphoneos")
            args.append("-no-pch")

        for module in self._submodules:
            if not self.options.get_safe(module):
                args.append("-skip " + module)

        args.append("--zlib=system")
        args.append("-disable-deprecated-up-to " + str(self.options.deprecated_up_to))

        # openGL
        if self.options.opengl == "no":
            args += ["-no-opengl"]
        elif self.options.opengl == "es2":
            args += ["-opengl es2"]
        elif self.options.opengl == "desktop":
            args += ["-opengl desktop"]
        elif self.options.opengl == "dynamic":
            args += ["-opengl dynamic"]

        # securetransport
        if self.options.securetransport:
            args += ["-securetransport"]

        # openSSL
        if not self.options.openssl:
            args += ["-no-openssl"]
        else:
            if self.dependencies["openssl"].options.shared:
                args += ["-openssl-runtime"]
            else:
                args += ["-openssl-linked"]

        args.append("--glib=" + ("yes" if self.options.with_glib else "no"))
        args.append("--pcre=" + ("system" if self.options.with_pcre2 else "qt"))
        args.append("--fontconfig=" + ("yes" if self.options.with_fontconfig else "no"))

        if self.settings.os == "Android":
            args.append("-no-warnings-are-errors")

        args.append("--icu=" + ("yes" if self.options.with_icu else "no"))
        args.append("--sql-mysql=" + ("yes" if self.options.with_mysql else "no"))
        args.append("--sql-psql=" + ("yes" if self.options.with_pq else "no"))
        args.append("--sql-odbc=" + ("yes" if self.options.with_odbc else "no"))

        if self.options.qtmultimedia:
            args.append("--alsa=" + ("yes" if self.options.with_libalsa else "no"))

        if self.options.with_thread:
            args.append("--feature-thread")
        else:
            args.append("--no-feature-thread")

        args.append("-no-feature-printsupport")

        for opt, conf_arg in [
                              ("with_doubleconversion", "doubleconversion"),
                              ("with_freetype", "freetype"),
                              ("with_harfbuzz", "harfbuzz"),
                              ("with_libjpeg", "libjpeg"),
                              ("with_libpng", "libpng"),
                              ("with_webp", "webp"),
                              ("with_tiff", "tiff"),
                              ("with_mng", "mng"),
                              ("with_jasper", "jasper")]:
            if getattr(self.options, opt):
                if self.options.multiconfiguration:
                    args += ["-qt-" + conf_arg]
                else:
                    args += ["-system-" + conf_arg]
            else:
                args += ["-no-" + conf_arg]

        if 'mysql-connector' in self.dependencies:
            args.append("-mysql_config " + os.path.join(self.dependencies['mysql-connector'].cpp_info.rootpath, "bin", "mysql_config"))
        if 'libpq' in self.dependencies:
            args.append("-psql_config " + os.path.join(self.dependencies['libpq'].cpp_info.rootpath, "bin", "pg_config"))
        if self.settings.os == "Macos":
            args += ["-no-framework"]
        elif self.settings.os == "Android":
            abi = {"x86": "x86",
                   "x86_64": "x86_64",
                   "armv6": "armeabi-v7a",
                   "armv7": "armeabi-v7a",
                   "armv7hf": "armeabi-v7a",
                   "armv8": "arm64-v8a"}.get(str(self.settings.arch))

            args += ["-android-abis %s" % abi]
            args += ["-android-sdk $ANDROID_SDK_ROOT"]
            args += ["-android-ndk $ANDROID_NDK_ROOT"]
            args += ["-android-ndk-platform android-%s" % self.settings.os.api_level]
            args += ["-xplatform %s" % self._xplatform()]

        if self.options.sysroot:
            args += ["-sysroot %s" % self.options.sysroot]
            #if "CXXFLAGS" in os.environ:
            #    args += ['QMAKE_CXXFLAGS+="%s"' % os.environ.get("CXXFLAGS")]
            #if "CFLAGS" in os.environ:
            #    args += ['QMAKE_CFLAGS+="%s"' % os.environ.get("CFLAGS")]

        if self.options.cross_compile:
            args += ["-qt-host-path %s" % os.environ.get("QT_HOST_PATH"), "-skip qttools", "-skip qttranslations"]

            xplatform_val = self._xplatform()
            if xplatform_val:
                if self.settings.os == "Emscripten" :
                    args += ["-platform %s" % xplatform_val]
                elif self.settings.os == "Windows":
                    args += ["-platform %s" % xplatform_val]
                elif not cross_building(self, skip_x64_x86=True):
                    args += ["-platform %s" % xplatform_val]
                elif self.settings.os == "Macos":
                    args += ["-platform %s" % xplatform_val]
                else:
                    args += ["-platform %s" % xplatform_val]
            else:
                self.output.warn("host not supported: %s %s %s %s" %
                                 (self.settings.os, self.settings.compiler,
                                  self.settings.compiler.version, self.settings.arch))

        if self.options.get_safe("qtwebengine") and self.settings.os == "Linux":
            if not self.options.shared:
                raise ConanInvalidConfiguration("Static builds of Qt WebEngine are not supported")
            if not self.options.with_dbus:
                raise ConanInvalidConfiguration("option qt:webengine requires also qt:with_dbus on Linux")

            args += ["-no-webengine-alsa"]
            args += ["-no-webengine-pulseaudio"]
            args += ["-no-webengine-icu"]
            args += ["-no-webengine-ffmpeg"]
            args += ["-no-webengine-opus"]
            args += ["-no-webengine-pepper-plugins"]
            args += ["-no-webengine-printing-and-pdf"]
            args += ["-no-webengine-proprietary-codecs"]
            args += ["-no-webengine-spellchecker"]
            args += ["-no-webengine-webrtc"]

        self.run("%s/configure %s -- -DFEATURE_optimize_full=ON %s %s -DCMAKE_PREFIX_PATH=%s -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_MODULE_PATH=%s" % (self.source_folder, " ".join(args), "-DQT_FEATURE_openssl_linked=ON" if self.options.openssl else "", "-DCMAKE_OSX_DEPLOYMENT_TARGET=16.7" if self.settings.os == "iOS" else "", self.generators_folder, self.generators_folder), env="conanbuild")
        self.run("cmake --build . --parallel", env="conanbuild")
        self.run("cmake --install .")

        with open('qtbase/bin/qt.conf', 'w') as f:
            f.write('[Paths]\nPrefix = ..')

        if self.settings.os != "Windows" and self.settings.os != "WindowsStore":

            # For some reason I don't understand in the moment, some dependencies are not placed in the prl file. So - do that
            # manually...
            zlib_libdirs = ["-L{}".format(it) for it in self.dependencies["zlib"].cpp_info.aggregated_components().libdirs]
            pcre2_libdirs = ["-L{}".format(it) for it in self.dependencies["pcre2"].cpp_info.aggregated_components().libdirs]
            if self.options.with_harfbuzz:
                harfbuzz_libdirs = ["-L{}".format(it) for it in self.dependencies["harfbuzz"].cpp_info.aggregated_components().libdirs]
            dc_libdirs = ["-L{}".format(it) for it in self.dependencies["double-conversion"].cpp_info.aggregated_components().libdirs]
            freetype_libdirs = []
            freetype_libs = []
            freetype_libdirs.extend(["-L{}".format(it) for it in self.dependencies["freetype"].cpp_info.aggregated_components().libdirs])
            for lib, _ in self.dependencies["freetype"].cpp_info.required_components:
                freetype_libdirs.extend(["-L{}".format(it) for it in self.dependencies[lib].cpp_info.aggregated_components().libdirs])
                freetype_libs.extend(["-l{}".format(it) for it in self.dependencies[lib].cpp_info.aggregated_components().libs])

            # Because Qt gets compiled with paths to dependencies compiled into a smattering
            # of files, we'll have to find these paths and replace them with a HOME placeholder.
            # This only works if conan is in $HOME/.conan, though.
            user_conan_home = "${CONAN_HOME:-$HOME/.conan2}"
            paths_to_replace = ["/home/dev"]
            extensions = (".h", ".prl", ".pri", ".pc", ".la")
            files_to_fix = []
            for root, dirs, files in os.walk(self.package_folder):
                files_to_fix += [os.path.join(root, f) for f in files if f.endswith(extensions)]

            for f in files_to_fix:
                for p in paths_to_replace:
                    replace_in_file(self, f, p, user_conan_home, strict=False)

                if f.endswith(".pri") or f.endswith(".prl"):
                    rename(self, f, f + ".orig")

                    with open(f, 'a') as of:
                        with open(f + ".orig", 'r', encoding='UTF-8') as file:
                            while line := file.readline():
                                line = line.rstrip()
                                of.write(self._replacer.sub(r"\1", line) + '\n')

                    os.unlink(f + ".orig")

                    replace_in_file(self, f, " -lfreetype ", " -lfreetype %s %s " % (" ".join(freetype_libdirs), " ".join(freetype_libs)), strict=False)
                    if self.options.with_harfbuzz:
                        replace_in_file(self, f, " -lharfbuzz ", " -lharfbuzz %s " % (" ".join(harfbuzz_libdirs)), strict=False)

                    replace_in_file(self, f, " -lz ", " -lz %s " % (" ".join(zlib_libdirs)), strict=False)
                    replace_in_file(self, f, " -lpcre2-16 ", " -lpcre2-16 %s " % (" ".join(pcre2_libdirs)), strict=False)
                    replace_in_file(self, f, " -ldouble-conversion ", " -ldouble-conversion %s " % (" ".join(dc_libdirs)), strict=False)

    def package(self):
        copy(self, "LICENSE.QT-LICENSE-AGREEMENT", dst="licenses", src=self.source_folder)
        #copy(self, "bin/qt.conf", dst=os.path.join(self.package_folder), src="qtbase")
        #copy(self, "mkspecs/qconfig.pri", dst=os.path.join(self.package_folder), src="qtbase")

    def package_id(self):
        # Backwards compatibility for cross_compile to not affect package_id
        # for people who don't use the `device` option
        self.info.options.cross_compile = None
        del self.info.options.sysroot

    def package_info(self):
        self.env_info.CMAKE_PREFIX_PATH.append(self.package_folder)
