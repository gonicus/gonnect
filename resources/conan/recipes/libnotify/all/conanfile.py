from conan import ConanFile
from conan.tools.system import package_manager
from conan.tools.gnu import PkgConfig
from conan.errors import ConanInvalidConfiguration

required_conan_version = ">=2.0"


class LibNotifyConan(ConanFile):
    name = "libnotify"
    version = "system"
    description = "Sends desktop notifications to a notification daemon"
    topics = ("ui", "notifications", "toolkit")
    homepage = "https://invisible-mirror.net/archives/ncurses/"
    license = "MIT"
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
        dnf.install(["libnotify-devel"], update=True, check=True)

        yum = package_manager.Yum(self)
        yum.install(["libnotify-devel"], update=True, check=True)

        apt = package_manager.Apt(self)
        apt.install(["libnotify-dev"], update=True, check=True)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []

        self.cpp_info.set_property("cmake_file_name", "LibNotify")
        self.cpp_info.set_property("cmake_target_name", "LibNotify::LibNotify")

        pkg_config = PkgConfig(self, 'libnotify')
        pkg_config.fill_cpp_info(self.cpp_info, is_system=True)
