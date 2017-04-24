//////////////////////////////////////////////////////////////////////////////
//  File: cpp-enum-variant/optional.hpp
//////////////////////////////////////////////////////////////////////////////
//  Copyright 2017 Samuel Sleight
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//////////////////////////////////////////////////////////////////////////////

#ifndef ENUM_OPTIONAL_HPP
#define ENUM_OPTIONAL_HPP

#include "enum.hpp"

#include <stdexcept>

struct None {};

template<typename T>
using OptionalBase = venum::Enum::Variant<::None>::Variant<T>;

template<typename T>
class Optional : public OptionalBase<T> {
public:
    using ValueType = T;

    static Optional<T> Some(T t) {
        return Optional(t);
    }

    static Optional<T> None() {
        return Optional(::None{});
    }

    explicit operator bool() {
        return this->match(
            [](::None) { return false; },
            [](T) { return true; }
        );
    }

    T get() {
        return this->match(
            [](::None) -> T { throw std::runtime_error("Error: attempted get() on Optional::None"); },
            [](T t) { return t; }
        );
    }

    template<typename F>
    auto map(F f) {
        using U = decltype(f(get()));

        return this->match(
            [](::None) { return Optional<U>::None(); },
            [&f](T t) { return Optional<U>::Some(f(t)); }
        );
    }

    template<typename F>
    auto and_then(F f) {
        using U = typename decltype(f(get()))::ValueType;

        return this->match(
            [](::None) { return Optional<U>::None(); },
            [&f](T t) { return f(t); }
        );
    }

    template<typename... Args>
    Optional(Args... args) : OptionalBase<T>(std::forward<Args>(args)...) {}
};

#endif
