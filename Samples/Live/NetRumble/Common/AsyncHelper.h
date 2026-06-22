//--------------------------------------------------------------------------------------
// AsyncHelper.h
//
// Helper function + opbject that allows for capturing lambdas to be using when
// calling *Async methods in the Microsoft GDK.  These helpers also manage the lifetime
// of the XAsyncBlock to reduce the number of memory leaks by forgetting to
// clean them up in all code paths.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <XAsync.h>

namespace ATG
{
    template <typename T>
    struct AsyncData
    {
        XAsyncBlock async;
        T callback;

        operator XAsyncBlock* () { return &async; }

        static AsyncData* Create(T&& cb, XTaskQueueHandle queue) noexcept
        {
            return new(std::nothrow) AsyncData(std::forward<T&&>(cb), std::forward<XTaskQueueHandle>(queue));
        }
    private:
        AsyncData(T&& cb, XTaskQueueHandle queue) noexcept :
            async{ queue, this, Invoke, {} },
            callback{ cb }
        {
        }

        static void Invoke(XAsyncBlock* async) noexcept
        {
            auto data = reinterpret_cast<AsyncData<T>*>(async->context);

            data->callback(async);

            delete data;
        }
    };

    template <typename Args> 
    struct AsyncInvokerHelper
    {
        static_assert(std::is_pointer_v<Args> && std::is_function_v<Args>, "Template parameter must be pointer to function.");
    };

    template <typename... Args>
    struct AsyncInvokerHelper<HRESULT(Args...)>
    {
        using ptr_type = HRESULT(*)(Args...);
        using arg_tuple = std::tuple<Args...>;
        static constexpr auto arity = sizeof...(Args);

        template <typename Is> struct Invoker;
        template <std::size_t... Is>
        struct Invoker<std::index_sequence<Is...>>
        {
            ptr_type f;

            template <typename CompletionCallback>
            HRESULT Invoke(typename std::tuple_element<Is, arg_tuple>::type... args, CompletionCallback &&callback, XTaskQueueHandle queue) const noexcept
            {
                static_assert(std::is_invocable_v<CompletionCallback, XAsyncBlock*>, "Completion Callback must have signature: \"void onCompletion(XAsyncBlock *)\"");

                auto data = AsyncData<CompletionCallback>::Create(std::forward<CompletionCallback&&>(callback), queue);

                HRESULT hr = f(args..., *data);

                if (FAILED(hr))
                {
                    delete data;
                }

                return hr;
            }

            template <typename CompletionCallback, typename ErrorCallback>
            HRESULT Invoke(typename std::tuple_element<Is, arg_tuple>::type... args, CompletionCallback &&callback, ErrorCallback &&onError, XTaskQueueHandle queue) const noexcept
            {
                static_assert(std::is_invocable_v<CompletionCallback, XAsyncBlock*>, "Completion Callback must have signature: \"void onCompletion(XAsyncBlock *)\"");
                static_assert(std::is_invocable_v<ErrorCallback, HRESULT>, "Error Callback must have signature: \"void onError(HRESULT)\"");

                auto data = AsyncData<CompletionCallback>::Create(std::forward<CompletionCallback&&>(callback), queue);

                HRESULT hr = f(args..., *data);

                if (FAILED(hr))
                {
                    onError(hr);
                    delete data;
                }

                return hr;
            }
        };

        using type = Invoker<std::make_index_sequence<sizeof...(Args) - 1>>;
    };
    template<class... Args> struct AsyncInvokerHelper<HRESULT(__stdcall *)(Args...)> : public AsyncInvokerHelper<HRESULT(Args...)> {};
    template<class... Args> struct AsyncInvokerHelper<HRESULT(__stdcall *)(Args...) noexcept> : public AsyncInvokerHelper<HRESULT(Args...)> {};

    template <typename T, typename invoker = typename AsyncInvokerHelper<typename std::decay<T>::type>::type>
    inline invoker AsyncHelper(T && fn) noexcept
    {
        return invoker{ fn };
    }

    template <typename AsyncWork>
    HRESULT RunAsync(AsyncWork &&callback, XTaskQueueHandle queue, XTaskQueuePort port)
    {
        static_assert(std::is_invocable<AsyncWork, bool>::value, "AsyncWork must have signature: \"void callback(bool cancelled)\"");

        auto callback_persistant = new AsyncWork{ std::move(callback) };

        HRESULT hr = XTaskQueueSubmitCallback(queue, port, callback_persistant, [](void * context, bool cancelled)
            {
                auto callback_persistant = reinterpret_cast<AsyncWork*>(context);

                (*callback_persistant)(cancelled);

                delete callback_persistant;
            });


        if (FAILED(hr))
        {
            delete callback_persistant;
        }
    }

    template <typename AsyncWork>
    HRESULT DelayedRunAsync(AsyncWork &&callback, uint32_t delayMs, XTaskQueueHandle queue, XTaskQueuePort port)
    {
        static_assert(std::is_invocable<AsyncWork, bool>::value, "AsyncWork must have signature: \"void callback(bool cancelled)\"");

        auto callback_persistant = new AsyncWork{ std::move(callback) };

        HRESULT hr = XTaskQueueSubmitDelayedCallback(queue, port, delayMs, callback_persistant, [](void * context, bool cancelled)
            {
                auto callback_persistant = reinterpret_cast<AsyncWork*>(context);

                (*callback_persistant)(cancelled);

                delete callback_persistant;
            });


        if (FAILED(hr))
        {
            delete callback_persistant;
        }
    }
}
