#!/usr/bin/env python3
"""Add ARM64 platform configurations to vcxproj and sln files.

Scans for .vcxproj and .sln files containing x64 or Gaming.Desktop.x64
configurations and adds corresponding ARM64 configurations.

Usage:
    python add_arm64_config.py              # Dry run (default)
    python add_arm64_config.py --apply      # Apply changes
    python add_arm64_config.py -v           # Verbose dry run
"""

import argparse
import os
import re
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def is_x64_ref(text):
    """True if text references x64 or Gaming.Desktop.x64 (not Gaming.Xbox)."""
    if 'Gaming.Xbox' in text:
        return False
    return '|x64' in text or 'Gaming.Desktop.x64' in text or "=='x64'" in text


def transform_vcxproj_line(line):
    """Clone a vcxproj line, replacing x64/Gaming.Desktop.x64 → ARM64.

    Carefully avoids changing PreferredToolArchitecture or other non-platform
    occurrences of 'x64'.
    """
    r = line
    # Longer match first to avoid partial replacement
    r = r.replace('Gaming.Desktop.x64', 'ARM64')
    # Condition / Include attribute patterns: |x64' and |x64"
    r = r.replace("|x64'", "|ARM64'")
    r = r.replace('|x64"', '|ARM64"')
    # Standalone platform equality: =='x64'
    r = r.replace("=='x64'", "=='ARM64'")
    # <Platform> element text
    r = r.replace('<Platform>x64</Platform>', '<Platform>ARM64</Platform>')
    # Library / binary directory paths (backslash before x64)
    r = re.sub(r'\\x64(?=[\\;"<])', r'\\ARM64', r)
    # Forward-slash paths (less common but possible)
    r = re.sub(r'/x64(?=[/;])', '/ARM64', r)
    return r


def transform_sln_line(line):
    """Clone a sln line, replacing x64/Gaming.Desktop.x64 → ARM64."""
    if 'Gaming.Xbox' in line:
        return line
    r = line.replace('Gaming.Desktop.x64', 'ARM64')
    r = r.replace('|x64', '|ARM64')
    return r


def find_block_end(lines, start, tag):
    """Return the line index of </tag> matching the opening at *start*."""
    close_tag = f'</{tag}>'

    # Same-line close or self-closing element
    if close_tag in lines[start] or lines[start].rstrip().endswith('/>'):
        return start

    depth = 1
    for i in range(start + 1, len(lines)):
        ln = lines[i]
        # Nested opening (not self-closing, not same-line close)
        if f'<{tag}' in ln and '/>' not in ln.rstrip() and close_tag not in ln:
            depth += 1
        if close_tag in ln:
            depth -= 1
            if depth == 0:
                return i
    return start  # fallback – should not happen in well-formed XML


def read_file(filepath):
    """Read file bytes; return (text, has_bom, line_ending)."""
    with open(filepath, 'rb') as f:
        raw = f.read()
    has_bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig')
    line_ending = '\r\n' if '\r\n' in text else '\n'
    return text, has_bom, line_ending


def write_file(filepath, text, has_bom):
    """Write text to file, preserving BOM if originally present."""
    with open(filepath, 'wb') as f:
        if has_bom:
            f.write(b'\xef\xbb\xbf')
        f.write(text.encode('utf-8'))


# ---------------------------------------------------------------------------
# vcxproj processing
# ---------------------------------------------------------------------------

# Tags that appear as direct children of <Project> with Condition attributes
_BLOCK_TAGS = ('PropertyGroup', 'ImportGroup', 'ItemDefinitionGroup')


def process_vcxproj(filepath, dry_run=True):
    """Add ARM64 configurations to a single .vcxproj file.

    Returns the number of insertions made (0 if nothing to do).
    """
    text, has_bom, le = read_file(filepath)

    # Fast-path checks
    if '|ARM64' in text:
        return 0
    if not any(is_x64_ref(l) for l in text.split('\n')):
        return 0

    lines = text.split(le)
    insertions = []  # list of (after_line_idx, [new_lines])

    i = 0
    while i < len(lines):
        stripped = lines[i].strip()

        # Never touch Gaming.Xbox-conditioned lines
        if 'Gaming.Xbox' in lines[i]:
            i += 1
            continue

        # --- ProjectConfiguration items ---
        if ('<ProjectConfiguration' in stripped
                and 'Include=' in stripped
                and is_x64_ref(stripped)):
            end = find_block_end(lines, i, 'ProjectConfiguration')
            block = [transform_vcxproj_line(l) for l in lines[i:end + 1]]
            insertions.append((end, block))
            i = end + 1
            continue

        # --- Top-level conditioned blocks (PropertyGroup, ImportGroup, …) ---
        matched = False
        for tag in _BLOCK_TAGS:
            if (stripped.startswith(f'<{tag}')
                    and 'Condition=' in stripped
                    and is_x64_ref(stripped)):
                end = find_block_end(lines, i, tag)
                block = [transform_vcxproj_line(l) for l in lines[i:end + 1]]
                insertions.append((end, block))
                i = end + 1
                matched = True
                break
        if matched:
            continue

        # --- Platform-conditioned ItemGroups ---
        if (stripped.startswith('<ItemGroup')
                and 'Condition=' in stripped
                and is_x64_ref(stripped)):
            end = find_block_end(lines, i, 'ItemGroup')
            block = [transform_vcxproj_line(l) for l in lines[i:end + 1]]
            insertions.append((end, block))
            i = end + 1
            continue

        # --- Per-file / per-item conditions (child element attributes) ---
        if 'Condition=' in stripped and is_x64_ref(stripped):
            is_block = any(stripped.startswith(f'<{t}')
                           for t in (*_BLOCK_TAGS, 'ItemGroup',
                                     'ProjectConfiguration', 'Project'))
            if not is_block:
                new_line = transform_vcxproj_line(lines[i])
                insertions.append((i, [new_line]))

        i += 1

    if not insertions:
        return 0

    # Insert from bottom to top so indices stay valid
    insertions.sort(key=lambda x: x[0], reverse=True)
    for after_idx, new_lines in insertions:
        for j, nl in enumerate(new_lines):
            lines.insert(after_idx + 1 + j, nl)

    if not dry_run:
        write_file(filepath, le.join(lines), has_bom)

    return len(insertions)


# ---------------------------------------------------------------------------
# sln processing
# ---------------------------------------------------------------------------

def process_sln(filepath, dry_run=True):
    """Add ARM64 configurations to a single .sln file.

    Returns the number of ARM64 entries added (0 if nothing to do).
    """
    text, has_bom, le = read_file(filepath)

    if '|ARM64' in text:
        return 0
    if not any(is_x64_ref(l) for l in text.split('\n')):
        return 0

    lines = text.split(le)
    insertions = []
    in_config = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        if ('GlobalSection(SolutionConfigurationPlatforms)' in stripped
                or 'GlobalSection(ProjectConfigurationPlatforms)' in stripped):
            in_config = True
            continue
        if 'EndGlobalSection' in stripped:
            in_config = False
            continue

        if in_config and is_x64_ref(stripped):
            new_line = transform_sln_line(line)
            if new_line != line:
                insertions.append((i, [new_line]))

    if not insertions:
        return 0

    # Deduplicate identical generated lines (e.g. x64 + Gaming.Desktop.x64
    # both mapping to the same ARM64 entry in SolutionConfigurationPlatforms)
    seen = set()
    unique = []
    for idx, new_lines in insertions:
        key = tuple(l.strip() for l in new_lines)
        if key not in seen:
            seen.add(key)
            unique.append((idx, new_lines))
    insertions = unique

    # Insert bottom-to-top
    insertions.sort(key=lambda x: x[0], reverse=True)
    for after_idx, new_lines in insertions:
        for j, nl in enumerate(new_lines):
            lines.insert(after_idx + 1 + j, nl)

    if not dry_run:
        write_file(filepath, le.join(lines), has_bom)

    return len(insertions)


# ---------------------------------------------------------------------------
# File discovery
# ---------------------------------------------------------------------------

def find_files(root, pattern, exclude_dirs=None):
    """Recursively find files matching *pattern*, skipping *exclude_dirs*."""
    exclude_dirs = set(exclude_dirs or [])
    results = []
    for path in Path(root).rglob(pattern):
        if not (exclude_dirs & set(path.parts)):
            results.append(str(path))
    return sorted(results)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Add ARM64 platform configurations to vcxproj and sln files.')
    parser.add_argument('--apply', action='store_true',
                        help='Apply changes (default is dry-run)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='List every file that would be / was modified')
    parser.add_argument('--root', default='.',
                        help='Root directory to scan (default: .)')
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    dry_run = not args.apply
    mode = 'DRY RUN' if dry_run else 'APPLYING CHANGES'
    print(f'=== {mode} ===')
    if dry_run:
        print('(use --apply to write changes)\n')
    else:
        print()

    # Discover files
    vcxproj_files = find_files(root, '*.vcxproj', exclude_dirs=['_Deprecated'])
    sln_files = find_files(root, '*.sln', exclude_dirs=['_Deprecated'])
    print(f'Scanning {len(vcxproj_files)} .vcxproj and {len(sln_files)} .sln files ...\n')

    # --- Process vcxproj ---
    vcx_changed = 0
    vcx_insertions = 0
    for fp in vcxproj_files:
        n = process_vcxproj(fp, dry_run=dry_run)
        if n:
            vcx_changed += 1
            vcx_insertions += n
            if args.verbose:
                verb = 'Would modify' if dry_run else 'Modified'
                print(f'  {verb}: {os.path.relpath(fp, root)}  ({n} insertions)')

    # --- Process sln ---
    sln_changed = 0
    sln_entries = 0
    for fp in sln_files:
        n = process_sln(fp, dry_run=dry_run)
        if n:
            sln_changed += 1
            sln_entries += n
            if args.verbose:
                verb = 'Would modify' if dry_run else 'Modified'
                print(f'  {verb}: {os.path.relpath(fp, root)}  ({n} ARM64 entries)')

    # --- Summary ---
    print()
    print('--- Summary ---')
    verb = 'Would modify' if dry_run else 'Modified'
    print(f'  vcxproj: {verb} {vcx_changed}/{len(vcxproj_files)} files  '
          f'({vcx_insertions} insertions)')
    print(f'  sln:     {verb} {sln_changed}/{len(sln_files)} files  '
          f'({sln_entries} ARM64 entries)')

    if dry_run and (vcx_changed or sln_changed):
        print('\nRun with --apply to write changes.')

    return 0


if __name__ == '__main__':
    sys.exit(main())
