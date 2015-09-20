#ifndef ENUM_ENUM_HPP
#define ENUM_ENUM_HPP

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace venum {

// Index of T in Ts
template<typename, typename...>
struct IndexOf;

template<typename T, typename U, typename... Ts>
struct IndexOf<T, U, Ts...> {
    static constexpr int value = 1 + IndexOf<T, Ts...>::value;
};

template<typename T, typename... Ts>
struct IndexOf<T, T, Ts...> {
    static constexpr int value = 0;
};

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
template<typename... Args>
struct Or;

template<typename T, typename... Args>
struct Or<T, Args...> {
    static constexpr bool value = T::value || Or<Args...>::value;
};

template<typename T>
struct Or<T> {
    static constexpr bool value = T::value;
};

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

// Error Handling
// X macro for error states
#define VARIANT_ERROR_EXPAND \
VARIANT_ERROR_X(CopyThrew, VariantCopyThrew, "Variant invalidated after copy constructor throw") \
VARIANT_ERROR_X(MoveThrew, VariantMoveThrew, "Variant invalidated after move constructor throw") \
VARIANT_ERROR_X(MovedFrom, VariandMovedFrom, "Variant invalidated after being moved from") \
VARIANT_ERROR_X(Unknown, UnknownVariantError, "Variant invalidated after copy constructor throw")

// Error state enum
#define VARIANT_ERROR_X(name, error, msg) name,
enum InvalidReason {
    VARIANT_ERROR_EXPAND
};
#undef VARIANT_ERROR_X

// Base exception class
struct InvalidVariantError : public std::exception {
    InvalidVariantError() : std::exception() {}

    virtual InvalidReason reason() const noexcept = 0;
    virtual const char* what() const noexcept = 0;
};

// Error state exceptions
#define VARIANT_ERROR_X(name, error, msg) \
struct error : public InvalidVariantError { \
    error() : InvalidVariantError() {} \
\
    InvalidReason reason() const noexcept { \
        return InvalidReason::name; \
    } \
\
    const char* what() const noexcept { \
        return msg; \
    } \
};

VARIANT_ERROR_EXPAND
#undef VARIANT_ERROR_X

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
                } else {
                    return F<T, n>::invalid(tag, std::forward<Args>(args)...);
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

                try {
                    ::new (&(to->storage)) T(*reinterpret_cast<T*>(&(from.storage)));
                } catch(std::exception&) {
                    to->tag = to->variants + InvalidReason::CopyThrew;
                }
            }

            static void invalid(const std::size_t& tag, const Self& from, Self* to) {
                to->tag = from.tag;
            }
        };

        // Move Constructor
        template<typename T, std::size_t n>
        struct MoveConstructorT {
            static void call(Self&& from, Self* to) {
                to->tag = n;
                from.tag = from.variants + InvalidReason::MovedFrom;

                try {
                    ::new (&(to->storage)) T(std::move(*reinterpret_cast<T*>(&(from.storage))));
                } catch(std::exception&) {
                    to->tag = to->variants + InvalidReason::MoveThrew;
                }
            }

            static void invalid(const std::size_t& tag, Self&& from, Self* to) {
                to->tag = from.tag;
                from.tag = from.variants + InvalidReason::MovedFrom;
            }
        };

        // Destructor
        template<typename T, std::size_t n>
        struct DestructorT {
            static void call(Self* e) {
                reinterpret_cast<T*>(&(e->storage))->~T();
            }

            static void invalid(const std::size_t& tag, Self* e) {}
        };

        // Apply
        template<typename T, std::size_t n>
        struct ApplyT {
            template<typename F>
            static auto call(Self* e, F f) {
                return f(*reinterpret_cast<T*>(&(e->storage)));
            }

            template<typename F>
            static auto invalid(const std::size_t& tag, Self* e, F f) -> decltype(f(*(T*)nullptr)) {
                #define VARIANT_ERROR_X(name, error, msg) case InvalidReason::name: throw error();
                
                switch(tag - e->variants) {
                    VARIANT_ERROR_EXPAND

                    default:
                        throw UnknownVariantError();
                }

                #undef VARIANT_ERROR_X
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

            static auto invalid(const std::size_t& tag, F f, Fs... fs) {
                return CallNth<T, n - 1, Fs...>::invalid(tag, std::forward<Fs>(fs)...);
            }
        };

        template<typename T, typename F, typename... Fs>
        struct CallNth<T, 0, F, Fs...> {
            static auto call(T t, F f, Fs... fs) {
                return f(t);
            }

            static auto invalid(const std::size_t& tag, F f, Fs... fs) {
                #define VARIANT_ERROR_X(name, error, msg) case InvalidReason::name: return f(error());

                switch(tag - variants) {
                    VARIANT_ERROR_EXPAND

                    default:
                        return f(UnknownVariantError());
                }

                #undef VARIANT_ERROR_X
            }
        };

        template<typename T, std::size_t n, bool no_check, typename... Fs>
        struct MatchTBase;

        template<typename T, std::size_t n, typename... Fs>
        struct MatchTBase<T, n, true, Fs...> {
            static auto call(Self* e, Fs... fs) {
                return CallNth<T, n, Fs...>::call(*reinterpret_cast<T*>(&(e->storage)), std::forward<Fs>(fs)...);
            }

            static auto invalid(const std::size_t& tag, Self* e, Fs... fs) 
                -> decltype(CallNth<T, n, Fs...>::call(*(T*)nullptr, fs...)) {

                #define VARIANT_ERROR_X(name, error, msg) case InvalidReason::name: throw error();
                
                switch(tag - e->variants) {
                    VARIANT_ERROR_EXPAND

                    default:
                        throw UnknownVariantError();
                }

                #undef VARIANT_ERROR_X
            }
        };

        template<typename T, std::size_t n, typename... Fs>
        struct MatchTBase<T, n, false, Fs...> {
            static auto call(Self* e, Fs... fs) {
                return CallNth<T, n, Fs...>::call(*reinterpret_cast<T*>(&(e->storage)), std::forward<Fs>(fs)...);
            }

            static auto invalid(const std::size_t& tag, Self* e, Fs... fs) {
                return CallNth<T, variants, Fs...>::invalid(tag, std::forward<Fs>(fs)...);
            }
        };

        template<typename T, std::size_t n>
        struct MatchT {
            template<typename... Fs>
            using Base = MatchTBase<T, n, sizeof...(Fs) == 1 + sizeof...(Variants), Fs...>;

            template<typename... Fs>
            static auto call(Self* e, Fs... fs) {
                return Base<Fs...>::call(e, std::forward<Fs>(fs)...);
            }

            template<typename... Fs>
            static auto invalid(const std::size_t& tag, Self* e, Fs... fs) {
                return Base<Fs...>::invalid(tag, e, std::forward<Fs>(fs)...);
            }
        };
    };

    template<typename... Args>
    using Constructor = typename std::conditional<
        Or<typename impl::template TypeCheck<VariantT, Args...>, typename impl::template TypeCheck<Variants, Args...>...>::value,
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

    // Private default constructor, for construct<T>
    EnumT() : storage() {}

public:
    template<typename T>
    using Variant = EnumT<VariantT, Variants..., T>;

    template<typename T, typename... Args>
    static Self construct(Args&&... args) {
        EnumT ret;
        ret.tag = IndexOf<T, VariantT, Variants...>::value;
        ::new (&(ret.storage)) T(std::forward<Args>(args)...);
        return ret;
    }

    template<typename... Args>
    EnumT(Args&&... args) {
        Constructor<Args...>::construct(this, std::forward<Args>(args)...);
    }

    EnumT(const Self& other) noexcept {
        CopyConstructor::call(other.tag, std::forward<Self>(other), this);
    }

    EnumT(Self&& other) noexcept {
        MoveConstructor::call(other.tag, std::forward<Self>(other), this);
    }

    EnumT& operator=(const Self& other) noexcept {
        if(this->tag != other.tag) {
            Destructor::call(this->tag, this);
        }

        CopyConstructor::call(other.tag, std::forward<Self>(other), this);
        return *this;
    }

    EnumT& operator=(Self&& other) noexcept {
        if(this->tag != other.tag) {
            Destructor::call(this->tag, this);
        }

        MoveConstructor::call(other.tag, std::forward<Self>(other), this);
        return *this;
    }

    // Apply the object to a polymorphic function
    template<typename F>
    auto apply(F f) {
        return Apply<F>::call(this->tag, this, std::forward<F>(f));
    }

    // Apply to a function based on the contained type
    template<typename... Fs>
    auto match(Fs... fs) {
        return Match<Fs...>::call(this->tag, this, std::forward<Fs>(fs)...);
    }

    // Returns the identifying tag
    std::size_t which() const noexcept {
        return tag;
    }

    // Returns true if the contained value is of type T
    template<typename T>
    bool contains() const noexcept {
        return tag == IndexOf<T, VariantT, Variants...>::value;
    }

    // Returns the object as the specified type, or throws
    template<typename T>
    T& get() {
        if(tag == IndexOf<T, VariantT, Variants...>::value) {
            return *reinterpret_cast<T*>(&storage);
        } else {
            throw std::runtime_error("Attempted get<T> on incorrect type");
        }
    }

    // I don't recommend this function
    template<typename T>
    T& get_unchecked() {
        return *reinterpret_cast<T*>(&storage);
    }

    // Returns true if the variant is valid
    bool valid() const noexcept {
        return tag < variants;
    }

    // Returns true if the variant is valid
    explicit operator bool() const noexcept {
        return valid();
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

}

#undef VARIANT_ERROR_EXPAND

#endif
