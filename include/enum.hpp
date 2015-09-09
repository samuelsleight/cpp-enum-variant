#ifndef ENUM_ENUM_HPP
#define ENUM_ENUM_HPP

#include <algorithm>
#include <type_traits>
#include <utility>

// Variadic Max
template<typename T>
constexpr auto const_max(T a) {
    return a;
}

template<typename T, typename... Args>
constexpr T const_max(T a, T b, Args... args) {
    return a > b ? const_max(a, args...) : const_max(b, args...);
}

// Variadic Or
constexpr auto const_or(bool b) {
    return b;
}

template<typename... Args>
constexpr auto const_or(bool b, Args... args) {
    return b || const_or(args...);
}

// TypeList 
template<typename T, std::size_t n>
struct NthImpl : public NthImpl<typename T::Tail, n - 1> {};

template<typename T>
struct NthImpl<T, 0> {
    using value = typename T::Head;
};

template<typename...>
struct TypeList {};

template<typename H, typename... Ts>
struct TypeList<H, Ts...> {
    using Head = H;
    using Tail = TypeList<Ts...>;

    template<std::size_t n>
    using Nth = typename NthImpl<TypeList<H, Ts...>, n>::value;
};

// Enum implementation
template<typename VariantT, typename... Variants>
class EnumT {
public:
    static constexpr std::size_t storage_size = const_max(sizeof(VariantT), sizeof(Variants)...);
    static constexpr std::size_t storage_align = const_max(alignof(VariantT), alignof(Variants)...);

    static constexpr std::size_t variants = sizeof...(Variants) + 1;

private:
    using Self = EnumT<VariantT, Variants...>;
    using VariantList = TypeList<VariantT, Variants...>;

    using StorageT = typename std::aligned_storage<storage_size, storage_align>::type;

    // Implementation detail
    struct impl {
        // Constructor
        template<template<typename...> typename Check, bool enable, std::size_t n, typename... Args>
        struct ConstructorT;

        template<template<typename...> typename Check, std::size_t n, typename... Args>
        struct ConstructorT<Check, false, n, Args...> 
            : public ConstructorT<Check, Check<typename Self::VariantList::template Nth<n + 1>, Args...>::value, n + 1, Args...> {};

        template<template<typename...> typename Check, std::size_t n, typename... Args>
        struct ConstructorT<Check, true, n, Args...> {
            static void construct(Self* e, Args&&... args) {
                using T = typename Self::VariantList::template Nth<n>;

                ::new (&(e->storage)) T(std::forward<Args>(args)...);
                e->tag = n;
            }
        };

        // Type Check for Constructor
        template<typename T, typename... Args>
        struct TypeCheck {
            static constexpr bool value = false;
        };

        template<typename T, typename U>
        struct TypeCheck<T, U> {
            static constexpr bool value = std::is_same<typename std::decay<T>::type, typename std::decay<U>::type>::value && std::is_constructible<T, U>::value;
        };

        // Helper
        template<std::size_t n, std::size_t m, template<typename, std::size_t> typename F, typename... Args>
        struct HelperT {
            static auto call(const std::size_t& tag, Args... args) {
                using T = typename Self::VariantList::template Nth<n>;

                if(tag == n) {
                    return F<T, n>::call(std::forward<Args>(args)...);
                } else {
                    return HelperT<n + 1, m, F, Args...>::call(tag, std::forward<Args>(args)...);
                }
            }
        };

        template<std::size_t n, template<typename, std::size_t> typename F, typename... Args>
        struct HelperT<n, n, F, Args...> {
            static auto call(const std::size_t& tag, Args... args) {
                using T = typename Self::VariantList::template Nth<n>;

                if(tag == n) {
                    return F<T, n>::call(std::forward<Args>(args)...);
                }
            }
        };

        template<template<typename, std::size_t> typename F, typename... Args>
        using Helper = HelperT<0, Self::variants - 1, F, Args...>;

        // Copy Constructor
        template<typename T, std::size_t n>
        struct CopyConstructorT {
            static void call(const Self& from, Self* to) {
                to->tag = n;
                ::new (&(to->storage)) T(*reinterpret_cast<T*>(&(from.storage)));
            }
        };

        // Move Constructor
        template<typename T, std::size_t n>
        struct MoveConstructorT {
            static void call(Self&& from, Self* to) {
                to->tag = std::move(n);
                ::new (&(to->storage)) T(std::move(*reinterpret_cast<T*>(&(from.storage))));
            }
        };

        // Destructor
        template<typename T, std::size_t n>
        struct DestructorT {
            static void call(Self* e) {
                reinterpret_cast<T*>(&(e->storage))->~T();
            }
        };

        // Apply
        template<typename T, std::size_t n>
        struct ApplyT {
            template<typename F>
            static auto call(Self* e, F f) {
                return f(*reinterpret_cast<T*>(&(e->storage)));
            }
        };

        // Match
        template<typename T, std::size_t n, typename... Fs>
        struct CallNth;

        template<typename T, std::size_t n, typename F, typename... Fs>
        struct CallNth<T, n, F, Fs...> {
            static auto call(T t, F f, Fs... fs) {
                return CallNth<T, n - 1, Fs...>::call(t, std::forward<Fs>(fs)...);
            }
        };

        template<typename T, typename F, typename... Fs>
        struct CallNth<T, 0, F, Fs...> {
            static auto call(T t, F f, Fs... fs) {
                return f(t);
            }
        };

        template<typename T, std::size_t n>
        struct MatchT {
            template<typename... Fs>
            static auto call(Self* e, Fs... fs) {
                return CallNth<T, n, Fs...>::call(*reinterpret_cast<T*>(&(e->storage)), std::forward<Fs>(fs)...);
            }
        };
    };

    template<typename... Args>
    using Constructor = typename std::conditional<
        const_or(impl::template TypeCheck<VariantT, Args...>::value, impl::template TypeCheck<Variants, Args...>::value...),
        typename impl::template ConstructorT<impl::template TypeCheck, impl::template TypeCheck<VariantT, Args...>::value, 0, Args...>,
        typename impl::template ConstructorT<std::is_constructible, std::is_constructible<VariantT, Args...>::value, 0, Args...>
    >::type;

    using CopyConstructor = typename impl::template Helper<impl::template CopyConstructorT, const Self&, Self*>;
    using MoveConstructor = typename impl::template Helper<impl::template MoveConstructorT, Self&&, Self*>;
    using Destructor = typename impl::template Helper<impl::template DestructorT, Self*>;

    template<typename F>
    using Apply = typename impl::template Helper<impl::template ApplyT, Self*, F>;

    template<typename... Fs>
    using Match = typename impl::template Helper<impl::template MatchT, Self*, Fs...>;

    std::size_t tag;
    StorageT storage;

public:
    template<typename T>
    using Variant = EnumT<VariantT, Variants..., T>;

    EnumT() = delete;

    template<typename... Args>
    EnumT(Args&&... args) {
        Constructor<Args...>::construct(this, std::forward<Args>(args)...);
    }

    EnumT(const Self& other) {
        CopyConstructor::call(other.tag, std::forward<Self>(other), this);
    }

    EnumT(Self&& other) noexcept {
        MoveConstructor::call(other.tag, std::forward<Self>(other), this);
    }

    EnumT& operator=(const Self& other) {
        Destructor::call(this->tag, this);
        CopyConstructor::call(other.tag, std::forward<Self>(other), this);
        return *this;
    }

    EnumT& operator=(Self&& other) noexcept {
        Destructor::call(this->tag, this);
        MoveConstructor::call(other.tag, std::forward<Self>(other), this);
        return *this;
    }

    template<typename F>
    auto apply(F f) {
        return Apply<F>::call(this->tag, this, std::forward<F>(f));
    }

    template<typename... Fs>
    auto match(Fs... fs) {
        return Match<Fs...>::call(this->tag, this, std::forward<Fs>(fs)...);
    }

    ~EnumT() {
        Destructor::call(this->tag, this);
    }
};


class Enum {
public:
    template<typename T>
    using Variant = EnumT<T>;
};

#endif
