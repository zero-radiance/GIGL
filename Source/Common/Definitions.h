#pragma once

using uint = unsigned int;

#if __cplusplus <= 201103L
    /* No full C++ 11 support */
    // Assume noexcept is not supported
    #define NOEXCEPT throw()
    // Assume constexpr is not supported
    #define CONSTEXPR static const
    // Assume [[ noreturn ]] is not supported
    #define NORETURN __declspec( noreturn )
    // Assume that std::generate_canonical is broken
    #define BROKEN_STD_GENERATE_CANONICAL
    // Synthesizing default move CTORs and assignment OPs is not supported by MSVC 2013
    // MSVC will use copy CTORs and assignment OPs instead
    #define DEFAULT_MOVE(T)
#else
    #define NOEXCEPT  noexcept
    #define CONSTEXPR constexpr
    #define NORETURN  [[ noreturn ]]
    #define DEFAULT_MOVE(T) T(T&&) NOEXCEPT = default;    \
                            T& operator=(T&&) NOEXCEPT = default
#endif

// Declares -default- Dtor, copy/move Ctors and assignment Ops
#define RULE_OF_ZERO(T) T(const T&) = default;            \
                        T& operator=(const T&) = default; \
                        DEFAULT_MOVE(T);                  \
                        ~T() = default

// Declares -explicit- Dtor, copy/move Ctors and assignment Ops
#define RULE_OF_FIVE(T) T(const T&);                \
                        T& operator=(const T&);     \
                        T(T&&) NOEXCEPT;            \
                        T& operator=(T&&) NOEXCEPT; \
                        ~T()

// Declares -default- Dtor, move Ctor and assignment Op
// Forbids copy construction and copy assignment
#define RULE_OF_ZERO_NO_COPY(T) T(const T&) = delete;            \
                                T& operator=(const T&) = delete; \
                                DEFAULT_MOVE(T);                 \
                                ~T() = default

// Declares -explicit- Dtor, move Ctor and assignment Op
// Forbids copy construction and copy assignment
#define RULE_OF_FIVE_NO_COPY(T) T(const T&) = delete;    \
                        T& operator=(const T&) = delete; \
                        T(T&&) NOEXCEPT;                 \
                        T& operator=(T&&) NOEXCEPT;      \
                        ~T()

// Use old-style vertex array binding; disable on Quadro
#define OLD_STYLE_BINDING
