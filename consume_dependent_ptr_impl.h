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

#ifndef consume_dependent_ptr_impl_h
#define consume_dependent_ptr_impl_h

template<typename T> inline dependent_ptr<T>::dependent_ptr(T* ptr) : ptr(ptr) {}
template<typename T> inline dependent_ptr<T>::dependent_ptr(std::nullptr_t ptr) : ptr(ptr) {}

template<typename T> inline dependent_ptr<T>::dependent_ptr(T* ptr, class dependency d) : ptr(reinterpret_cast<T*>((d | dependent_ptr<T>(ptr)) | reinterpret_cast<uintptr_t>(ptr))) {}
template<typename T> inline dependent_ptr<T>::dependent_ptr(std::nullptr_t ptr, class dependency d) : dependent_ptr(static_cast<T*>(ptr), d) {}
template<typename T> inline dependent_ptr<T>::dependent_ptr(dependent<uintptr_t> d) : dependent_ptr(reinterpret_cast<T*>(d.value), d.dependency) {}
template<typename T> inline dependent_ptr<T>::dependent_ptr(dependent<intptr_t> d) : dependent_ptr(reinterpret_cast<T*>(d.value), d.dependency) {}

template<typename T> inline dependent_ptr<T>::dependent_ptr(const dependent_ptr<T>& rhs) : ptr(rhs.ptr) {}

template<typename T> inline dependent_ptr<T>& dependent_ptr<T>::operator=(T* rhs) { ptr = rhs; return *this; }
template<typename T> inline dependent_ptr<T>& dependent_ptr<T>::operator=(std::nullptr_t rhs) { ptr = rhs; return *this; }

template<typename T> inline dependent_ptr<T>& dependent_ptr<T>::operator=(const dependent_ptr<T>& rhs) { ptr = rhs; return *this; }

template<typename T> inline dependent<uintptr_t> dependent_ptr<T>::to_uintptr_t() const { return dependent<uintptr_t>(reinterpret_cast<uintptr_t>(ptr)); }
template<typename T> inline dependent<intptr_t> dependent_ptr<T>::to_intptr_t() const { return dependent<intptr_t>(reinterpret_cast<intptr_t>(ptr)); }

template<typename T> inline dependent<T> dependent_ptr<T>::operator[](size_t offset) const { return dependent<T>(*(ptr + offset)); }

template<typename T> inline T* dependent_ptr<T>::operator->() const { return ptr; }

template<typename T> inline dependent<T> dependent_ptr<T>::operator*() const { using shadowed = class dependency; return dependent<T>(*ptr, shadowed(ptr)); }

template<typename T> inline dependent_ptr<T*> dependent_ptr<T>::operator&() const { return dependent_ptr<T*>(&ptr); }

template<typename T> inline T* dependent_ptr<T>::value() const { return ptr; }

template<typename T> inline dependency dependent_ptr<T>::dep() const { return dep(ptr); }

#endif
