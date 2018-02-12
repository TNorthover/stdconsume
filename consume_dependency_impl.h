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

#ifndef consume_dependency_impl_h
#define consume_dependency_impl_h

namespace {

template<typename T, typename std::enable_if<sizeof(T) == 8>::type* = nullptr>
inline dependency::dependency_type __create_dependency(T value) {
    dependency::dependency_type dep;
#if CPU(ARM64)
    asm volatile("eor %w[dep], %w[in], %w[in]" : [dep] "=r"(dep) : [in] "r"(bit_cast<uint64_t>(value)));
#elif CPU(ARM)
    asm volatile("eor %[dep], %[in], %[in]" : [dep] "=r"(dep) : [in] "r"(bit_cast<uint64_t>(value)));
#elif CPU(X86) || CPU(X86_64)
    dep = bit_cast<uint64_t>(value) ^ bit_cast<uint64_t>(value); // Any zero will do for x86
    std::atomic_signal_fence(std::memory_order_acquire);
#elif CPU(PPC64)
    asm volatile("xor %w[dep], %w[in], %w[in]" : [dep] "=r"(dep) : [in] "r"(bit_cast<uint64_t>(value)));
#else
#error Architecture unsupported.
#endif
    return dep;
}

template<typename T, typename std::enable_if<sizeof(T) == 4>::type* = nullptr>
inline dependency::dependency_type __create_dependency(T value) {
    dependency::dependency_type dep;
#if CPU(ARM64)
    asm volatile("eor %w[dep], %w[in], %w[in]" : [dep] "=r"(dep) : [in] "r"(bit_cast<uint32_t>(value)));
#elif CPU(ARM)
    asm volatile("eor %[dep], %[in], %[in]" : [dep] "=r"(dep) : [in] "r"(bit_cast<uint32_t>(value)));
#elif CPU(X86) || CPU(X86_64)
    dep = bit_cast<uint32_t>(value) ^ bit_cast<uint32_t>(value); // Any zero will do for x86.
    std::atomic_signal_fence(std::memory_order_acquire);
#elif defined(__PPC64__)
    asm volatile("xor %[dep], %[in], %[in]" : [dep] "=r"(dep) : [in] "r"(bit_cast<uint32_t>(value)));
#else
#error Architecture unsupported.
#endif
    return dep;
}

template <typename T, typename std::enable_if<sizeof(T) == 2>::type* = nullptr>
inline dependency::dependency_type __create_dependency(T value) { return __create_dependency(static_cast<uint32_t>(value)); }

template <typename T, typename std::enable_if<sizeof(T) == 1>::type* = nullptr>
inline dependency::dependency_type __create_dependency(T value) { return __create_dependency(static_cast<uint32_t>(value)); }

} // anonymous namespace

template<typename T> dependency::dependency(T value) : dep(__create_dependency(value)) {}

inline dependency dependency::operator|(dependency d) { return dep + d.dep; }

template<typename T> inline dependency operator|(dependency lhs, dependent_ptr<T> rhs) { return lhs.dep + dependency(rhs.value()).dep; }
template<typename T> inline dependency operator|(dependent_ptr<T> lhs, dependency rhs) { return dependency(lhs.value()).dep + rhs.dep; }

inline uintptr_t operator|(dependency lhs, uintptr_t rhs) { return lhs.dep | rhs; }
inline uintptr_t operator|(uintptr_t lhs, dependency rhs) { return lhs | rhs.dep; }
inline intptr_t operator|(dependency lhs, intptr_t rhs) { return lhs.dep | rhs; }
inline intptr_t operator|(intptr_t lhs, dependency rhs) { return lhs | rhs.dep; }

#endif
