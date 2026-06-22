# ATG GameCore Samples

Xbox Advanced Technology Group (ATG) DirectX 12 samples for GameCore on Xbox Series X|S, Xbox One, and Windows PC (x64/ARM64). These samples demonstrate graphics, audio, networking, system, and Live integration techniques for game developers using the Microsoft Game Development Kit (GDK).

Copyright (c) Microsoft Corporation. All rights reserved.

## Repository layout

| Folder | Contents |
|--------|----------|
| `Samples/` | Sample source code organized by category (Audio, Graphics, Handheld, IntroGraphics, Live, System, Tools) |
| `Kits/` | Shared toolkits (DirectXTK12, ATGTK, LiveTK, UITK, DirectXTex, DirectXMesh) |
| `Media/` | Shared assets (textures, models, fonts) managed via Git LFS |
| `Infrastructure/` | Build system, CI pipelines, and project templates |

## Getting started

### Prerequisites

- **Visual Studio 2022** (or later) with the C++ game development workload
- **Microsoft Game Development Kit (GDK)**
- **Git LFS 3.x** (required for cloning this repo)

### Git LFS setup (read before cloning!)

This repo uses [Git LFS (Large File Storage)](https://git-lfs.com/) to manage binary assets in the `Media/` folder.

> **Note:** Git LFS is only compatible with HTTPS cloning. Do not use SSH or the clone will fail.

[Git for Windows](https://gitforwindows.org/) includes Git LFS by default. To verify your installation:

```
$ git lfs version
git-lfs/3.7.1 (GitHub; windows amd64; go 1.25.1; git b84b3384)

$ git lfs install
Git LFS initialized.
```

If `git lfs install` was not previously run, this command registers the hooks needed to download LFS-tracked files during checkout.

### Cloning the repo

1. Clone using HTTPS:
   ```
   git clone https://xbox-atg.visualstudio.com/DefaultCollection/ATG%20Sample%20Development-Git/_git/gx_dev
   ```
2. After cloning, verify that `Media/` contains actual asset files (not small text pointer files). If you see pointer files, Git LFS is not installed correctly.

### Building a sample

Open any sample `.sln` in Visual Studio, or from an ATG Developer Command Prompt:

```
msbuild SampleName.sln /p:Configuration=Debug /p:Platform="Gaming.Xbox.Scarlett.x64"
```

Supported platforms: `Gaming.Xbox.Scarlett.x64`, `Gaming.Xbox.XboxOne.x64`, `x64`, `ARM64`.

## Documentation

See the [ATG Samples Wiki](Wiki/overview.md) for detailed guides and API references.

## Support

For questions or issues, contact [atgsv@microsoft.com](mailto:atgsv@microsoft.com).
