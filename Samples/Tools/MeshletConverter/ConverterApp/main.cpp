//--------------------------------------------------------------------------------------
// main.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "Importer.h"

#include <iostream>
#include <string>

using namespace ATG;

namespace
{
    void PrintHelp()
    {
        std::cout << std::endl;
        std::cout << "---------------------------- ATG Meshlet Converter ----------------------------" << std::endl;
        std::cout << "This tool generates a meshlet structure from meshes in FBX or OBJ file formats." << std::endl;
        std::cout << "The meshlet structures are packed into a '.bin' file which is placed alongside" << std::endl;
        std::cout << "the input file. No vertex data is modified or exported with the meshlet data, " << std::endl;
        std::cout << "thus the mesh is required to be triangulated." << std::endl;
        std::cout << std::endl;

        std::cout << "Usage:" << std::endl;
        std::cout << "\t<string list> -- Specifies paths to the .fbx or .obj files to process." << std::endl;
        std::cout << std::endl;

        std::cout << "Switches:" << std::endl;
        std::cout << "\t-h            -- Display this help message." << std::endl;
        std::cout << "\t-v <int>      -- Specifies the maximum vertex count of a meshlet. Must be less than 256. Default is 128" << std::endl;
        std::cout << "\t-p <int>      -- Specifies the maximum primitive count of a meshlet. Must be less than 256. Default is 128" << std::endl;
        std::cout << "\t-s <float>    -- Specifies a global scaling factor for scene geometry. Default is 1.0" << std::endl;
        std::cout << "\t-i            -- Forces vertex indices to be 32 bits, even if only 16 bits are required. Default is false" << std::endl;
        std::cout << "\t-fz           -- Flips the Z axis of the scene geometry. Default is false" << std::endl;
        std::cout << "\t-ft           -- Flips the triangle winding of the scene geometry. Default is false" << std::endl;
        std::cout << "\t-t            -- Triangulates scene meshes file using FbxGeometryConverter. Default is false" << std::endl;
        std::cout << std::endl;

        std::cout << "Example:" << std::endl;
        std::cout << "\tConverterApp.exe -v 256 -p 256 -i -f -t Path/To/MyFile1.fbx Path/To/MyFile2.fbx " << std::endl;
        std::cout << std::endl;
    }

    bool ParseCommandLine(int argc, const char* args[], std::vector<std::string>& files, ImportOptions& options)
    {
        if (argc < 2)
        {
            PrintHelp();
            return false;
        }

        for (int i = 1; i < argc; ++i)
        {
            if (std::strcmp(args[i], "-h") == 0)
            {
                PrintHelp();
                return false;
            }
            else if (std::strcmp(args[i], "-v") == 0)
            {
                if (i + 1 == argc)
                {
                    std::cout << "Must provide an integral value for meshlet max vertex count if supplying -v switch." << std::endl;
                    return false;
                }

                uint32_t maxSize = std::strtoul(args[++i], nullptr, 10);
                uint32_t adjSize = min(max(maxSize, 32u), 256u);

                if (maxSize != adjSize)
                {
                    std::cout << "Meshlet max vertex count must be between 32 and 256, inclusively." << std::endl;
                    std::cout << "Specified: " << maxSize << ", Adjusted: " << adjSize << std::endl;

                    maxSize = adjSize;
                }

                options.MeshletMaxVerts = maxSize;
            }
            else if (std::strcmp(args[i], "-p") == 0)
            {
                if (i + 1 == argc)
                {
                    std::cout << "Must provide an integral value for meshlet max primitive count if supplying -p switch." << std::endl;
                    return false;
                }

                uint32_t maxSize = std::strtoul(args[++i], nullptr, 10);
                uint32_t adjSize = min(max(maxSize, 32u), 256u);

                if (maxSize != adjSize)
                {
                    std::cout << "Meshlet max primitive count must be between 32 and 256, inclusively." << std::endl;
                    std::cout << "Specified: " << maxSize << ", Adjusted: " << adjSize << std::endl;

                    maxSize = adjSize;
                }

                options.MeshletMaxPrims = maxSize;
            }
            else if (std::strcmp(args[i], "-s") == 0)
            {
                if (i + 1 == argc)
                {
                    std::cout << "Must provide a float value for scaling factor if supplying -s switch." << std::endl;
                    return false;
                }

                float scale = std::strtof(args[++i], nullptr);
                if (isnan(scale))
                    scale = 1.0f;

                options.UnitScale = scale;
            }
            else if (std::strcmp(args[i], "-i") == 0)
            {
                std::cout << "Forcing vertex indices to 32 bits." << std::endl;
                options.Force32BitIndices = true;
            }
            else if (std::strcmp(args[i], "-t") == 0)
            {
                std::cout << "Triangulating scene meshes." << std::endl;
                options.TriangulateMeshes = true;
            }
            else if (std::strcmp(args[i], "-fz") == 0)
            {
                std::cout << "Flipping the Z axis." << std::endl;
                options.FlipZ = true;
            }
            else if (std::strcmp(args[i], "-ft") == 0)
            {
                std::cout << "Flipping triangle winding order." << std::endl;
                options.FlipTriangles = true;
            }
            else
            {
                files.push_back(args[i]);
            }
        }

        std::cout << "Using meshlet size - Vertices: " << options.MeshletMaxVerts << "   Primitives: " << options.MeshletMaxPrims <<  std::endl;
        std::cout << "Using global scale factor - " << options.UnitScale << std::endl;

        return true;
    }
}

int main(int argc, const char* args[])
{
    std::vector<std::string> files;

    ImportOptions options;
    ParseCommandLine(argc, args, files, options);

    std::vector<MeshletSet> meshlets;

    bool allSucceeded = true;
    for (auto& filename : files)
    {
        std::cout << std::endl;

        auto loc = filename.find_last_of(".");
        auto fileType = filename.substr(loc + 1);
        bool success = false;
        if (fileType.compare("sdkmesh") == 0)
        {
            success = ImportFileSDKMesh(filename.c_str(), options, meshlets);
        }
        else
        {
            success = ImportFile(filename.c_str(), options, meshlets);
        }

        if (success)
        {
            
            auto path = filename.substr(0, loc) + ".bin";

            if (MeshletSet::Write(path.c_str(), meshlets))
            {
                std::cout << "Wrote " << meshlets.size() << " set(s) of meshlets from file \"" << filename << "\"." << std::endl;
            }
            else
            {
                std::cout << "Failed to open file \"" << path << "\" for writing." << std::endl;
                allSucceeded = false;
            }
        }
    }

    return allSucceeded ? 0 : 1;
}
