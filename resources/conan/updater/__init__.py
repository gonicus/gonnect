import os
import yaml
import difflib
from pathlib import Path
from typing import Optional
from .logger import create_logger
from .releases import *

ROOT_DIR = Path(__file__).parent.parent.parent.parent.resolve()

class CompareError(Exception):
    pass
class CompareWarning(Exception):
    pass

def diff(a, b, fromfile='Original', tofile='Current', fromfiledate='', tofiledate='', n=3, lineterm='\n'):
    return "\n".join(difflib.unified_diff(a.splitlines(), b.splitlines(), fromfile, tofile, fromfiledate, tofiledate, n, lineterm))

class Dependency:
    def __init__(self, name, content: dict, recipes_dir, suffix=""):
        self._name = name
        self._content = content
        self.current_release = None
        self.latest_release = None
        self._old_version = None
        self._new_version = None
        self._conandata_path = None
        self._conanfile_path = None
        self._package_suffix = suffix
        self._platforms = {}
        self.logger = create_logger("pkg."+name)
        versions = list(self._content["versions"].keys())
        self._recipe_dir = recipes_dir.joinpath(self._name)
        self._config_path = self._recipe_dir.joinpath('config.yml')
        self.blocked = False
        if len(versions) == 1 and versions[0] == "system":
            self.blocked = True
        else:
            releases = map(lambda x: Release(x, content["updater"], name=self._name, logger=self.logger), versions)
            # sort by version descending
            releases = sorted(list(releases), reverse=True)
            self.current_release = releases[0]
            self._old_version = self.current_release.version
            self._new_version = self.current_release.version
            conandata = self.read_conandata()

            version_data = conandata["sources"][self.current_release.version]
            if "url" in version_data:
                self._platforms["all"] = {"all": version_data}
            elif "Linux" in version_data or "Macos" in version_data:
                for platform in version_data.keys():
                    if not platform in self._platforms:
                        self._platforms[platform] = {}
                    if "url" in version_data[platform]:
                        self._platforms[platform]["all"] = version_data[platform]
                    else:
                        for architecture in version_data[platform].keys():
                            self._platforms[platform][architecture] = version_data[platform][architecture]


        # this dependency has been updated
        self.has_update = False
        # another dependency that is used be this one has been updated
        self.dependencies_updated = []
        # manually marked for updating build number
        self._bump_build = False

    def __str__(self):
        if self._new_version != self._old_version:
            return f"Dependency({self._name} [{self._new_version}])"
        return f"Dependency({self._name} [{self._old_version}])"

    def __repr__(self):
        return str(self)

    @property
    def name(self):
        return self._name

    @property
    def package(self):
        return self.conan_version()

    def conan_version(self, version=None, latest=False, old=False, new=False):
        if version is not None:
            return f"{self._name}/{version}{self._package_suffix}"
        if old:
            return f"{self._name}/{self._old_version}{self._package_suffix}"
        if new:
            return f"{self._name}/{self._new_version}{self._package_suffix}"
        if latest:
            return f"{self._name}/{self.latest_release.version}{self._package_suffix}"

        return f"{self._name}/{self.current_release.version}{self._package_suffix}"

    @property
    def changed(self):
        return self.has_update or len(self.dependencies_updated) > 0 or self._bump_build

    def add_updated_dependency(self, other):
        if other not in self.dependencies_updated:
            self.dependencies_updated.append(other)
            return True
        return False

    def flush(self, dry_run=False) -> Optional[tuple[str, str]]:
        if not self.has_update and len(self.dependencies_updated) == 0 and self._bump_build is False:
            return None

        current = self.current_release
        conandata = self.read_conandata()
        old_version = self.current_release.version
        new_version = None
        if self.has_update:
            for platform in self._platforms.keys():
                for arch in self._platforms[platform].keys():
                    url = self.latest_release.get_download_url(platform, arch)

                    if url is None:
                        # replace version number in url of existing release as fallback
                        #url = conandata["sources"][self.current_release.version]["url"].replace(self.current_release.version, self.latest_release.version)
                        raise Exception(f"{self._name} no url found for {platform}:{arch}")

                    r = requests.head(url)
                    if r.status_code >= 400:
                        self.logger.error(f"invalid download URL: {url}, {r.status_code}")
                        continue

                    if not dry_run:
                        # download sources and calculate sha256
                        self.logger.debug(f"creating hash for {url}")
                        hash = sha256(url)
                        if hash is None:
                            # could not create hash, URL might be incorrect
                            raise Exception(f"update error for {self._name}")
                    else:
                        hash = "<not-calculated-in-dry-run>"

                    if platform == "all" and arch == "all":
                        # add latest release
                        conandata["sources"][self.latest_release.version] = {
                            "url": url,
                            "sha256": hash
                        }
                    else:
                        if self.latest_release.version not in conandata["sources"]:
                            conandata["sources"][self.latest_release.version] = {}
                        if platform not in conandata["sources"][self.latest_release.version]:
                            conandata["sources"][self.latest_release.version][platform] = {}

                        conandata["sources"][self.latest_release.version][platform][arch] = {
                            "url": url,
                            "sha256": hash
                        }

            current = self.latest_release
            new_version = current.version
            # delete current release
            del conandata["sources"][self.current_release.version]
            if "patches" in conandata and self.current_release.version in conandata["patches"]:
                # copy patches to new release
                conandata["patches"][self.latest_release.version] = conandata["patches"][self.current_release.version]
                del conandata["patches"][self.current_release.version]

        if not self.has_update and (len(self.dependencies_updated) > 0 or self._bump_build is True):
            old = current.version
            current.update_buildno()
            new_version = current.version
            for key in conandata:
                if old in conandata[key]:
                    conandata[key][new_version] = conandata[key][old]
                    del conandata[key][old]
            if self._bump_build is True:
                self.logger.debug(f"manually increasing build number")
            else:
                self.logger.debug(f"increasing build number because dependencies have changed: {self.dependencies_updated}")

        if new_version is not None:
            self._content["versions"][new_version] = self._content["versions"][old_version]
            del self._content["versions"][old_version]

        self._old_version = old_version
        self._new_version = new_version
        log_prefix = ""
        if self.has_update:
            log_prefix += "NEW"
            if len(self.dependencies_updated) > 0:
                log_prefix += "  DEP"
        elif len(self.dependencies_updated) > 0:
                log_prefix += "     DEP"
        elif self._bump_build is True:
            log_prefix += "  BUMP  "

        if len(self.dependencies_updated) > 0:
            self.logger.info(f"{log_prefix:10}replacing version '{old_version}' with '{new_version}', changed dependencies: {self.dependencies_updated}")
        else:
            self.logger.info(f"{log_prefix:10}replacing version '{old_version}' with '{new_version}'")
        if dry_run:
            with open(self._config_path) as f:
                old_config = yaml.safe_load(f)
            old_conandata = self.read_conandata()

            config_diff = diff(yaml.dump(old_config, default_flow_style=False, sort_keys=False), yaml.dump(self._content, default_flow_style=False, sort_keys=False))
            conandata_diff = diff(yaml.dump(old_conandata, default_flow_style=False, sort_keys=False), yaml.dump(conandata, default_flow_style=False, sort_keys=False))
            self.logger.debug(f"change {self._config_path} to:\n{config_diff}")
            self.logger.debug(f"change {self._conandata_path} to:\n{conandata_diff}")
        else:
            with open(self._config_path, 'w') as f:
                yaml.dump(self._content, f, default_flow_style=False, sort_keys=False)
            self.write_conandata(conandata)
        return (old_version, new_version)

    def post_flush(self, dry_run=False):
        conanfile_content = ""
        changed_content = ""
        if len(self.dependencies_updated) > 0:
            # update ref in our conandata.py
            with open(self._conanfile_path) as f:
                conanfile_content = f.read()
                changed_content = conanfile_content
            for dep in self.dependencies_updated:
                changed_content = changed_content.replace(dep.conan_version(old=True), dep.conan_version(new=True))

            if dry_run:
                changes = diff(conanfile_content, changed_content)
                self.logger.debug(f"change {self._conanfile_path} to:\n{changes}")
            else:
                with open(self._conanfile_path, 'w') as f:
                    f.write(changed_content)


    def dump_renovate(self, dry_run=False) -> dict:
        result = {}
        releases = []
        url = None

        if self.has_update and self.latest_release is not None:
            url = self.latest_release.get_download_url()

            if url is None:
                raise Exception(f"{self._name} no download url found")

            r = requests.head(url)
            if r.status_code >= 400:
                self.logger.error(f"invalid download URL: {url}, {r.status_code}")
            else:
                releases.append({
                    "version": self.latest_release.version,
                    "sourceUrl": url
                })

        if self.current_release is not None:
            releases.append({
                "version": self.current_release.version
            })
        result["releases"] = releases
        if url:
            result["sourceUrl"] = url
        return result


    def read_conandata(self) -> dict:
        if self._conandata_path is None:
            self._update_conandata_path()

        with open(self._conandata_path) as f:
            try:
                return yaml.safe_load(f)
            except yaml.YAMLError as exc:
                self.logger.error(exc)
        return {}

    def write_conandata(self, data):
        if self._conandata_path is None:
            self._update_conandata_path()

        with open(self._conandata_path, 'w') as f:
             yaml.dump(data, f, default_flow_style=False, sort_keys=False)

    def _update_conandata_path(self, force=False):
        if self._conandata_path is None or force:
            folder = self._content["versions"][self.current_release.version]["folder"]
            if folder is None or folder == "":
                raise Exception(f"{self._name}: no folder set it config.yml")

            self._conandata_path = self._recipe_dir.joinpath(folder, 'conandata.yml')
            self._conanfile_path = self._recipe_dir.joinpath(folder, 'conanfile.py')

    def validate(self):
        res = []
        self._check(silent=True)
        if not self.blocked and self.local_url() == "" and (not "locked" in self._content["updater"] or self._content["updater"]["locked"] is not True):
            # check url-pattern
            if "url-pattern" in self._content["updater"]:
                for platform in self._platforms.keys():
                    for arch in self._platforms[platform].keys():
                        try:
                            url = self.latest_release.get_download_url(platform, arch)
                        except Exception as e:
                            res.append(f"{e}")
                            continue

                        if url is None:
                            res.append(f"no url found for {platform}_{arch}")

                        r = requests.head(url)
                        if r.status_code >= 400:
                            res.append(f"invalid url for {platform}_{arch}: {url} [CODE: {r.status_code}]")

            else:
                res.append("missing url-pattern")
        return res


    def check(self, locked_override=None, pre_releases=None, force=False):
        self.has_update = self._check(locked_override, pre_releases, force=force)

    def local_url(self) -> str:
        for platform in self._platforms.keys():
            for arch in self._platforms[platform]:
                if "url" in self._platforms[platform][arch]:
                    if isinstance(self._platforms[platform][arch]["url"], list):
                        for url in self._platforms[platform][arch]["url"]:
                            if not url.startswith("http"):
                                return url

                    elif not self._platforms[platform][arch]["url"].startswith("http"):
                        return self._platforms[platform][arch]["url"]
        return ""

    def bump_build(self):
        if self.blocked is False:
            self._bump_build = True
        else:
            self.logger.debug(f"not bumping build because this version locked to 'system'")

    def _check(self, locked_override=None, pre_releases=None, silent=False, force=False) -> bool:
        if self.blocked:
            if not silent:
                self.logger.debug(f"locked to 'system'")
            return False

        local_url = self.local_url()
        if local_url != "":
            if not silent:
                self.logger.debug(f"updated locked because it uses local file {local_url}")
            return False

        if locked_override is not None:
            locked = False if locked_override == "false" else True if locked_override == "true" else locked_override
        else:
            locked = self._content["updater"]["locked"] if "locked" in self._content["updater"] else False

        if locked is True:
            if not silent:
                self.logger.debug(f"locked to '{self.current_release.version}'")
            return False

        type = self._content["updater"]["source"] if "source" in self._content["updater"] else "anitya"
        if type == "anitya":
            self.latest_release = get_latest_release_anitya(self._content["updater"], self.current_release, locked, pre_releases)
        elif type == "github":
            self.latest_release = get_latest_release_github(self._content["updater"], self.current_release, locked, pre_releases)
        elif type == "custom":
            self.latest_release = get_latest_release_custom(self._content["updater"], self.current_release, locked, pre_releases)
        elif type == "local":
            if not silent:
                self.logger.debug("updated locked because it uses local file")
            return False
        else:
            if not silent:
                self.logger.error(f"unknown source '{type}'")
            return False

        if self.latest_release is None:
            if not silent:
                self.logger.error(f'no version found (ID={self._content["updater"]["id"]})')
            return False

        self.latest_release.logger = self.logger
        try:
            res = self.compare()

            if res == 1:# and self.latest_release.version not in versions:
                if not silent:
                    self.logger.info(f"new version found '{self.current_release}'=>'{self.latest_release}'")
                return True
            elif res == -1:
                if not silent:
                    self.logger.warning(f"has newer current version than found '{self.current_release}' vs. '{self.latest_release}'")
                if force:
                    return True
            else:
                if not silent:
                    self.logger.info(f"still up-to-date '{self.current_release}' [{self.latest_release}]")
        except CompareError as e:
            self.logger.error(f"{e}")
        except CompareWarning as e:
            self.logger.warning(f"{e}")

        return False


    def compare(self) -> int:
        current_valid = self.current_release is not None
        latest_valid = self.latest_release is not None

        if not current_valid:
            if latest_valid:
                # if we have a valid latest version, we consider that as newer version
                return 1
            raise CompareError(f"invalid current version '{self.current_release}'")
        if not latest_valid:
            raise CompareError(f"invalid latest version '{self.latest_release}'")

        if current_valid and latest_valid:
            if self.latest_release > self.current_release:
                return 1
            elif self.latest_release < self.current_release:
                return -1
        return 0