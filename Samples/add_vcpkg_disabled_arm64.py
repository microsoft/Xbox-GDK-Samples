#!/usr/bin/env python3
"""Add VcpkgEnabled=false to vcxproj files that have ARM64 configurations.

Scans for .vcxproj files containing ARM64 platform configurations and adds
<VcpkgEnabled>false</VcpkgEnabled> in a new PropertyGroup just before the
first ItemDefinitionGroup, if not already present.

Usage:
    python add_vcpkg_disabled_arm64.py              # Dry run (default)
    python add_vcpkg_disabled_arm64.py --apply      # Apply changes
    python add_vcpkg_disabled_arm64.py -v           # Verbose dry run
"""

import argparse
import os
import re
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

SKIP_DIRS = {'_Deprecated', '.git', '.vs', 'packages'}

VCPKG_BLOCK = [
    '  <PropertyGroup>',
    '    <VcpkgEnabled>false</VcpkgEnabled>',
    '  </PropertyGroup>',
]


# ---------------------------------------------------------------------------
# Detection helpers
# ---------------------------------------------------------------------------

def has_arm64_config(lines):
    """Check if the vcxproj has ARM64 platform configurations."""
    for line in lines:
        if 'ARM64' in line and 'ProjectConfiguration' in line:
            return True
    return False


def has_vcpkg_disabled(lines):
    """Check if VcpkgEnabled is already set to false anywhere in the file."""
    for line in lines:
        stripped = line.strip()
        if stripped == '<VcpkgEnabled>false</VcpkgEnabled>':
            return True
    return False


def find_first_item_definition_group(lines):
    """Return the line index of the first <ItemDefinitionGroup line, or -1."""
    for i, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith('<ItemDefinitionGroup'):
            return i
    return -1


# ---------------------------------------------------------------------------
# File I/O helpers (preserve encoding and line endings)
# ---------------------------------------------------------------------------

def read_file(path):
    """Read file, returning (lines, encoding, line_ending).

    Lines are returned without line endings so we can re-join with the
    original ending character(s).
    """
    raw = path.read_bytes()

    # Detect BOM / encoding
    if raw.startswith(b'\xef\xbb\xbf'):
        encoding = 'utf-8-sig'
    elif raw.startswith(b'\xff\xfe'):
        encoding = 'utf-16-le'
    elif raw.startswith(b'\xfe\xff'):
        encoding = 'utf-16-be'
    else:
        encoding = 'utf-8'

    text = raw.decode(encoding)

    # Detect line ending
    if '\r\n' in text:
        eol = '\r\n'
    elif '\r' in text:
        eol = '\r'
    else:
        eol = '\n'

    lines = text.split(eol)
    # If file ends with a line ending, split produces a trailing empty string
    trailing_eol = text.endswith(eol)

    return lines, encoding, eol, trailing_eol


def write_file(path, lines, encoding, eol, trailing_eol):
    """Write lines back to file preserving encoding and line endings."""
    text = eol.join(lines)
    if trailing_eol and not text.endswith(eol):
        text += eol
    path.write_bytes(text.encode(encoding))


# ---------------------------------------------------------------------------
# Core logic
# ---------------------------------------------------------------------------

def process_vcxproj(path, apply, verbose):
    """Process a single vcxproj file. Returns a status string."""
    lines, encoding, eol, trailing_eol = read_file(path)

    if not has_arm64_config(lines):
        if verbose:
            print(f'  SKIP (no ARM64): {path}')
        return 'no_arm64'

    if has_vcpkg_disabled(lines):
        if verbose:
            print(f'  SKIP (already has VcpkgEnabled=false): {path}')
        return 'already_set'

    insert_idx = find_first_item_definition_group(lines)
    if insert_idx == -1:
        print(f'  WARNING: ARM64 config found but no ItemDefinitionGroup in: {path}')
        return 'no_idg'

    # Insert the VcpkgEnabled PropertyGroup before the first ItemDefinitionGroup
    new_lines = lines[:insert_idx] + VCPKG_BLOCK + [''] + lines[insert_idx:]

    if apply:
        write_file(path, new_lines, encoding, eol, trailing_eol)
        print(f'  MODIFIED: {path}')
    else:
        print(f'  WOULD MODIFY: {path} (insert before line {insert_idx + 1})')

    return 'modified'


def main():
    parser = argparse.ArgumentParser(
        description='Add VcpkgEnabled=false to ARM64 vcxproj files.')
    parser.add_argument('--apply', action='store_true',
                        help='Apply changes (default is dry run)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Show skipped files')
    parser.add_argument('root', nargs='?', default='.',
                        help='Root directory to scan (default: current dir)')
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f'Error: {root} is not a directory', file=sys.stderr)
        sys.exit(1)

    print(f'Scanning: {root}')
    print(f'Mode: {"APPLY" if args.apply else "DRY RUN"}')
    print()

    counts = {
        'scanned': 0,
        'no_arm64': 0,
        'already_set': 0,
        'modified': 0,
        'no_idg': 0,
    }

    for dirpath, dirnames, filenames in os.walk(root):
        # Skip excluded directories
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]

        for fname in filenames:
            if not fname.endswith('.vcxproj'):
                continue

            fpath = Path(dirpath) / fname
            counts['scanned'] += 1
            status = process_vcxproj(fpath, args.apply, args.verbose)
            counts[status] += 1

    print()
    print('--- Summary ---')
    print(f'  Files scanned:          {counts["scanned"]}')
    print(f'  No ARM64 config:        {counts["no_arm64"]}')
    print(f'  Already has VcpkgEnabled=false: {counts["already_set"]}')
    print(f'  {"Modified" if args.apply else "Would modify"}:             {counts["modified"]}')
    if counts['no_idg']:
        print(f'  WARNING (no ItemDefinitionGroup): {counts["no_idg"]}')


if __name__ == '__main__':
    main()
