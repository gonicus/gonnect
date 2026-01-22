#!/usr/bin/env python3

import argparse
from pathlib import Path
import yaml
import os
import re
from updater.logger import logger
from updater import Dependency, diff
from updater.releases import RenovateRelease
import colorlog
import json
from sh import grep, ErrorReturnCode_1

SCRIPT_DIR = Path(__file__).parent.resolve()
ROOT_DIR = SCRIPT_DIR.parent.parent.resolve()
RECIPES = ('resources', 'conan', 'recipes')
PACKAGE_SUFFIX = "@_/_"

log_level = colorlog.INFO
# uncomment this to see the sh commands
# import logging
# logging.basicConfig(level=log_level)


class Conan:
    def __init__(self, dir : Path, renovate_dir=None):
        self.dependencies = {}
        self._dir = dir
        self._profile_path = dir.joinpath('conanfile.py')
        if not self._profile_path.exists():
            logger.error(f"{self._profile_path} not found")

        self._recipes_dir = dir.joinpath(*RECIPES)
        if not self._recipes_dir.exists():
            logger.error(f"{self._recipes_dir} not found")
            logger.error(next(os.walk(dir), (None, None, [])))

        self._project = None
        self._requires_pattern = re.compile(r'self.requires\("([^"]+)"')
        self._renovate_dir = renovate_dir

    def create(self, name, content):
        dep = Dependency(name, content, recipes_dir=self._recipes_dir, suffix=PACKAGE_SUFFIX)
        dep.logger.setLevel(log_level)
        self.dependencies[name] = dep
        return dep

    def get_tool_requires(self):
        tools = []
        for profile_path in grep('-rl', 'tool_requires', '--include=conanfile.py', self._dir).splitlines():
            with open(profile_path) as f:
                in_section = False
                for line in f.readlines():
                    content = line.strip()
                    if content == "[tool_requires]":
                        in_section = True
                    elif content == "":
                        in_section = False
                    elif in_section:
                        tools.append(content.split("/")[0])
        return tools

    def run(self, filter=None, locked=None, pre_releases=None,
            dry_run=False, validate=False, force=False, blacklist=[],
            bump_build=False):

        whitelist = []
        if filter is not None:
            whitelist = [x for x in filter.split(",")]

        tool_requires = self.get_tool_requires()
        if bump_build is True:
            blacklist = tool_requires

        paths = list(self._recipes_dir.rglob('config.yml'))
        if len(paths) == 0:
            logger.error(f"no config.yml files found in {self._recipes_dir}")
        results = {}

        tool_requirement_changed = []

        for path in paths:
            with open(path) as f:
                try:
                    content = yaml.safe_load(f)
                    if validate and "updater" not in content and "system" not in content["versions"].keys():
                        results[path.parent.name] = "has no updater configuration in its config.yml"
                    elif "updater" in content:
                        dep = self.create(path.parent.name, content)
                        if (len(whitelist) == 0 or path.parent.name in whitelist) and path.parent.name not in blacklist:
                            if bump_build is True:
                                dep.bump_build()
                            elif validate:
                                res = dep.validate()
                                if len(res) > 0:
                                    print(f'results for {dep.name}:')
                                    print(f"  - {"\n  - ".join(res)}")
                            else:
                                dep.check(locked, pre_releases, force)
                                if dep.changed and dep.name in tool_requires:
                                    tool_requirement_changed.append(dep.name)
                    elif "system" not in content["versions"].keys():
                        logger.warning(f"no updater section in {path}")

                except yaml.YAMLError as exc:
                    logger.error(exc)

        if self._renovate_dir is not None:
            # we only need to check to get the latest version for the dependencies
            self._export_renovate_files(whitelist, blacklist, dry_run)
            return

        if len(tool_requirement_changed) > 0:
            logger.info(f"bumping all build numbers because tool requirements have been updated: {tool_requirement_changed}")
            for dep in self.dependencies.values():
                if (len(whitelist) == 0 or dep.name in whitelist) and dep.name not in tool_requires:
                    dep.bump_build()


        nothing_changed = False
        c = 0
        while not nothing_changed and c < 5:
            nothing_changed = True
            c+=1
            for dep in self.dependencies.values():
                if dep.changed:
                    # check all other dependencies that use this one and mark them to update their build number
                    try:
                        others = []
                        for path in grep('-ir', fr'requires("{dep.name}/.*{PACKAGE_SUFFIX}"', '--include=conanfile.py', self._recipes_dir).strip().split("\n"):
                            others.append(path[len(str(ROOT_DIR))+1:].split("/")[1])

                        for other in others:
                            if self.dependencies[other].add_updated_dependency(dep):
                                nothing_changed = False
                    except ErrorReturnCode_1:
                        logger.debug(f"no usage of '{dep.name}' found")

        self._flush(dry_run)


    def _flush(self, dry_run=False):
        replacements = {}
        for dep in self.dependencies.values():
            try:
                res = dep.flush(dry_run)
                if res is not None:
                    old_version, new_version = res
                    replacements[dep.conan_version(version=old_version)] = dep.conan_version(version=new_version)
            except Exception as e:
                logger.error(f"{dep}: {e}")

        for dep in self.dependencies.values():
            try:
                dep.post_flush(dry_run)
            except Exception as e:
                logger.error(f"{dep}: {e}")

        if len(replacements) > 0:

            # check profiles
            profile_refs = {}
            for match in grep("-r", "self.requires", "--include=conanfile.py", self._dir).splitlines():
                if "/test_package/" in match:
                    continue
                file_path, raw_ref = [x.strip() for x in match.split(":")]
                m = self._requires_pattern.search(raw_ref)
                if m:
                    ref = m.group(1)

                    if ref in replacements:
                        if file_path not in profile_refs:
                            profile_refs[file_path] = [ref]
                        else:
                            profile_refs[file_path].append(ref)
                else:
                    logger.error(f"unknown ref format {raw_ref} in {file_path}")

            for file_path, refs in profile_refs.items():
                with open(file_path, "r+") as f:
                    orig = f.read()
                    changed = orig

                    for ref in refs:
                        changed = changed.replace(ref, replacements[ref])

                    if dry_run:
                        changes = diff(orig, changed)
                        logger.debug(f"change {file_path} to:\n{changes}")
                    else:
                        f.seek(0)
                        f.write(changed)
                        f.truncate()

    def _export_renovate_files(self, whitelist, blacklist, dry_run=False):
        for dep in self.dependencies.values():
            if (len(whitelist) == 0 or dep.name in whitelist) and dep.name not in blacklist:
                data = dep.dump_renovate()
                filename = Path(self._renovate_dir).joinpath(dep.package, 'versions.json')
                logger.debug(f"{filename}: {json.dumps(data, indent=2)}")
                if not dry_run:
                    filename.parent.mkdir(parents=True, exist_ok=True)
                    with open(filename, "w") as f:
                        f.write(json.dumps(data, indent=2))

    def update_recipes(self, data_file, dry_run=False):
        with open(data_file) as json_data:
            try:
                data = json.load(json_data)
            except json.JSONDecodeError as e:
                logger.error("Invalid JSON syntax:", e)
                return

            config_path = self._recipes_dir.joinpath(data["depName"], "config.yml")
            if config_path.exists():
                with open(config_path) as f:
                    try:
                        content = yaml.safe_load(f)

                        if "updater" in content:
                            dep = self.create(data["depName"], content)
                            if not dep.blocked:
                                dep.check()
                                # only call flush, renovate already did the post_flush part
                                dep.flush(dry_run=dry_run)

                    except yaml.YAMLError as exc:
                        logger.error(exc)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", dest="verbose", action="store_true",
                        help="Activate debug logging")
    parser.add_argument("--dry-run", dest="dry_run", action="store_true",
                        help="Do not change anything")
    parser.add_argument("-f", "--filter", dest="filter",
                        help="Process only these dependencies (comma separated list)")
    parser.add_argument("--locked", dest="locked", choices=['false', 'true', 'major', 'minor'],
                        help="Override configured locked state")
    parser.add_argument("--pre-releases", dest="pre_releases", action="store_true",
                        help="Allow pre-releases")
    parser.add_argument("--validate", dest="validate", action="store_true",
                        help="Validate updater configuration")
    parser.add_argument("--force", dest="force", action="store_true",
                        help="Force updating to latest version found")
    parser.add_argument("--bump-build", dest="bump_build", action="store_true",
                        help="Increase all build versions (except android tools)")
    parser.add_argument("--renovate", dest="renovate_dir",
                        help="Generate renovate local datasource files in this folder")
    parser.add_argument("--dir", help="Directory where the updates should be checked")
    parser.add_argument("--update-recipes", dest="update_recipes",
                        help="Path to a file generated for renovates postUpgradeTasks, updates urls / versions in recipes in that file")

    args = parser.parse_args()
    args_dict = vars(args)
    if args.verbose:
        log_level = colorlog.DEBUG
        logger.setLevel(log_level)
    del args_dict["verbose"]

    dir = ROOT_DIR
    if args.dir:
        dir = Path(args.dir)
    del args_dict["dir"]

    conan = Conan(dir, renovate_dir=args.renovate_dir)
    del args_dict["renovate_dir"]
    merge_request = None
    if args.update_recipes:
        conan.update_recipes(args.update_recipes, args.dry_run)
    else:
        del args_dict["update_recipes"]
        conan.run(**args_dict)
