#!/usr/bin/env python3
"""Build the Board Manager release: platform zip + package index JSON.

Usage:
    python package/make_release.py --repo OWNER/NAME [--previous-index FILE] [--local]

Output (package/build/):
    phyduino-rp2-<version>.zip      platform/ (minus EXCLUDE files), one root folder
    package_phyduino_index.json     index for the Arduino Board Manager
    tools/                          re-packed tool archives not yet published (if any)

The version comes from VERSION and must match platform.txt. Tool URLs come from
package/tools.json; their checksums are computed here (downloads are cached in
package/build/cache/). Recipe: docs/maintenance/release.md
"""
import argparse
import fnmatch
import hashlib
import json
import pathlib
import re
import sys
import urllib.error
import urllib.request
import zipfile

REPO = pathlib.Path(__file__).resolve().parent.parent
PLATFORM = REPO / "platform"
BUILD = REPO / "package" / "build"
# Never shipped, even if they appear under platform/: per-machine settings and
# CMake files (the CMake verification build lives outside platform/ anyway).
EXCLUDE = ["platform.local.txt", "CMakeLists.txt", "CMakePresets.json", "*.cmake"]

PACKAGER = "phyduino"
ARCH = "rp2"
MAINTAINER = "University of Toronto, Department of Physics"


def read_version():
    version = (REPO / "VERSION").read_text().strip()
    match = re.search(r"^version=(.+)$", (PLATFORM / "platform.txt").read_text(), re.M)
    if not match or match.group(1).strip() != version:
        sys.exit(f"VERSION ({version}) and platform.txt version= must match")
    return version


def board_names():
    text = (PLATFORM / "boards.txt").read_text()
    return [{"name": m} for m in re.findall(r"^[\w]+\.name=(.+)$", text, re.M)]


def file_info(path):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return {"checksum": f"SHA-256:{digest}", "size": str(path.stat().st_size)}


def zip_platform(version):
    root = f"phyduino-{ARCH}-{version}"
    out = BUILD / f"{root}.zip"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for path in sorted(PLATFORM.rglob("*")):
            if path.is_file() and not any(fnmatch.fnmatch(path.name, pat) for pat in EXCLUDE):
                z.write(path, f"{root}/{path.relative_to(PLATFORM).as_posix()}")
    return out


def download(url, dest):
    """Downloads url to dest unless cached. Returns False on HTTP 404."""
    if dest.exists():
        return True
    print(f"downloading {url}")
    try:
        urllib.request.urlretrieve(url, dest)
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return False
        raise
    return True


def repack_zip(src, dest, root):
    """Rewrites a zip so every entry sits under one root folder (Arduino requires it).

    Entry order, dates and permissions are kept, so the output is reproducible.
    """
    with zipfile.ZipFile(src) as zin, zipfile.ZipFile(dest, "w", zipfile.ZIP_DEFLATED) as zout:
        for info in zin.infolist():
            new = zipfile.ZipInfo(f"{root}/{info.filename}", info.date_time)
            new.external_attr = info.external_attr
            new.compress_type = zipfile.ZIP_DEFLATED
            zout.writestr(new, zin.read(info))


def tool_entries(repo, local):
    """Index entries for package/tools.json.

    Systems marked "repack" are re-packed with a single root folder and hosted
    as assets of the GitHub release named by the tool's "host_tag". Once that
    release exists, the hosted file is downloaded and checksummed (never
    re-packed again), so the index always matches what users download.
    Files that still need publishing are left in package/build/tools/.
    """
    cache, to_publish = BUILD / "cache", BUILD / "tools"
    cache.mkdir(parents=True, exist_ok=True)
    tools = json.loads((REPO / "package" / "tools.json").read_text())["tools"]
    entries = []
    for tool in tools:
        systems = []
        for host, source in tool["systems"].items():
            url = source if isinstance(source, str) else source["url"]
            original = cache / url.rsplit("/", 1)[-1]

            if isinstance(source, str):  # upstream archive is usable as-is
                download(url, original)
                systems.append({"host": host, "url": url, "archiveFileName": original.name,
                                **file_info(original)})
                continue

            name = f"{tool['name']}-{tool['version']}-{host}.zip"
            tag = tool["host_tag"]
            hosted_url = (f"http://localhost:8000/tools/{name}" if local else
                          f"https://github.com/{repo}/releases/download/{tag}/{name}")
            hosted = cache / f"hosted-{name}"
            if local or not download(hosted_url, hosted):
                to_publish.mkdir(exist_ok=True)
                hosted = to_publish / name
                if not hosted.exists():
                    download(url, original)
                    print(f"re-packing {original.name} -> {name}")
                    repack_zip(original, hosted, f"{tool['name']}-{tool['version']}")
            systems.append({"host": host, "url": hosted_url, "archiveFileName": name,
                            **file_info(hosted)})
        entries.append({"name": tool["name"], "version": tool["version"], "systems": systems})
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--repo", required=True, help="GitHub OWNER/NAME hosting the releases")
    parser.add_argument("--previous-index", type=pathlib.Path,
                        help="last published index; its other versions are kept")
    parser.add_argument("--local", action="store_true",
                        help="point the index at http://localhost:8000/ to test installing; "
                             "serve with: python -m http.server -d package/build 8000")
    args = parser.parse_args()

    BUILD.mkdir(parents=True, exist_ok=True)
    version = read_version()
    archive = zip_platform(version)
    tools = tool_entries(args.repo, args.local)

    platform = {
        "name": "Phyduino Boards",
        "architecture": ARCH,
        "version": version,
        "category": "Contributed",
        "url": f"http://localhost:8000/{archive.name}" if args.local else
               f"https://github.com/{args.repo}/releases/download/v{version}/{archive.name}",
        "archiveFileName": archive.name,
        **file_info(archive),
        "help": {"online": f"https://github.com/{args.repo}"},
        "boards": board_names(),
        "toolsDependencies": [
            {"packager": PACKAGER, "name": t["name"], "version": t["version"]} for t in tools
        ],
    }

    platforms, known_tools = [platform], {(t["name"], t["version"]) for t in tools}
    if args.previous_index and args.previous_index.exists():
        old = json.loads(args.previous_index.read_text())["packages"][0]
        platforms += [p for p in old["platforms"] if p["version"] != version]
        tools += [t for t in old["tools"] if (t["name"], t["version"]) not in known_tools]

    index = {"packages": [{
        "name": PACKAGER,
        "maintainer": MAINTAINER,
        "websiteURL": f"https://github.com/{args.repo}",
        "email": "",
        "help": {"online": f"https://github.com/{args.repo}"},
        "platforms": platforms,
        "tools": tools,
    }]}
    out = BUILD / "package_phyduino_index.json"
    out.write_text(json.dumps(index, indent=2) + "\n")
    print(f"wrote {archive.relative_to(REPO)}\nwrote {out.relative_to(REPO)}")


if __name__ == "__main__":
    main()
