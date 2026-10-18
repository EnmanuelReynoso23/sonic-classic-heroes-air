#!/usr/bin/env python3
"""Builds the installable mod zip: a single folder "sonic-classic-heroes-air" with only the files the game needs."""

import json
import os
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FOLDER = "sonic-classic-heroes-air"
FILES = ["mod.json", "icon.png", "icon-64px.png", "icon-16px.png", "README.md", "LICENSE"]


def main():
    version = json.load(open(os.path.join(ROOT, "mod.json"), encoding="utf-8"))["Metadata"]["ModVersion"]
    out_dir = os.path.join(ROOT, "dist")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"{FOLDER}-{version}.zip")

    with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in FILES:
            archive.write(os.path.join(ROOT, name), f"{FOLDER}/{name}")
        for base, _, files in os.walk(os.path.join(ROOT, "scripts")):
            for name in sorted(files):
                path = os.path.join(base, name)
                archive.write(path, f"{FOLDER}/" + os.path.relpath(path, ROOT).replace(os.sep, "/"))

    print(out_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
