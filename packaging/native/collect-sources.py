#!/usr/bin/env python3
"""Retain source/recipe revisions and license notices alongside native binaries."""
import json
import re
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path


def cache_path(ref, folder=None):
    command = ["conan", "cache", "path", ref]
    if folder:
        command += ["--folder", folder]
    return Path(subprocess.check_output(command, text=True).strip())


def main():
    graph_file, output = map(Path, sys.argv[1:])
    output.mkdir(parents=True, exist_ok=True)
    licenses = Path("build/Release/native-licenses")
    licenses.mkdir(parents=True, exist_ok=True)
    graph = json.loads(graph_file.read_text())["graph"]
    manifest = []
    with tarfile.open(output / "gonnect-corresponding-source.tar.gz", "w:gz") as archive:
        source_manifest = json.loads(Path(".native-source-manifest.json").read_text())
        for name in source_manifest["files"]:
            path = Path(name)
            if path.is_absolute() or ".." in path.parts:
                raise RuntimeError(f"Unsafe source manifest path: {name}")
            archive.add(path, arcname=f"gonnect/{name}", recursive=False)
        archive.add(".native-source-manifest.json", arcname="source-manifest.json")
        for node in graph["nodes"].values():
            ref = node.get("ref")
            if not ref or node.get("context") != "host" or ref.startswith("gonnect/"):
                continue
            if ref.split("#")[0].split("/")[-1] == "system":
                continue
            label = re.sub(r"[^A-Za-z0-9.+_-]", "_", ref)
            recipe = cache_path(ref)
            source = cache_path(ref, "source")
            if not source.exists() or not any(source.iterdir()):
                raise RuntimeError(f"No corresponding source retained for {ref}")
            archive.add(recipe, arcname=f"dependencies/{label}/recipe")
            exported = recipe.parent / "es"
            if exported.is_dir():
                archive.add(exported, arcname=f"dependencies/{label}/exported-source")
            archive.add(source, arcname=f"dependencies/{label}/source")
            count = 0
            roots = [source]
            package = node.get("package_folder")
            if package and (Path(package) / "licenses").is_dir():
                roots.append(Path(package) / "licenses")
            for index, root in enumerate(roots):
                for path in root.rglob("*"):
                    if path.is_file() and not path.is_symlink() and (
                        index > 0 or path.name.upper().startswith(("LICENSE", "COPYING", "NOTICE"))
                    ):
                        target = licenses / label / str(index) / path.relative_to(root)
                        target.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(path, target)
                        count += 1
            if not count:
                raise RuntimeError(f"No license notices retained for {ref}")
            manifest.append({"ref": ref, "license": node.get("license"), "notices": count})
        archive.add(graph_file, arcname="build-dependencies.json")
    (licenses / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    shutil.copy2(licenses / "manifest.json", output / "third-party-manifest.json")


if __name__ == "__main__":
    main()
