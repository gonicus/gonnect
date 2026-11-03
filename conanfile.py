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
        # QT Version marker
        #self.requires("qt/6.12.0");

        self.requires("hidapi/0.15.0")
        self.requires("pjproject/2.17")
        self.requires("openldap/2.7.2")
        self.requires("libical/3.0.20")
        self.requires("vcard/cci.20250408")
        self.requires("logfault/0.8.1-1")
        #self.requires("hunspell/1.7.2")
        self.requires("snowballstem/3.1.1")
        self.requires("cmark/0.31.2")
        self.requires("ksyntaxhighlighting/6.26.0")
        self.requires("icu/78.2")

        if self.settings.os != "Windows":
            self.requires("libuuid/1.0.3")

        if self.settings.os == "Windows":
            self.requires("wintoast/1.3.2")
            self.requires("qhotkey/2026-03-19")

        if self.settings.os == "Linux":
            self.requires("libnotify/system")
            self.requires("libpulse/system")
            self.requires("xorg/system")
            self.requires("wayland/1.24.0", override=True)

        self.requires("openssl/3.6.3", override=True)
        self.requires("qtkeychain/0.17.0")
        self.requires("qcoro6/0.13.0")
        self.requires("libusb/1.0.30")
        self.requires("sqlcipher/4.17.0")

    def build_requirements(self):
        if not self.conf.get("tools.gnu:pkg_config", check_type=str):
            self.tool_requires("pkgconf/2.5.1")

    def configure(self):
        self.options["*/*"].shared = False
        self.options["openldap/*"].with_cyrus_sasl=False
        self.options["harfbuzz/*"].with_subset=True

        if self.settings.os == "Windows":
            self.options["pjproject/*"].with_uuid=False
