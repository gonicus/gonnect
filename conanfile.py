import os
from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.gnu import PkgConfigDeps
from conan.tools.build import can_run
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv, Environment


class GOnnectRecipe(ConanFile):
    name = "gonnect"
    version = "1.0"
    package_type = "application"

    license = "MIT"
    author = "Cajus Pollmeier <pollmeier@gonicus.de>"
    url = "https://github.com/gonicus/gonnect"
    description = "GOnnect is a simple, easy to use UC client"
    topics = ("communication", "VoIP", "SIP")

    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*"

    options = {
        "with_conan_qt": [True, False],
    }
    default_options = {
        "with_conan_qt": False,
    }

    def layout(self):
        cmake_layout(self)

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        if can_run(self):
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        deps = CMakeDeps(self)
        deps.generate()

        pc = PkgConfigDeps(self)
        pc.generate()
        pkg_lib_folder = os.path.join(self.generators_folder, "lib", "pkgconfig")
        copy(self, "*.pc", self.generators_folder, pkg_lib_folder)

        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def requirements(self):
        self.requires("hidapi/0.15.0")
        self.requires("pjproject/2.15.1")
        self.requires("openldap/2.6.10")
        self.requires("libical/3.0.20")
        self.requires("vcard/cci.20250408")

        if self.settings.os != "Windows":
            self.requires("libuuid/1.0.3")

        if self.settings.os == "Windows":
            self.requires("wintoast/1.3.2")

        if self.settings.os == "Linux":
            self.requires("libnotify/system")
            self.requires("libpulse/system")

        if self.options.with_conan_qt:
            self.requires("qt/6.10.0")
            self.requires("openssl/3.5.4", override=True)
            self.requires("mpg123/1.33.0", override=True)
            self.requires("wayland/1.23.92", override=True)

        self.requires("qca/2.3.10")
        self.requires("qtwebdav/3.19.0")
        self.requires("qtkeychain/0.15.0")
        self.requires("libusb/1.0.26")

    def build_requirements(self):
        if not self.conf.get("tools.gnu:pkg_config", check_type=str):
            self.tool_requires("pkgconf/2.0.3")

    def configure(self):
        self.options["*/*"].shared = False
        self.options["openldap/*"].with_cyrus_sasl=False
        self.options["harfbuzz/*"].with_subset=True

        if self.settings.os == "Windows":
            self.options["pjproject/*"].with_uuid=False

        if self.options.with_conan_qt:
            self.options["*/*"].with_conan_qt=True
            self.options["qt/*"].with_mysql=False
            self.options["qt/*"].with_glib=False
            self.options["qt/*"].with_pq=False
            self.options["qt/*"].with_odbc=False
            self.options["qt/*"].with_openal=False
            self.options["qt/*"].qtconnectivity=True
            self.options["qt/*"].qtsvg=True
            self.options["qt/*"].qtdeclarative=True
            self.options["qt/*"].qtactiveqt=False
            self.options["qt/*"].qtmultimedia=True
            self.options["qt/*"].qttools=True
            self.options["qt/*"].qttranslations=True
            self.options["qt/*"].qtshadertools=True
            self.options["qt/*"].qtdoc=False
            self.options["qt/*"].qtrepotools=False
            self.options["qt/*"].qtqa=False
            self.options["qt/*"].qtlocation=False
            self.options["qt/*"].qtsensors=False
            self.options["qt/*"].qt5compat=True
            self.options["qt/*"].qtcoap=False
            self.options["qt/*"].qtopcua=False
            self.options["qt/*"].qtpositioning=False
            self.options["qt/*"].qtquick3d=False
            self.options["qt/*"].qtquicktimeline=False
            self.options["qt/*"].qt3d=False
            self.options["qt/*"].qtimageformats=True
            self.options["qt/*"].qtserialbus=False
            self.options["qt/*"].qtserialport=False
            self.options["qt/*"].qtwebsockets=True
            self.options["qt/*"].qtwebchannel=True
            self.options["qt/*"].qtwebengine=True
            self.options["qt/*"].qtnetworkauth=True
            self.options["qt/*"].qthttpserver=True
            self.options["qt/*"].qtscxml=False
            self.options["qt/*"].qtmqtt=False
            self.options["qt/*"].qtwebview=False
            self.options["qt/*"].qtcharts=False
            self.options["qt/*"].qtdatavis3d=False
            self.options["qt/*"].qtvirtualkeyboard=False
            self.options["qt/*"].qtremoteobjects=False
            self.options["qt/*"].qtlottie=False
            self.options["qt/*"].qtquick3dphysics=False
            self.options["qt/*"].qtspeech=False
            self.options["qt/*"].qtgrpc=False
            self.options["qt/*"].qtquickeffectmaker=False
            self.options["qt/*"].qtgraphs=False
            self.options["qt/*"].qtwayland=False

            if self.settings.os == "Linux":
                self.options["qt/*"].with_dbus=True
                self.options["qt/*"].with_fontconfig=True
                self.options["qt/*"].qtwayland=False
            else:
                self.options["qt/*"].with_dbus=False

            if self.settings.os == "Macos":
                self.options["qt/*"].with_harfbuzz=False # TODO: why? cmake info export for harfbuzz != both?
