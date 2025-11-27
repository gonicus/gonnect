#!/usr/bin/env python3
import yaml
import pathlib
from conan.errors import ConanException
from conan.api.conan_api import ConanAPI
from conan.api.subapi.export import ExportAPI
from conan.internal.model.version_range import validate_conan_version

required_conan_version = ">=2.20"
try:
    validate_conan_version(required_conan_version)
except ConanException:
    print(f"Error: conan version {required_conan_version} required")
    exit(1)

conan_api = ConanAPI()
api = ExportAPI(conan_api, conan_api._api_helpers)

recipes = pathlib.Path(__file__).parent / pathlib.Path("recipes")
for recipe_dir in [r for r in recipes.iterdir() if r.is_dir()]:
    recipe_conf = pathlib.Path(recipe_dir, "config.yml")
    if recipe_conf.is_file():
        package_name = pathlib.PurePath(recipe_dir).name

        with open(recipe_conf) as conf:
            info = yaml.load(conf, Loader=yaml.FullLoader)
            version = list(info.get('versions').keys())[0]
            sub_path = info.get('versions').get(version).get('folder')

            api.export(pathlib.PurePath(recipe_dir, sub_path, "conanfile.py"), package_name, version)
