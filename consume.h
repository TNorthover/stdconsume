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

#ifndef consume_h
#define consume_h

#include <atomic>
#include <cstddef>
#include <cstdint>
#include "helpers.h"

/*

# p0750r0 consume

## Background

Memory order consume as specified in C++11 hasn't been implemented by any
compiler. The main reason for this shortcoming is that the specification creates
a fundamental mismatch between the model of "dependency" at the C++ source code
level and "dependency" at the ISA-specific assembly level. The goal of consume
was to expose what ARM calls the "address dependency rule" as explained in
section 6.3 of [the ARM Barrier Litmus Tests and
Cookbook](http://infocenter.arm.com/help/topic/com.arm.doc.genc007826/Barrier_Litmus_Tests_and_Cookbook_A08.pdf). POWER
has a similar ISA feature. On ISAs such as ARM and POWER consume allows writing
release / acquire style code, where the reader side doesn't need extra
fences. Ordering is guaranteed by creating dependencies on the reader side
between the released location and the subsequent reads that should be observed
to happen after that location's value was written by the writer. A "dependency"
at the ISA level means that computation of the value of the address of
subsequent loads depend on the value loaded from the release location.

Despite not being implemented by any C++11 compiler, consume-style code is
widely used in software. The best known example of such usage is Linux's [Read
Copy Update](https://www.kernel.org/doc/Documentation/RCU/whatisRCU.txt). To
maintain correctness, these uses rely on:

  - A gentleperson's agreement with compiler authors to avoid breaking code;
  - Human inspection of compiled code disassembly;
  - Extensive testing;
  - Judicious application of inline assembly; and
  - Luck 🍀.

This proposal continuing work from:

  - [Proposal for New memory order consume Definition](http://wg21.link/p0190r3)
  - [Marking memory order consume Dependency Chains](https://wg21.link/p0462r1)

It is different from prior proposals in that it contains a single proposed API,
which is known to work for at least one code base. Indeed, it is based on [work
in
WebKit](https://trac.webkit.org/browser/webkit/trunk/Source/WTF/wtf/Atomics.h?rev=+217722#L342)
which has been successfully using consume ordering in C++ on ARM platforms for a
while.

The proposed API for C++ is substantially different from the WebKit API:

 - The proposal has fewer sharp edges than the WebKit one; and
 - The proposal attempts to support more usecases as documented by p0462r1


## Wording as of C++17

<blockquote>
    ❡7 An evaluation A carries a dependency to an evaluation B if

    <ol>
    <li>the value of A is used as an operand of B, unless:
    <ol>
        <li>B is an invocation of any specialization of `std::kill_dependency`, or
        <li>A is the left operand of a built-in logical AND `&&` or logical OR `||` operator, or
        <li>A is the left operand of a conditional `?:` operator, or
        <li>A is the left operand of the built-in comma `,` operator; or
    </ol>
    <li>A writes a scalar object or bit-field M, B reads the value written by A from M, and A is sequenced before B, or
    <li>for some evaluation X, A carries a dependency to X, and X carries a dependency to B.
    </ol>

    [ Note: “Carries a dependency to” is a subset of “is sequenced before”, and
      is similarly strictly intra-thread. — end note ]

    ❡8 An evaluation A is dependency-ordered before an evaluation B if

    <ol>
    <li>A performs a release operation on an atomic object M, and, in another
        thread, B performs a consume operation on M and reads a value written
        by any side effect in the release sequence headed by A, or
    <li>for some evaluation X, A is dependency-ordered before X and X carries
        a dependency to B.
    </ol>

    [ Note: The relation “is dependency-ordered before” is analogous to
      “synchronizes with”, but uses release/consume in place of release/acquire.
      — end note ]
</blockquote>

The above wording and subsequent uses of it terms likely need to be updated.


## Other considerations

The following can also be considered as part of this proposal:

  1. Deprecate passing `memory_order_consume` to the existing variable functions.
  2. Make atomic ordering a template parameter tag instead of a variable. This
     would be a huge change, maybe we should do it separately, but it would
     simplify this proposal if we had it.
  3. Deprecate attribute `[[carries_dependency]]`.

The author is happy to add these to the current proposal if SG1 thinks it would
be useful to explore these.


## Future work

The current proposal has a rough API, very rough wording, and a small number of
tests. More work needs to go towards these items. Furthermore, the following
operations need to be added:

  - xchg
  - cmpxchg
  - RMW

It seems like the easiest way to go about is to try out the API in a
from-scratch C++ codebase. An obvious candidate would be a user-space RCU
implementation in C++ using this API.


## Proposed API

*/

class dependency;

// A value and its dependency.
template<typename T, typename Dependency = dependency>
struct dependent {
    T value;
    Dependency dependency;

    dependent() = delete;
    dependent(T value, Dependency dependency) : value(value), dependency(dependency) {}
    dependent(T value) : value(value), dependency(value) {}
};

// A dependent_ptr contains a pointer which was obtained through a consume load
// operation. It supports a restricted set of operations compared to regular
// pointers, which allows it to continue carrying its dependency.
//
// dependent_ptr differs from dependent<T*> by having a single data member, and
// by acting similarly to how regular pointers act. A dependent_ptr is a useful
// abstraction because it closely matches the low-level details of modern
// ISA-specific dependencies.
template<typename T>
class dependent_ptr {
public:
    // Constructors

    // No dependency yet.
    dependent_ptr() = default;
    dependent_ptr(T*);
    dependent_ptr(std::nullptr_t);

    // With dependency.
    dependent_ptr(T*, dependency);
    dependent_ptr(std::nullptr_t, dependency);
    dependent_ptr(dependent<uintptr_t>);
    dependent_ptr(dependent<intptr_t>);

    // Copy construction extends the right-hand side chain to cover both
    // dependent pointers. The left-hand side chain is broken.
    dependent_ptr(const dependent_ptr&);

    // Moving, Copying, and Casting

    // Assigning a non-dependent right-hand side breaks the left-hand side's
    // chain.
    dependent_ptr& operator=(T*);
    dependent_ptr& operator=(std::nullptr_t);

    // Using a dependent pointer as the right-hand side of an assignment
    // expression extends the chain to cover both the assignment and the value
    // returned by that assignment statement.
    dependent_ptr& operator=(const dependent_ptr&);

    // If a pointer value is part of a dependency chain, then converting it to
    // intptr_t or uintptr_t extends the chain to the result's dependency. This
    // can be used to perform pointer tagging (with usual C++ caveats on pointer
    // tagging) while retaining dependencies.
    dependent<uintptr_t> to_uintptr_t() const;
    dependent<intptr_t> to_intptr_t() const;

    // Pointer Offsets

    // Indexing though a dependent pointer extends the chain to the resulting
    // value.
    dependent<T> operator[](size_t offset) const;

    // Class-member access operators can be thought of as computing an offset. The
    // access itself is in the dependency chain, but such access does not extend
    // the chain to cover the result.
    T* operator->() const;

    // Dereferencing and Address-Of

    // Dereferencing a dependent pointer extends the chain to the resulting value.
    dependent<T> operator*() const;

    // If a pointer is part of a dependency chain, then applying the unary &
    // address-of operator extends the chain to the result.
    dependent_ptr<T*> operator&() const;

    // In some circumstances, such as for function pointers, the raw pointer
    // value is required. The chain extend to that value.
    T* value() const;

    // A pure dependency from the dependent_ptr.
    dependency dependency() const;

    // Comparisons aren't needed because the T* themselves can be compared
    // without breaking the dependency chain of the dependent_ptr. This is
    // important because compilers could optimize certain accesses based on the
    // result of comparisons, breaking explicitly constructed chains in the
    // process.

private:
    // Exposition only:
    T* ptr { nullptr };
};

// A dependency is an opaque value which can be chained through consume
// operations. Chaining dependencies ensures that load operations carry
// dependencies between each other. Dependencies can also be combined to create
// a new dependency which implies a dependency on the combined inputs.
//
// [ Note: Dependencies create false dependencies as defined by existing ISAs.
//   — end note ]
class dependency {
public:
    template<typename T> dependency(T);
    dependency() = delete;

    // Dependency combination.
    dependency operator|(dependency d);
    template<typename T> friend dependency operator|(dependency, dependent_ptr<T>);
    template<typename T> friend dependency operator|(dependent_ptr<T>, dependency);

    // Pointer tagging.
    friend uintptr_t operator|(dependency, uintptr_t);
    friend uintptr_t operator|(uintptr_t, dependency);
    friend intptr_t operator|(dependency, intptr_t);
    friend intptr_t operator|(intptr_t, dependency);

    // Exposition only:
    typedef unsigned dependency_type;

private:
    // Exposition only:
    dependency_type dep;
};

// Free-function dependency combination.
template<typename T> dependency operator|(dependency, dependent_ptr<T>);
template<typename T> dependency operator|(dependent_ptr<T>, dependency);

// Free-function pointer tagging with dependency.
uintptr_t operator|(dependency, uintptr_t);
uintptr_t operator|(uintptr_t, dependency);
intptr_t operator|(dependency, intptr_t);
intptr_t operator|(intptr_t, dependency);

// Beginning of dependency chain.
template<typename T> dependent_ptr<T> consume_load(const std::atomic<T*>&);
template<typename T> dependent<T> consume_load(const std::atomic<T>&);

// Subsequent dependent operations.
template<typename T> dependent_ptr<T> consume_load(dependent_ptr<T*>);
template<typename T> dependent<T> consume_load(dependent_ptr<T>);
template<typename T> dependent_ptr<T> consume_load(T**, dependency);
template<typename T> dependent<T> consume_load(T*, dependency);

#include "consume_dependency_impl.h"
#include "consume_dependent_ptr_impl.h"
#include "consume_load_impl.h"

#endif
