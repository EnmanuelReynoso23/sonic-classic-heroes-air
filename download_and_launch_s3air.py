#!/usr/bin/env python3
"""
🚀 Sonic 3 A.I.R. Portable Setup & Launcher
Permite descargar, configurar y ejecutar Sonic 3 A.I.R. en modo desarrollador
con el mod Sonic Classic Heroes AIR preinstalado y activo.
"""

import os
import sys
import zipfile
import urllib.request
import subprocess

if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

PORTABLE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "sonic3air")
MOD_DIR = os.path.dirname(os.path.abspath(__file__))

def check_existing_install():
    possible_paths = [
        os.path.join(PORTABLE_DIR, "Sonic3AIR.exe"),
        os.path.expandvars(r"%LOCALAPPDATA%\Sonic3AIR\Sonic3AIR.exe"),
        os.path.expandvars(r"%ProgramFiles%\Sonic3AIR\Sonic3AIR.exe"),
    ]
    for p in possible_paths:
        if os.path.exists(p):
            return p
    return None

def download_sonic3air():
    print(f"📦 Descargando Sonic 3 A.I.R. (Portable Windows)...")
    url = "https://github.com/Eukaryot/sonic3air/releases/download/v24.02.02.0/sonic3air_game.zip"
    zip_path = os.path.join(os.path.dirname(PORTABLE_DIR), "sonic3air_game.zip")
    
    os.makedirs(os.path.dirname(PORTABLE_DIR), exist_ok=True)
    
    headers = {'User-Agent': 'Mozilla/5.0'}
    req = urllib.request.Request(url, headers=headers)
    
    try:
        with urllib.request.urlopen(req) as resp, open(zip_path, "wb") as out_file:
            total_size = int(resp.info().get('Content-Length', 0))
            downloaded = 0
            while True:
                chunk = resp.read(65536)
                if not chunk:
                    break
                out_file.write(chunk)
                downloaded += len(chunk)
                if total_size > 0:
                    pct = (downloaded / total_size) * 100
                    print(f"\rProgreso: {pct:.1f}% ({downloaded // (1024*1024)} MB)", end="")
        print("\n✅ Descarga completada.")
    except Exception as e:
        print(f"\n❌ Error durante la descarga automática: {e}")
        print("💡 Puedes descargar 'sonic3air_game.zip' manualmente desde https://sonic3air.org/")
        return None

    print(f"📂 Extrayendo en: {PORTABLE_DIR}...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(PORTABLE_DIR)
        os.remove(zip_path)
        print("✅ Extracción exitosa.")
        return os.path.join(PORTABLE_DIR, "Sonic3AIR.exe")
    except Exception as e:
        print(f"❌ Error al descomprimir: {e}")
        return None

def link_mod_to_portable(exe_path):
    portable_base = os.path.dirname(exe_path)
    mods_target = os.path.join(portable_base, "data", "mods", "sonic-classic-heroes-air")
    os.makedirs(os.path.dirname(mods_target), exist_ok=True)

    if not os.path.exists(mods_target):
        try:
            cmd = f'cmd /c mklink /J "{mods_target}" "{MOD_DIR}"'
            subprocess.run(cmd, shell=True, capture_output=True)
            print("✅ Mod enlazado directamente a la versión portátil.")
        except Exception:
            pass

def main():
    print("=" * 65)
    print("  🦔 Sonic 3 A.I.R. - Lanzador de Sonic Classic Heroes")
    print("=" * 65)

    exe_path = check_existing_install()
    if not exe_path:
        print("🔍 No se detectó instalación previa en rutas habituales.")
        exe_path = download_sonic3air()

    if exe_path and os.path.exists(exe_path):
        print(f"🎮 Ejecutable localizado: {exe_path}")
        link_mod_to_portable(exe_path)
        
        print("\n🚀 Lanzando Sonic 3 A.I.R. en modo desarrollador...")
        print("   (Presiona Ctrl+C en esta terminal para detener si lo deseas)")
        subprocess.Popen([exe_path, "--developer"], cwd=os.path.dirname(exe_path))
    else:
        print("⚠️ No se pudo iniciar Sonic 3 A.I.R.")
        print("Por favor, asegúrate de colocar Sonic3AIR.exe en:")
        print(f"   {PORTABLE_DIR}")

if __name__ == "__main__":
    main()
