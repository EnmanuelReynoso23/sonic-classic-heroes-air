#!/usr/bin/env python3
"""
Sonic 3 A.I.R. - descarga, instalacion del mod y arranque.

1. Busca Sonic3AIR.exe; si no esta, descarga la ultima version estable oficial
   desde https://github.com/Eukaryot/sonic3air/releases (Windows 64 bits).
2. Enlaza este mod en %APPDATA%\\Sonic3AIR\\mods (install_mod_to_s3air.py).
3. Abre el juego.

La ROM no se descarga: Sonic 3 A.I.R. necesita tu propia copia de
"Sonic 3 & Knuckles" (Sonic_Knuckles_wSonic3.bin, p. ej. la de Steam).
La primera vez que abras el juego te la pide.
"""

import json
import os
import subprocess
import sys
import urllib.request
import zipfile

if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

MOD_DIR = os.path.dirname(os.path.abspath(__file__))
GAME_DIR = os.path.join(os.path.dirname(MOD_DIR), "sonic3air")
RELEASES_API = "https://api.github.com/repos/Eukaryot/sonic3air/releases/latest"
ASSET_NAME = "sonic3air_game_64bit.zip"


def find_game():
    candidates = [
        os.path.join(GAME_DIR, "Sonic3AIR.exe"),
        os.path.join(GAME_DIR, "sonic3air_game", "Sonic3AIR.exe"),
        os.path.expandvars(r"%USERPROFILE%\sonic-dev\game\sonic3air_game\Sonic3AIR.exe"),
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None


def download_game():
    print("Buscando la ultima version estable de Sonic 3 A.I.R....")
    request = urllib.request.Request(RELEASES_API, headers={"User-Agent": "sonic-classic-heroes-air"})
    with urllib.request.urlopen(request) as response:
        release = json.load(response)

    asset = next((a for a in release.get("assets", []) if a["name"] == ASSET_NAME), None)
    if asset is None:
        print(f"No se encontro {ASSET_NAME} en {release.get('tag_name')}. Descargalo a mano desde https://sonic3air.org")
        return None

    os.makedirs(GAME_DIR, exist_ok=True)
    zip_path = os.path.join(GAME_DIR, ASSET_NAME)
    print(f"Descargando {release['tag_name']} ({asset['size'] // (1024 * 1024)} MB)...")
    urllib.request.urlretrieve(asset["browser_download_url"], zip_path)

    with zipfile.ZipFile(zip_path) as archive:
        archive.extractall(GAME_DIR)
    os.remove(zip_path)
    return find_game()


def main():
    exe = find_game() or download_game()
    if exe is None:
        return 1
    print(f"Juego: {exe}")

    # Enlazar el mod en la carpeta de mods del juego
    subprocess.run([sys.executable, os.path.join(MOD_DIR, "install_mod_to_s3air.py")], check=False)

    print("Abriendo Sonic 3 A.I.R....")
    print("Activa el mod en el menu principal -> Mods, si todavia no lo esta.")
    subprocess.Popen([exe], cwd=os.path.dirname(exe))
    return 0


if __name__ == "__main__":
    sys.exit(main())
