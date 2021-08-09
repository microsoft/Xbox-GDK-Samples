#pragma once

#ifndef DISABLE_PLAYFABENTITY_API

#include <playfab/PlayFabError.h>
#include <playfab/PlayFabEventsDataModels.h>

// This file contains declaration of base interfaces for any custom playstream events
// as well as PlayFab-specific implementations

namespace PlayFab
{
    /// <summary>
    /// The enumeration of all possible "types" of events
    /// </summary>
    enum class PlayFabEventType
    {
        Default, // Default type (e.g. the one set by global configuration)
        Lightweight, // Event will be sent using WriteTelemetryEvents method in EventsAPI
        Heavyweight // Event will be sent using WriteEvents method in EventsAPI
    };

    /// <summary>
    /// Possible outcomes of "emit event" operations
    /// </summary>
    enum class EmitEventResult
    {
        Success, // An event was successfully emitted
        Overflow, // An event wasn't emitted because the emitter capacity is full
        Disabled, // An event wasn't emitted because the emitter is disabled (its functionality is "turned off")
        NotSupported // An event wasn't emitted because the emitter doesn't support the operation
    };

    /// <summary>
    /// Interface for any event
    /// </summary>
    class IPlayFabEvent
    {
    public:
        virtual ~IPlayFabEvent() {}
    };

    /// <summary>
    /// PlayFab-specific implementation of an event
    /// </summary>
    class PlayFabEvent : public IPlayFabEvent
    {
    public:
        PlayFabEvent();
        void SetName(const std::string& eventName); // Sets the event name
        const std::string& GetName() const; // Gets the event name
        void SetNamespace(const std::string& eventNamespace); // Sets the event namespace
        void SetEntity(const EventsModels::EntityKey& entity); // Set EntityToken
        void SetProperty(const std::string& name, const std::string& value); // Sets a value of a string property by name
        void SetProperty(const std::string& name, const bool value); // Sets a value of a bool property by name
        void SetProperty(const std::string& name, const int8_t value); // Sets a value of a int8_t property by name
        void SetProperty(const std::string& name, const int16_t value); // Sets a value of a int16_t property by name
        void SetProperty(const std::string& name, const int32_t value); // Sets a value of a int32_t property by name
        void SetProperty(const std::string& name, const int64_t value); // Sets a value of a int64_t property by name
        void SetProperty(const std::string& name, const uint8_t value); // Sets a value of a uint8_t property by name
        void SetProperty(const std::string& name, const uint16_t value); // Sets a value of a uint16_t property by name
        void SetProperty(const std::string& name, const uint32_t value); // Sets a value of a uint32_t property by name
        void SetProperty(const std::string& name, const uint64_t value); // Sets a value of a uint64_t property by name
        void SetProperty(const std::string& name, const double value); // Sets a value of a double property by name

    public:
        PlayFabEventType eventType;
    private:
        EventsModels::EventContents eventContents;

        friend class PlayFabEventPipeline; // to access eventContents directly from PF pipeline for perf optimization
    };

    /// <summary>
    /// Interface for any emit event request
    /// </summary>
    class IPlayFabEmitEventRequest
    {
    public:
        virtual ~IPlayFabEmitEventRequest() {}
    };

    /// <summary>
    /// Interface for any emit event response
    /// </summary>
    class IPlayFabEmitEventResponse
    {
    public:
        virtual ~IPlayFabEmitEventResponse() {}
    };

    // A callback that can be used in asynchronous emit event operations that take IPlayFabEvent as a parameter
    // and return back an IPlayFabEmitEventResponse. The callback procedure must be thread-safe.
    using PlayFabEmitEventCallback = void(*)(std::shared_ptr<const IPlayFabEvent>, std::shared_ptr<const IPlayFabEmitEventResponse>);

    /// <summary>
    /// PlayFab-specific implementation of an emit event request
    /// </summary>
    class PlayFabEmitEventRequest : public IPlayFabEmitEventRequest
    {
    public:
        std::shared_ptr<const PlayFabEvent> event; // a pointer to the user's event object itself
        PlayFabEmitEventCallback callback; // user's callback function to return the final result of emit event operation after event is completely sent out or any error occurred
        std::function<void(std::shared_ptr<const IPlayFabEvent>, std::shared_ptr<const IPlayFabEmitEventResponse>)> stdCallback; // same as EventCallback but can be used with member variables if needed.
    };

    /// <summary>
    /// PlayFab-specific implementation of an emit event response
    /// </summary>
    class PlayFabEmitEventResponse : public IPlayFabEmitEventResponse
    {
    public:
        EmitEventResult emitEventResult; // result of immediate "emit event" operation
        std::shared_ptr<const PlayFabError> playFabError; // error information and/or operation result
        std::shared_ptr<const EventsModels::WriteEventsResponse> writeEventsResponse; // additional data with the outcome of the operation
        std::shared_ptr<const std::vector<std::shared_ptr<const IPlayFabEmitEventRequest>>> batch; // the batch this event was part of
        size_t batchNumber; // the incremental batch number
    };
}

#endif
