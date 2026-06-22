//--------------------------------------------------------------------------------------
// precomp.hpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define PROFILE 1
#include "..\Common\precomp.hpp"
#if defined(_XBOX_ONE) && defined(_TITLE)
#else
#include <dxgi1_4.h>
#endif

#pragma warning (disable: 4311 4312 4838 )

#include "TiledResourcesTier.h"