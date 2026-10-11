#!/usr/bin/env python3
"""Map resolved QML imports to distro packages, including lazy view imports."""
import json
import re
import subprocess
import sys
from pathlib import Path


def package_owner(kind, path):
    if kind == "DEB":
        owner = subprocess.check_output(["dpkg-query", "-S", path], text=True)
        return owner.split(": ", 1)[0].split(":", 1)[0].strip()
    if kind == "RPM":
        return subprocess.check_output(["rpm", "-qf", "--qf", "%{NAME}", path], text=True).strip()
    raise ValueError("Expected DEB or RPM")


def main():
    kind, filename, output = sys.argv[1:]
    imports = json.loads(Path(filename).read_text())
    direct = set()
    for path in Path("src").rglob("*.qml"):
        direct.update(re.findall(r"^\s*import\s+([A-Za-z_][\w.]*)", path.read_text(), re.M))
    resolved = {item["name"] for item in imports if item.get("path")}
    missing = direct - resolved - {"base"}
    if missing:
        raise RuntimeError(f"Unresolved application QML imports: {sorted(missing)}")
    packages = set()
    modules = []
    for item in imports:
        # Qt's scanner also reports conditional Windows/macOS style imports.
        path = item.get("path", "")
        if item.get("type") != "module" or not path.startswith("/usr/"):
            continue
        qmldir = str(Path(path) / "qmldir")
        package = package_owner(kind, qmldir)
        if "-dev" in package or package.endswith("-devel"):
            raise RuntimeError(f"QML runtime import unexpectedly owned by development package: {package}")
        packages.add(package.strip())
        modules.append({"name": item["name"], "path": path, "package": package.strip()})
    plugin_root = Path(subprocess.check_output(
        ["qtpaths6", "--query", "QT_INSTALL_PLUGINS"], text=True).strip())
    for relative in ("imageformats/libqsvg.so", "sqldrivers/libqsqlite.so"):
        plugin = plugin_root / relative
        if not plugin.is_file():
            raise RuntimeError(f"Required runtime plugin missing: {plugin}")
        owner = package_owner(kind, str(plugin))
        packages.add(owner)
        modules.append({"name": relative, "path": str(plugin), "package": owner, "kind": "plugin"})
    if not modules:
        raise RuntimeError("No system QML modules were scanned")
    Path(output).write_text(json.dumps(modules, indent=2) + "\n")
    print(", ".join(sorted(packages)))


if __name__ == "__main__":
    main()
