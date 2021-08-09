#pragma once

#define UILOG_LEVEL_TRACE 0
#define UILOG_LEVEL_DEBUG 1
#define UILOG_LEVEL_INFO 2
#define UILOG_LEVEL_WARN 3
#define UILOG_LEVEL_ERROR 4
#define UILOG_LEVEL_CRIT 5
#define UILOG_LEVEL_DISABLE 1024

#if defined(UITK_SAMPLE_DEVELOPER)
#ifndef UILOG_MIN_LEVEL
#define UILOG_MIN_LEVEL UILOG_LEVEL_DEBUG
#endif
#elif defined(UITK_DEVELOPER)
#define UILOG_MIN_LEVEL UILOG_LEVEL_TRACE
#else
#define UILOG_MIN_LEVEL UILOG_LEVEL_ERROR
#endif

#define UILOG_COMPILE_TESTS_ENABLE 0

#define UI_CRIT_ACTION

#ifndef UITK_ENABLE_DEBUGDRAW
#define UITK_ENABLE_DEBUGDRAW 0
#endif

// Allow generated names if a style { } object does not contain an "id" field
// If this is allowed, anonymously id'd styles will use classId + a_number
#ifndef UI_ALLOW_ANONYMOUS_STYLES
#define UI_ALLOW_ANONYMOUS_STYLES 0
#endif

#ifndef UI_EXTRA_DEBUG_CHECKS
#define UI_EXTRA_DEBUG_CHECKS 0
#endif

#ifndef UI_STRICT
#define UI_STRICT 0
#endif

#if defined(_GAMING_DESKTOP) && defined(USE_PIX) && defined(UI_USE_PIX_PROFILING)
#error PIX Profiling in UITK requires Pix3. https://devblogs.microsoft.com/pix/winpixeventruntime/
#else
#if !defined(PIXScopedEvent)
#define PIXScopedEvent(...)
#endif
#endif
