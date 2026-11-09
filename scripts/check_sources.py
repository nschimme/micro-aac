#!/usr/bin/env python3
"""Verify cmake/sources.cmake against upstream FAAC's Meson build.

Meson (upstream) is the source of truth for which files make up libfaac and
libfaad3. This script introspects it and compares with the hand-maintained
list in cmake/sources.cmake, so a submodule bump that adds, removes or renames
a source fails CI instead of breaking consumers. Meson is a dev/CI tool only.

Usage: scripts/check_sources.py [--faac-dir lib/faac]
"""
import argparse, json, pathlib, re, subprocess, sys, tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Upstream sources we intentionally do not compile, with the reason.
IGNORED = {
    "drc.c": "encoder DRC not used",
    "drc_analyzer.c": "encoder DRC not used",
}
# libfaad files built through forwarder copies in src/ (name collisions with
# libfaac under PlatformIO ESP-IDF): upstream path -> forwarder in src/.
FORWARDED = {
    "libfaad/sbr.c": "src/faad_sbr.c",
    "libfaad/stereo.c": "src/faad_stereo.c",
    "libfaad/tns.c": "src/faad_tns.c",
}


def upstream_sources(faac_dir):
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["meson", "setup", tmp, str(faac_dir), "-Dmax-channels=2"], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        out = subprocess.run(["meson", "introspect", tmp, "--targets"],
                             check=True, capture_output=True, text=True).stdout
    want = set()
    for t in json.loads(out):
        if t["type"] != "static library" or t["name"] not in ("faac", "faad3"):
            continue
        for s in t["target_sources"]:
            for f in s.get("sources", []):
                if f.endswith(".c"):
                    p = pathlib.Path(f).resolve().relative_to(faac_dir.resolve())
                    want.add(p.as_posix())
    return want


def cmake_sources():
    text = (ROOT / "cmake/sources.cmake").read_text()
    have = set(re.findall(r'^\s+"\$\{FAAC_DIR\}/([\w/.]+\.c)"', text, re.M))
    local = set(re.findall(r'\$\{CMAKE_CURRENT_SOURCE_DIR\}/(src/[\w.]+\.c)"', text))
    return have, local


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--faac-dir", default=str(ROOT / "lib/faac"))
    faac_dir = pathlib.Path(ap.parse_args().faac_dir)
    if not (faac_dir / "meson.build").exists():
        sys.exit(f"{faac_dir} is empty: run `git submodule update --init`")

    want = upstream_sources(faac_dir)
    have, local = cmake_sources()
    ignored = {"libfaac/" + k for k in IGNORED}
    errors = []
    # Built only for max-channels >= 3, so absent from the default (2) introspection.
    if "libfaad/coupling.c" not in (ROOT / "cmake/sources.cmake").read_text() \
            or "GREATER 2" not in (ROOT / "cmake/sources.cmake").read_text():
        errors.append("sources.cmake must add libfaad/coupling.c when MAX_CHANNELS > 2")
    for f in sorted(want - have - ignored - set(FORWARDED)):
        errors.append(f"upstream builds {f} but sources.cmake does not list it")
    for f in sorted(have - want):
        errors.append(f"sources.cmake lists {f} which upstream no longer builds")
    for f in sorted(have):
        if not (faac_dir / f).exists():
            errors.append(f"sources.cmake lists {f} which does not exist")
    for up, fw in FORWARDED.items():
        if up not in want:
            errors.append(f"forwarded {up} no longer built upstream; drop {fw}")
        if fw not in local:
            errors.append(f"{fw} (forwarder for {up}) missing from sources.cmake")
        if not (ROOT / fw).exists():
            errors.append(f"{fw} does not exist")
    if errors:
        print("\n".join("ERROR: " + e for e in errors))
        sys.exit(1)
    print(f"OK: {len(want)} upstream sources accounted for")


main()
