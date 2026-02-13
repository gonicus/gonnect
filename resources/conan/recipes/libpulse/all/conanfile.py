from conan import ConanFile
from conan.tools.system import package_manager
from conan.tools.gnu import PkgConfig
from conan.errors import ConanInvalidConfiguration

required_conan_version = ">=2.0"


class PulseConan(ConanFile):
    name = "libpulse"
    version = "system"
    description = "PulseAudio client development headers and libraries"
    topics = ("audio", "toolkit")
    homepage = "https://github.com/pulseaudio/pulseaudio"
    license = "LGPL"
    package_type = "shared-library"
    settings = "os", "arch", "compiler", "build_type"

    def package_id(self):
        self.info.clear()

    def validate(self):
        supported_os = ["Linux", "Macos", "FreeBSD"]
        if self.settings.os not in supported_os:
            raise ConanInvalidConfiguration(f"{self.ref} wraps a system package only supported by {supported_os}.")

    def system_requirements(self):
        dnf = package_manager.Dnf(self)
        dnf.install(["pulseaudio-libs-devel"], update=True, check=True)

        yum = package_manager.Yum(self)
        yum.install(["pulseaudio-libs-devel"], update=True, check=True)

        apt = package_manager.Apt(self)
        apt.install(["libpulse-dev"], update=True, check=True)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []

        self.cpp_info.set_property("cmake_file_name", "Pulse")
        self.cpp_info.set_property("cmake_target_name", "Pulse::Pulse")

        pkg_config = PkgConfig(self, 'libpulse')
        pkg_config.fill_cpp_info(self.cpp_info, is_system=True)
