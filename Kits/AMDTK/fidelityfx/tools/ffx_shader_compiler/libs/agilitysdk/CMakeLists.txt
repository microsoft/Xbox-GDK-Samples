# This file is part of the FidelityFX SDK.
# 
# Copyright (C) 2024 Advanced Micro Devices, Inc.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

add_library(agilitysdk INTERFACE)


set_property(TARGET agilitysdk PROPERTY VS_PACKAGE_REFERENCES "Microsoft.Direct3D.D3D12_1.615.1")

set(AGILITY_SDK_DIR ${CMAKE_SOURCE_DIR}/packages/Microsoft.Direct3D.D3D12.1.615.1/build/native)

target_include_directories(agilitysdk INTERFACE BEFORE ${AGILITY_SDK_DIR}/include/)

set(agilitysdk_binaries
    ${AGILITY_SDK_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/D3D12Core.dll
    ${AGILITY_SDK_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/d3d12SDKLayers.dll
    ${AGILITY_SDK_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}/d3dconfig.exe)

copyTargetCommand("${agilitysdk_binaries}" ${CMAKE_HOME_DIRECTORY}/bin/Debug copied_agilitysdk_bin_debug)
copyTargetCommand("${agilitysdk_binaries}" ${CMAKE_HOME_DIRECTORY}/bin/Release copied_agilitysdk_bin_release)
add_dependencies(agilitysdk copied_agilitysdk_bin_debug copied_agilitysdk_bin_release)
