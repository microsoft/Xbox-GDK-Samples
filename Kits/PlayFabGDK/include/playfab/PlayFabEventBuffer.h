#pragma once

#include <playfab/PlayFabSpinLock.h>
#include <playfab/PlayFabEvent.h>

namespace PlayFab
{
#pragma pack(push)
#pragma pack(8) // conservative memory alignment control to provide maximum platform compatibility
    /// <summary>
    /// A "packet" (wrapper) for an event request. The packets are internal custom allocations inside the buffer.
    /// Their purpose is to reduce heap churn as additional data about event (e.g. some event metadata) needs to be stored
    /// so that a wrapper object is not created on the heap for each event.
    /// The implementation is based on the implementation of FullEvent struct in Microsoft Gaming Cloud CELL library.
    /// </summary>
    class PlayFabEventPacket final
    {
    public:
        PlayFabEventPacket(const uint64_t index, const std::shared_ptr<const IPlayFabEmitEventRequest>& request) :
            next(nullptr),
            eventIndex(index),
            timestamp(std::time(nullptr)), // current time
            eventRequest(std::move(request))
        {
        }
        ~PlayFabEventPacket() {};

        std::atomic<PlayFabEventPacket*> next; // a pointer to the next event packet in buffer
        uint64_t eventIndex; // the incremental index of an event
        std::time_t timestamp; // the timestamp of event packet creation

        std::shared_ptr<const IPlayFabEmitEventRequest> eventRequest; // the event request
    };
#pragma pack(pop)

    /// <summary>
    /// Default PlayFab event buffer (a spinlock-enforced MPSC queue based on a circular buffer).
    /// The implementation is based on the implementation of CircularBuffer in Microsoft Gaming Cloud CELL library.
    /// </summary>
    class PlayFabEventBuffer final
    {
    public:
        /// <summary>
        /// Possible outcomes of "put event in buffer" operations
        /// </summary>
        enum class EventProducingResult
        {
            Success, // An event was successfully put in buffer
            Overflow, // An event wasn't put in buffer because it is full
            Disabled // An event wasn't put in buffer because the buffer is disabled (buffering functionality is "turned off")
        };

        /// Possible outcomes of "take event from buffer" operations
        enum class EventConsumingResult
        {
            Success, // An event was successfully taken from buffer
            Empty, // An event wasn't taken from buffer because it is empty
            Disabled // An event wasn't taken from buffer because the buffer is disabled (buffering functionality is "turned off")
        };

        explicit PlayFabEventBuffer(const size_t bufferSize);
        ~PlayFabEventBuffer();

        PlayFabEventBuffer(const PlayFabEventBuffer& source) = delete; // disable copy
        PlayFabEventBuffer(PlayFabEventBuffer&&) = delete; // disable move
        PlayFabEventBuffer& operator=(const PlayFabEventBuffer& source) = delete; // disable assignment
        PlayFabEventBuffer& operator=(PlayFabEventBuffer&& other) = delete; // disable move assignment

        // Attempts to put an event in buffer (add to the tail). This method must be thread-safe.
        EventProducingResult TryPut(std::shared_ptr<const IPlayFabEmitEventRequest> request);

        // Attempts to take an event from buffer (update the head).
        EventConsumingResult TryTake(std::shared_ptr<const IPlayFabEmitEventRequest>& request);

    private:
        // Creates (allocates and calls constructor) an event packet object in the buffer
        PlayFabEventPacket* CreateEventPacket(uint8_t* location, const uint64_t index, std::shared_ptr<const IPlayFabEmitEventRequest> request);

        // Deletes (calls destructor) an event packet object from the buffer
        void DeleteEventPacket(PlayFabEventPacket* eventPacket);

        AtomicSpin atomicSpin; // Used for spinlock in critical sections
        std::atomic<bool> disabled; // A state flag indicating whether the buffer is disabled/enabled

        const size_t buffMask; // A bit mask that is used for very fast buffer pointer arithmetics.
                               // The buffer's length is always a power of two and the buffer mask is (length - 1).
                               // For example if buffer length is 0x100 (256) then buffer mask is 0xFF (255)
                               // (or 100000000 and 011111111 in binary form). Performing binary "&" operations
                               // with a mask like that allows for efficient pointer adjustments in a circular buffer.

        std::unique_ptr<uint8_t[]> bufferArray;
        const uint64_t buffStart; // A pointer to the beginning of buffer (first byte of the buffer).
        const uint64_t buffEnd; // A pointer to the byte immediately after the last byte of buffer, i.e. (buffEnd - buffStart) gives the length of the buffer.

        std::atomic<PlayFabEventPacket*> head; // A pointer to the last consumed event packet. The last consumed event packet must always exist (not destroyed yet) by design,
                                               // as it may be the same event packet pointed to by tail (e.g. when there are no events currently in the buffer).
                                               // Events are consumed from the head (head is moving towards tail as events are consumed).

        // Variables below are touched only behind the lock:

        PlayFabEventPacket* tail; // A pointer to the last event packet added to the buffer.
                                  // Newly added events are added to the tail (tail is growing as new events are added).

        std::shared_ptr<std::atomic<uint64_t>> eventIndex; // The counter of produced events
    };
}
