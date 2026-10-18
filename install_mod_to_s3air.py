#!/usr/bin/env python3
"""
Script de instalación e integración para Sonic Classic Heroes AIR.
Enlaza automáticamente este repositorio a la carpeta de Mods oficial de Sonic 3 A.I.R.
en Windows (%APPDATA%\\Sonic3AIR\\mods).
"""

import os
import sys
import shutil
import subprocess

if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

def get_s3air_mods_dir():
    appdata = os.environ.get("APPDATA")
    if appdata:
        target = os.path.join(appdata, "Sonic3AIR", "mods")
        return target
    return None

def main():
    print("=" * 65)
    print("  🦔 Instalador Automático de Sonic Classic Heroes AIR")
    print("=" * 65)

    source_dir = os.path.dirname(os.path.abspath(__file__))
    mods_dir = get_s3air_mods_dir()

    if not mods_dir:
        print("❌ No se pudo determinar la ruta de %APPDATA%.")
        return 1

    print(f"📁 Directorio de origen: {source_dir}")
    print(f"🎯 Carpeta destino de mods: {mods_dir}")

    # Asegurar que la carpeta mods exista
    os.makedirs(mods_dir, exist_ok=True)

    dest_mod_path = os.path.join(mods_dir, "sonic-classic-heroes-air")

    # Si ya existe enlace o carpeta
    if os.path.exists(dest_mod_path):
        print(f"ℹ️ Ya existe una versión en {dest_mod_path}. Actualizando...")
        try:
            if os.path.islink(dest_mod_path):
                os.unlink(dest_mod_path)
            elif os.path.isdir(dest_mod_path):
                # Comprobar si es un junction de Windows
                try:
                    os.rmdir(dest_mod_path)
                except OSError:
                    shutil.rmtree(dest_mod_path)
        except Exception as e:
            print(f"⚠️ Nota al limpiar anterior: {e}")

    # Intentar crear un Junction (enlace de directorio de Windows) para sincronización en tiempo real
    success = False
    try:
        # mklink /J en Windows no requiere permisos de administrador
        cmd = f'cmd /c mklink /J "{dest_mod_path}" "{source_dir}"'
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if res.returncode == 0:
            print("✅ ¡Enlace Junction creado con éxito! Los cambios se sincronizarán en vivo.")
            success = True
        else:
            print(f"ℹ️ Falló mklink ({res.stderr.strip()}), usando copia de archivos...")
    except Exception as e:
        print(f"ℹ️ Error con enlace: {e}")

    if not success:
        # Fallback a copia directa
        shutil.copytree(source_dir, dest_mod_path, ignore=shutil.ignore_patterns(".git", "__pycache__"))
        print("✅ ¡Archivos del mod copiados con éxito a la carpeta de mods!")

    print("\n🎮 ¡Listo! Ahora cuando abras Sonic 3 A.I.R.:")
    print("   1. Ve al menú principal -> 'Mods'")
    print("   2. Activa 'Sonic Classic Heroes AIR'")
    print("   3. Opciones del mod: Options -> pestaña 'Mods' -> 'Classic Heroes'")
    print("   4. Inicia una partida y cambia de líder con R / L (o Y en el suelo)")
    print("=" * 65)
    return 0

if __name__ == "__main__":
    sys.exit(main())
