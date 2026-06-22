#!/usr/bin/env python3
"""Find vcxproj files that reference HarfBuzz or FreeType2.

Scans for .vcxproj files and reports which ones contain references to
HarfBuzz and/or FreeType libraries.

Usage:
    python find_harfbuzz_freetype.py              # Scan current directory
    python find_harfbuzz_freetype.py <path>        # Scan specified directory
"""

import os
import sys
from pathlib import Path


SKIP_DIRS = {'_Deprecated', '.git', '.vs', 'packages'}


def scan_file(path):
    """Scan a vcxproj file for HarfBuzz/FreeType references.

    Skips AdditionalIncludeDirectories elements.
    Returns a tuple (has_harfbuzz, has_freetype).
    """
    try:
        lines = path.read_text(encoding='utf-8-sig', errors='replace').splitlines()
    except OSError as e:
        print(f'  WARNING: Could not read {path}: {e}', file=sys.stderr)
        return False, False

    has_hb = False
    has_ft = False
    for line in lines:
        if 'AdditionalIncludeDirectories' in line:
            continue
        low = line.lower()
        if not has_hb and 'harfbuzz' in low:
            has_hb = True
        if not has_ft and 'freetype' in low:
            has_ft = True
        if has_hb and has_ft:
            break

    return has_hb, has_ft


def main():
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    if not root.is_dir():
        print(f'Error: {root} is not a directory', file=sys.stderr)
        sys.exit(1)

    print(f'Scanning: {root}')
    print()

    scanned = 0
    matches = []

    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]

        for fname in filenames:
            if not fname.endswith('.vcxproj'):
                continue

            fpath = Path(dirpath) / fname
            scanned += 1
            has_hb, has_ft = scan_file(fpath)

            if has_hb or has_ft:
                libs = []
                if has_hb:
                    libs.append('HarfBuzz')
                if has_ft:
                    libs.append('FreeType')
                label = ' + '.join(libs)
                rel = fpath.relative_to(root)
                print(f'  [{label}] {rel}')
                matches.append((str(rel), has_hb, has_ft))

    print()
    print('--- Summary ---')
    print(f'  Files scanned:  {scanned}')
    print(f'  Files matched:  {len(matches)}')
    hb_count = sum(1 for _, hb, _ in matches if hb)
    ft_count = sum(1 for _, _, ft in matches if ft)
    print(f'    HarfBuzz:     {hb_count}')
    print(f'    FreeType:     {ft_count}')


if __name__ == '__main__':
    main()
