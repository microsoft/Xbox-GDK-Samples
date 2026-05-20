# Copilot Instructions for PR Summaries

When generating a pull request summary for this repository, follow the format used
in our release notes. The summary should help reviewers quickly understand what
changed, which samples were affected, and any infrastructure/kit updates.

## Required Format

Use this structure for every PR summary:

```
## <Brief descriptive title of the change>

**<One-sentence bold summary>** explaining the purpose and scope of the PR.

### Changes
- **New samples:** <list any newly added samples, or omit this line if none>
- **Updated samples:** <list samples that were modified, or omit if none>
- **Updated kits:** <list any kit/library updates (e.g., ATGTK, OpenSource/imgui), or omit if none>
- **General:** <any other notable changes — config updates, build changes, docs, removals>
```

## Formatting Rules

- Use an H2 (`##`) heading for the PR title/theme
- The first line after the heading must be a **bold summary sentence**
- Use a bulleted list under a `### Changes` subheading
- Each bullet should have a **bold category prefix** followed by a colon
- Sample and kit names should be wrapped in **bold**
- Omit category lines that have no items (do not include empty categories)
- Keep the summary concise — aim for clarity over length

## Category Detection

Determine categories by inspecting the diff:
- Files under `Samples/` that are newly added → **New samples**
- Files under `Samples/` that are modified → **Updated samples**
- Files under `Kits/` that are modified → **Updated kits** (ATGTK or OpenSource/imgui)
- Changes to build files (`.vcxproj`, `.sln`), configs, docs, `Media/`, or vcpkg manifests → **General**

## Examples

Good summary:
```
## ARM64 Build Target Support

**Added ARM64 (Desktop) build configurations across all samples** to enable building and running on ARM64-based Windows PCs.

### Changes
- **Updated samples:** All samples (ARM64 platform targets added to project files)
- **Updated kits:** ATGTK
- **General:** Updated `.sln` solution files with ARM64 platform entries, updated vcpkg configuration
```
