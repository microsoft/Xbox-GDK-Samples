//--------------------------------------------------------------------------------------
// AsyncHelper.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <functional>
#include <utility>

#include <XAsync.h>

namespace ATG
{
    //Create AsyncHelper object using new
    //Pass in your custom lambda callback with any lambda capture you want
    //Call the platform function using the AsyncHelper object's XAsyncBlock
    //Make sure to delete the async helper object on failure.

    struct AsyncHelper
    {
        AsyncHelper(AsyncHelper&&) = default;
        AsyncHelper& operator=(AsyncHelper&&) = default;

        AsyncHelper(const AsyncHelper&) = delete;
        AsyncHelper& operator=(const AsyncHelper&) = delete;

        XAsyncBlock asyncBlock{};
        std::function<void(XAsyncBlock* asyncBlock)> fn;

        explicit AsyncHelper(XTaskQueueHandle queue, std::function<void(XAsyncBlock*)> fn) :
            fn(std::move(fn))
        {
            asyncBlock.queue = queue;
            asyncBlock.context = this;
            asyncBlock.callback = [](XAsyncBlock* async)
            {
                auto op = static_cast<AsyncHelper*>(async->context);

                op->fn(async);

                delete op;
            };
        }
    };
}
