import semver
import pprint
import requests
import requests_cache
import re
import hashlib
import dateutil.parser as parser
from string import Template
from typing import Optional
from .logger import logger

s = requests_cache.CachedSession('updater')


def fix_version(version: str) -> str:
    """_summary_
    Converts any incoming version string to a valid semver equivalent

    Args:
        version (str): version string

    Returns:
        str: version string that can be parsed by semver
    """
    # check for build number first
    m = re.match(r"^(.+)(\+\d+)$", version)
    build_no_suffix = ""
    base_version = version
    if m:
        base_version = m.group(1)
        build_no_suffix = m.group(2)

    skip = False

    m = re.match(r"^(\d+\.\d+)$", base_version)
    if m:
        # incomplete, e.g. "10.46" -> "10.46.0"
        base_version += ".0"
        skip = True

    m = re.match(r"^(\d+\.\d+\.\d+)([a-z]+)\.?(\d+)$", base_version)
    if m:
        # 1.1.0rc1 -> 1.1.0-rc.1
        base_version = f"{m.group(1)}-{m.group(2)}.{m.group(3)}"
        skip = True

    if not skip:
        m = re.match(r"^(\d{4})[\.-]?(\d{2})[\.-]?(\d{2})?$", base_version)
        if m:
            # 20250930 -> 2025.09.30
            base_version = f"{m.group(1)}.{m.group(2)}.{m.group(3)}"
            skip = True

    if not skip:
        m = re.match(r"^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})Z?$", base_version)
        if m:
            # ISO date
            base_version = f"{m.group(1)}.{m.group(2)}.{m.group(3)}"
            seconds_of_day = int(m.group(4)) * 3600 + int(m.group(5)) * 60 + int(m.group(6))
            build_no_suffix = "+" + str(seconds_of_day)
            skip = True

    if not skip:
        # try to parse as date
        try:
            date = parser.parse(base_version)
            base_version = f"{date.year}.{date.month}.{date.day}"
            seconds_of_day = date.hour * 3600 + date.minute * 60 + date.second
            build_no_suffix = "+" + str(seconds_of_day)
        except parser.ParserError as e:
            pass


    # leading zeros, e.g. 1.1.05
    base_version = re.sub(r"\.0+(\d+)", r".\1", base_version)
    return base_version + build_no_suffix

class Release:
    def __init__(self, version, config, payload=None, name="", logger=None):
        self._name = name
        self.version = version
        self.config = config
        self.payload = payload
        self.semver_version = None
        self._download_url = None
        self._hash = None
        self._logger = logger if logger is not None else logger
        self._buildno_updated = False

    def __str__(self):
        return self.version

    def update_buildno(self):
        if not self._buildno_updated: # only done once per run
            if semver.Version.is_valid(self.version):
                version = self.semver()
                self.semver_version = version.bump_build(token="")
                self.version = str(self.semver_version)
            else:
                m = re.match(r"^([^+]+)\+?(\d)?$", self.version)
                current_build_no = 0
                old_version = self.version
                if m and m.group(2) is not None:
                    old_version = m.group(1)
                    current_build_no = int(m.group(2))
                current_build_no+=1
                self.version = f"{old_version}+{current_build_no}"
                self._buildno_updated = True

    def semver(self) -> semver.Version:
        if self.semver_version is None:
            if semver.Version.is_valid(self.version):
                self.semver_version = semver.Version.parse(self.version)
            elif "pattern" in self.config:
                version = self.version
                if "prefix" in self.config and version.startswith(self.config["prefix"]):
                    version = version[len(self.config["prefix"]):]
                m = re.match(self.config["pattern"], version)
                if m:
                    groups = m.groupdict()
                    if "major" in groups or "minor" in groups:
                        major = 0
                        minor = 0
                        patch = 0
                        pre = None
                        if "major" in groups:
                            major = int(groups['major']) if groups['major'] is not None and groups['major'].isdigit() else ord(groups['major']) if len(groups['major']) > 0 else 0
                        if "minor" in groups:
                            minor = int(groups['minor']) if groups['minor'] is not None and groups['minor'].isdigit() else ord(groups['minor']) if len(groups['minor']) > 0 else 0
                        if "patch" in groups:
                            patch = int(groups['patch']) if groups['patch'] is not None and groups['patch'].isdigit() else ord(groups['patch']) if len(groups['patch']) > 0 else 0
                        if "pre" in groups:
                            pre = groups['pre']
                        self.semver_version = semver.Version(major, minor, patch, prerelease=pre)
                    else:
                        parts = [x if x.isdigit() else ord(x) if len(x) > 0 else 0 for x in m.groups()] # convert chars to numbers
                        parts += [0] * (3 - len(parts)) # fill up missing numbers
                        self.semver_version = semver.Version(*parts)
            else:
                version = self.version
                if "prefix" in self.config and version.startswith(self.config["prefix"]):
                    version = version[len(self.config["prefix"]):]
                version = fix_version(version)
                if semver.Version.is_valid(version):
                    self.semver_version = semver.Version.parse(version)
        if self.semver_version is None:
            raise Exception(f"{self.name:20} version {self.version} not parseable")

        return self.semver_version

    def get_download_url(self, platform="all", arch="all"):
        if "url-pattern" in self.config:
            data = self.payload.copy() if self.payload is not None else {}
            if platform != "all" and "${platform}" not in self.config["url-pattern"] and ("platform-mapping" not in self.config or self.config["platform-mapping"] is not False):
                raise Exception(f"url-pattern does not allow platform specific urls")
            elif platform == "all" and "${platform}" in self.config["url-pattern"]:
                raise Exception(f"url-pattern requires a defined platform")
            if arch != "all" and "${arch}" not in self.config["url-pattern"] and ("arch-mapping" not in self.config or self.config["arch-mapping"] is not False):
                raise Exception(f"url-pattern does not allow architecture specific urls")
            elif arch == "all" and "${arch}" in self.config["url-pattern"]:
                raise Exception(f"url-pattern requires a defined architecture")


            if platform != "all":
                if "platform-mapping" in self.config and self.config["platform-mapping"] is not False  and platform in self.config["platform-mapping"]:
                    data["platform"] = self.config["platform-mapping"][platform]
                else:
                    data["platform"] = platform

            if arch != "all":
                if "arch-mapping" in self.config and self.config["arch-mapping"] is not False and arch in self.config["arch-mapping"]:
                    data["arch"] = self.config["arch-mapping"][arch]
                else:
                    data["arch"] = arch
            data["version"] = self.version

            if self.semver_version is not None:
                data["major"] = self.semver_version.major
                data["minor"] = self.semver_version.minor
                data["patch"] = self.semver_version.patch

            templ = Template(self.config["url-pattern"])
            return templ.substitute(**data)

    @property
    def logger(self):
        return self._logger

    @logger.setter
    def logger(self, logger):
        self._logger = logger

    @property
    def major(self):
        return self.semver().major

    @property
    def minor(self):
        return self.semver().minor

    @property
    def patch(self):
        return self.semver().patch

    @property
    def is_prerelease(self):
        return self.semver().prerelease is not None or "prerelease" in self.payload and self.payload["prerelease"] is True

    @property
    def name(self):
        return self._name

    @property
    def hash(self) -> str:
        return self._hash

    @property
    def download_url(self) -> str:
        return self._download_url

    def __gt__(self, other):
        return self.semver() > other.semver()

    def __lt__(self, other):
        return self.semver() < other.semver()

    def __eq__(self, other):
        if self.version == other.version:
            return True
        return self.semver() == other.semver()

class GithubRelease(Release):
    @property
    def download_url(self) -> str:
        if self._download_url is None:
            if "zipball_url" in self.payload:
                self._download_url = self.payload["zipball_url"]
            elif "sha" in self.payload:
                self._download_url = self.payload["sha"]
            else:
                self._download_url = ""
        return self._download_url

    @property
    def hash(self) -> str:
        if self._hash is None:
            if "commit" in self.payload:
                # only available for tags
                self._hash = self.payload["commit"]["sha"]
            elif "sha" in self.payload:
                self._hash = self.payload["sha"]
            else:
                # for releases we only have the tag name
                self._hash = ""
        return self._hash

class AmityaRelease(Release):
    def __init__(self, version, config, payload={}):
        super().__init__(version, config, payload, payload["name"])
        self._github_repo = None

    @property
    def name(self):
        if self._name == "":
            self._name = self.payload["name"]
        return self._name

    @property
    def download_url(self):
        if self._download_url is None:
            self._load_details()
        return self._download_url

    @property
    def hash(self):
        if self._hash is None:
            self._load_details()
        return self._hash

    def _load_details(self):
        #pprint.pp(self.payload)
        if "version_url" in self.payload and self.payload["version_url"].startswith("https://github.com/"):
            # get repo from version url
            m = re.match(r"^https://github.com/([^/]+)/([^/]+)/(tags|releases)$", self.payload["version_url"])
            self.logger.debug(f'checking version_url {self.payload["version_url"]}')

            if m:
                self._github_repo = f"{m.group(1)}/{m.group(2)}"
                if m.group(3) == "tags":
                    version = self.version
                    if "prefix" in self.config:
                        version = self.config["prefix"] + version
                    self._download_url = f"https://api.github.com/repos/{self._github_repo}/zipball/{version}"
                    r = s.get(f'https://api.github.com/repos/{self._github_repo}/tags',
                        headers={
                            "Accept": "application/vnd.github+json"
                        })
                    if r.status_code == requests.codes.ok:
                        data = r.json()
                        for entry in data:
                            if entry["name"] == version:
                                self._hash = entry["commit"]["sha"]
                                self._download_url = entry["zipball_url"]
                                break
                    else:
                        logger.error(r.status_code)
                else:
                    raise Exception(f"unhandled github version url type: {m.group(3)}, currently only 'tags' is implemented")
            else:
                raise Exception(f"invalid github version url type: {self.payload["version_url"]}")

class RenovateRelease(Release):
    def __init__(self, version, config, payload={}):
        super().__init__(version, config, payload, payload["depName"])
        self._github_repo = None

    @property
    def name(self):
        if self._name == "":
            self._name = self.payload["depName"]
        return self._name

    @property
    def download_url(self):
        return self.payload["sourceUrl"]



def semver_parse(version):
    try:
        if not semver.Version.is_valid(version):
            version = fix_version(version)
        return semver.Version.parse(version)
    except:
        return "0.0.0"


def get_latest_release_anitya(config: dict, current_release: Release, locked, pre_releases=None) -> Optional[Release]:
    r = s.get(f'https://release-monitoring.org/api/project/{config["id"]}')
    if r.status_code == requests.codes.ok:
        data = r.json()
        if pre_releases is None:
            pre_releases = config["preReleases"] if "preReleases" in config else False
        if locked is False and pre_releases is True:
            return AmityaRelease(data["version"], config, data)
        else:
            # find latest version for locked state (major, minor, patch)
            versions = data["versions"] if pre_releases else data["stable_versions"]
            current = current_release.semver()
            if "pattern" in config:
                if locked is False:
                    versions = [x for x in map(lambda x: AmityaRelease(x, config, data), versions)]
                elif locked == "minor":
                    versions = [x for x in map(lambda x: AmityaRelease(x, config, data), versions) if x.major == current.major]
                elif locked == "patch":
                    versions = [x for x in map(lambda x: AmityaRelease(x, config, data), versions) if x.major == current.major and x.minor == current.minor]
                return max(versions) if len(versions) > 0 else None
            else:
                if locked == "minor":
                    versions = [v for v in versions if v.startswith(f"{current.major}.")]
                elif locked == "patch":
                    versions = [v for v in versions if v.startswith(f"{current.major}.{current.minor}.")]
            return AmityaRelease(max(versions, key=semver_parse), config, data) if len(versions) > 0 else None

    return None

def get_latest_release_github(config : dict, current_release: Release, locked, pre_releases=None, use_tags=True) -> Optional[Release]:
    mode = "tags" if use_tags else "releases"
    if "branch" in config:
        # check for new commits in branch
        url = f'https://api.github.com/repos/{config["id"]}/commits/{config["branch"]}'
        use_tags = False
        mode = "commits"
    else:
        url = f'https://api.github.com/repos/{config["id"]}/{mode}'

    r = s.get(url,
              headers={
                  "Accept": "application/vnd.github+json"
              })
    if r.status_code == requests.codes.ok:
        data = r.json()
        if mode == "commits":
            return GithubRelease(data["commit"]["author"]["date"].split("T")[0], config, data)
        else:
            if len(data) == 0 and mode == "tags":
                return get_latest_release_github(config, current_release, locked, pre_releases, False)

            tag_property = "name" if use_tags else "tag_name"

            if pre_releases is None:
                pre_releases = config["preReleases"] if "preReleases" in config else False
            releases = map(lambda x: GithubRelease(x[tag_property], config, x), data)
            # sort by version descending
            releases = sorted(list(releases), reverse=True)
            if locked is False:
                if pre_releases:
                    return releases[0]
                else:
                    for release in releases:
                        if release.is_prerelease is False:
                            return release
            else:
                current = current_release.semver()
                for release in releases:
                    if not pre_releases and release.is_prerelease is True:
                        continue

                    if locked == "minor":
                        if current.major == release.major:
                            return release
                    elif locked == "patch":
                        if current.major == release.major and current.minor == release.minor:
                            return release

    return None

def get_latest_release_custom(config : dict, current_release: Release, locked, pre_releases=None, use_tags=True) -> Optional[Release]:
    if "url" in config:
        r = s.get(config["url"])
        if r.status_code == requests.codes.ok:
            version_regex = re.compile(config["pattern"])
            releases = []
            for entry in re.finditer(version_regex, r.text):
                major = int(entry.group("major")) or "0"
                minor = int(entry.group("minor")) or "0"
                patch = int(entry.group("patch")) or "0"
                releases.append(Release(f"{major}.{minor}.{patch}", config, entry.groupdict(), name=current_release.name))

        # sort by version descending
        releases = sorted(list(releases), reverse=True)
        if locked is False:
            # currently we do not support pre-release detection for custom checks
            return releases[0]
        else:
            current = current_release.semver()
            for release in releases:
                if locked == "minor":
                    if current.major == release.major:
                        return release
                elif locked == "patch":
                    if current.major == release.major and current.minor == release.minor:
                        return release
    return None

def sha256(url):
    sha = hashlib.sha256()
    try:
        with s.get(url, stream=True) as r:
            r.raise_for_status()
            for chunk in r.iter_content(chunk_size=8192):
                sha.update(chunk)
        return sha.hexdigest()
    except requests.HTTPError as e:
        logger.error(f"{e}")
        return None
