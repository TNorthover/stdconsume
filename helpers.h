/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef helpers_h
#define helpers_h

#include <atomic>
#include <cstring>
#include <type_traits>

#define COMPILER_FENCE() std::atomic_signal_fence(std::memory_order_seq_cst)

// bit_cast from http://wg21.link/P0476

#define BIT_CAST_STATIC_ASSERTS(TO, FROM) do {                          \
        static_assert(sizeof(TO) == sizeof(FROM));                      \
        static_assert(std::is_trivially_copyable<TO>::value);           \
        static_assert(std::is_trivially_copyable<FROM>::value);         \
    } while (false)

template<typename To, typename From>
inline To bit_cast(const From& from) noexcept {
    BIT_CAST_STATIC_ASSERTS(To, From);
    typename std::aligned_storage<sizeof(To), alignof(To)>::type storage;
    std::memcpy(&storage, &from, sizeof(To));
    return reinterpret_cast<To&>(storage);
}

// From WTF:

#define CPU(WTF_FEATURE) (defined WTF_CPU_##WTF_FEATURE  && WTF_CPU_##WTF_FEATURE)
#if defined(__i386__) || defined(i386)  || defined(_M_IX86)  || defined(_X86_)  || defined(__THW_INTEL)
#define WTF_CPU_X86 1
#endif
#if defined(__x86_64__) || defined(_M_X64)
#define WTF_CPU_X86_64 1
#endif
#if (defined(__arm64__) && defined(__APPLE__)) || defined(__aarch64__)
#define WTF_CPU_ARM64 1
#endif
#if defined(arm)  || defined(__arm__)  || defined(ARM)  || defined(_ARM_)
#define WTF_CPU_ARM 1
#endif
#if defined(__PPC64__)
#define WTF_CPU_PPC64 1
#endif

#define COMPILER(WTF_FEATURE) (defined WTF_COMPILER_##WTF_FEATURE  && WTF_COMPILER_##WTF_FEATURE)
#if defined(__GNUC__)
#define WTF_COMPILER_GCC_OR_CLANG 1
#endif
#if COMPILER(GCC_OR_CLANG) && !COMPILER(CLANG)
#define WTF_COMPILER_GCC 1
#endif
#if defined(_MSC_VER)
#define WTF_COMPILER_MSVC 1
#endif

#if !defined(NEVER_INLINE) && COMPILER(GCC_OR_CLANG)
#define NEVER_INLINE __attribute__((__noinline__))
#endif
#if !defined(NEVER_INLINE) && COMPILER(MSVC)
#define NEVER_INLINE __declspec(noinline)
#endif

#if !defined(UNLIKELY) && COMPILER(GCC_OR_CLANG)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
#if !defined(UNLIKELY)
#define UNLIKELY(x) (x)
#endif

#endif
