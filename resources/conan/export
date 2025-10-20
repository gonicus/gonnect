#!/usr/bin/env python3
import yaml
import pathlib
import subprocess

recipes = pathlib.Path(__file__).parent / pathlib.Path("recipes")
for recipe_dir in [r for r in recipes.iterdir() if r.is_dir()]:
    recipe_conf = pathlib.Path(recipe_dir, "config.yml")
    if recipe_conf.is_file():
        package_name = pathlib.PurePath(recipe_dir).name

        with open(recipe_conf) as conf:
            info = yaml.load(conf, Loader=yaml.FullLoader)
            version = list(info.get('versions').keys())[0]
            sub_path = info.get('versions').get(version).get('folder')

            print(f"Exporting {package_name} {version}...")
            subprocess.run("conan export %s --version=%s" % (pathlib.PurePath(recipe_dir, sub_path), version) , shell=True, capture_output=True, text=True)
