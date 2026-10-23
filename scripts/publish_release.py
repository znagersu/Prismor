#!/usr/bin/env python3
import os
import sys
import re
import subprocess
import shutil

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def get_version():
    cmake_path = os.path.join(PROJECT_ROOT, "CMakeLists.txt")
    with open(cmake_path, "r", encoding="utf-8") as f:
        content = f.read()
    m = re.search(r"project\s*\(\s*Prismor\s+VERSION\s+([0-9.]+)", content, re.IGNORECASE)
    if not m:
        raise RuntimeError("Could not parse version from CMakeLists.txt")
    return m.group(1).strip()

def find_gh_cli():
    candidates = [
        "gh",
        r"C:\Program Files\GitHub CLI\gh.exe",
        os.path.join(os.environ.get("LOCALAPPDATA", ""), r"Programs\GitHub CLI\gh.exe")
    ]
    for c in candidates:
        try:
            subprocess.run([c, "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
            return c
        except Exception:
            continue
    raise RuntimeError("GitHub CLI (gh) was not found.")

def main():
    print("=== Prismor Autonomous Build-to-Release Pipeline ===")
    version = get_version()
    print(f"Detected Version: {version}")

    standalone_exe = os.path.join(PROJECT_ROOT, "build", "Prismor_artefacts", "Release", "Standalone", f"Prismor {version}.exe")
    vst3_dir = os.path.join(PROJECT_ROOT, "build", "Prismor_artefacts", "Release", "VST3", f"Prismor {version}.vst3")

    if not os.path.isfile(standalone_exe):
        raise FileNotFoundError(f"Standalone executable not found: {standalone_exe}")
    if not os.path.isdir(vst3_dir):
        raise FileNotFoundError(f"VST3 bundle not found: {vst3_dir}")

    staging_dir = os.path.join(PROJECT_ROOT, "build", "release_staging")
    if os.path.exists(staging_dir):
        shutil.rmtree(staging_dir)
    os.makedirs(staging_dir, exist_ok=True)

    shutil.copy2(standalone_exe, os.path.join(staging_dir, f"Prismor {version}.exe"))
    dst_vst3 = os.path.join(staging_dir, f"Prismor {version}.vst3")
    shutil.copytree(vst3_dir, dst_vst3)

    latest_zip = os.path.join(PROJECT_ROOT, "Prismor_Latest.zip")
    versioned_zip = os.path.join(PROJECT_ROOT, f"Prismor_v{version}.zip")

    if os.path.exists(latest_zip):
        os.remove(latest_zip)
    if os.path.exists(versioned_zip):
        os.remove(versioned_zip)

    print(f"Packaging {latest_zip}...")
    shutil.make_archive(os.path.splitext(latest_zip)[0], 'zip', staging_dir)
    shutil.copy2(latest_zip, versioned_zip)

    gh = find_gh_cli()
    tag = f"v{version}"
    title = f"Prismor v{version}"
    notes = (
        f"Automated build release for Prismor v{version}.\n\n"
        f"### Download Assets\n"
        f"- **Permanent Latest Download**: [Prismor_Latest.zip](https://github.com/znagersu/Prismor/releases/latest/download/Prismor_Latest.zip)\n"
        f"- **Release Archive**: [Prismor_v{version}.zip](https://github.com/znagersu/Prismor/releases/download/{tag}/Prismor_v{version}.zip)\n\n"
        f"Includes Standalone .exe and VST3 64-bit plugin."
    )

    print(f"Publishing release {tag} with GitHub CLI...")
    check_cmd = [gh, "release", "view", tag, "--repo", "znagersu/Prismor"]
    res = subprocess.run(check_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if res.returncode == 0:
        subprocess.run([gh, "release", "upload", tag, latest_zip, versioned_zip, "--repo", "znagersu/Prismor", "--clobber"], check=True)
        subprocess.run([gh, "release", "edit", tag, "--title", title, "--notes", notes, "--latest", "--repo", "znagersu/Prismor"], check=True)
    else:
        subprocess.run([gh, "release", "create", tag, latest_zip, versioned_zip, "--repo", "znagersu/Prismor", "--title", title, "--notes", notes, "--latest"], check=True)

    print(f"\n=== SUCCESS: Release {tag} published to GitHub ===")
    print("Permanent Static Download URL:")
    print("https://github.com/znagersu/Prismor/releases/latest/download/Prismor_Latest.zip\n")

if __name__ == "__main__":
    main()
