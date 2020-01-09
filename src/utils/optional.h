#pragma once
#ifndef CSC471_OPTIONAL_SUBSTITUTE_H_
#define CSC471_OPTIONAL_SUBSTITUTE_H_
#include <exception>

struct nullopt_t
{
    explicit constexpr nullopt_t(int) {}
};

inline constexpr nullopt_t nullopt{42};

class bad_optional_access : std::exception
{
 public:
    virtual const char* what() const noexcept{
        return("error: optional has no contained value");
    }
};

/* A minimal replica of std::optional found in C++17 */
template<typename T>
class optional
{
 public:
    using value_type = T;

    optional() noexcept {}
    optional(const value_type& value) {
        _mHasValue = true;
        _assign(value);
    }
    optional(nullopt_t) noexcept {}

    optional<T>& operator=(nullopt_t) noexcept {
        reset();
        return(*this);
    }
    optional<T>& operator=(const value_type& value) {
        _mHasValue = true;
        _assign(value);
        return(*this);
    }

    constexpr const value_type* operator->() const {
        return(reinterpret_cast<const T*>(_mValue));
    }
    constexpr value_type* operator->() {
        return(reinterpret_cast<T*>(_mValue));
    }
    constexpr const value_type& operator*() const {
        return(*reinterpret_cast<const T*>(_mValue));
    }
    constexpr value_type& operator*() {
        return(*reinterpret_cast<T*>(_mValue));
    }


    constexpr explicit operator bool() const noexcept {return(_mHasValue);}
    constexpr bool has_value() const noexcept {return(_mHasValue);}

    constexpr T& value() {
        if(!_mHasValue)
            throw bad_optional_access();
        T* interpreted_value = reinterpret_cast<T*>(_mValue);
        return(*interpreted_value);
    }
    constexpr const T& value() const {
        if(!_mHasValue)
            throw bad_optional_access();
        const T* interpreted_value = reinterpret_cast<const T*>(_mValue);
        return(*interpreted_value);
    }

    void reset() noexcept {
        if(_mHasValue){
            value().~T();
        }
        _mHasValue = false;
    }
    
 protected:
    void _assign(const value_type& value) {
        *reinterpret_cast<T*>(_mValue) = value; 
    }
    
 private:
    bool _mHasValue = false; 
    uint8_t _mValue[sizeof(value_type)];
};

#endif 