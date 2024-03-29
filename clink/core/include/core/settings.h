// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "str.h"

class Setting;

//------------------------------------------------------------------------------
namespace settings
{

Setting*            first();
Setting*            find(const char* name);
bool                load(const char* file);
bool                save(const char* file);

};



//------------------------------------------------------------------------------
class Setting
{
public:
    enum TypeE : uint8 {
        type_unknown,
        type_int,
        type_bool,
        type_string,
        type_enum,
    };

    virtual         ~Setting();
    Setting*        next() const;
    TypeE           get_type() const;
    const char*     get_name() const;
    const char*     get_short_desc() const;
    const char*     get_long_desc() const;
    virtual bool    is_default() const = 0;
    virtual void    set() = 0;
    virtual bool    set(const char* value) = 0;
    virtual void    get(StrBase& out) const = 0;

protected:
                    Setting(const char* name, const char* short_desc, const char* long_desc, TypeE Type);
    Str<32, false>  _name;
    Str<48, false>  _short_desc;
    Str<128>        _long_desc;
    Setting*        _prev;
    Setting*        _next;
    TypeE           _type;

    template <typename T>
    struct Store
    {
        explicit    operator T () const                  { return value; }
        bool        operator == (const Store& rhs) const { return value == rhs.value; }
        T           value;
    };
};

//------------------------------------------------------------------------------
template <> struct Setting::Store<const char*>
{
    explicit        operator const char* () const        { return value.c_str(); }
    bool            operator == (const Store& rhs) const { return value.equals(rhs.value.c_str()); }
    Str<64>         value;
};

//------------------------------------------------------------------------------
template <typename T>
class SettingImpl
    : public Setting
{
public:
                    SettingImpl(const char* name, const char* short_desc, T default_value);
                    SettingImpl(const char* name, const char* short_desc, const char* long_desc, T default_value);
    T               get() const;
    virtual bool    is_default() const override;
    virtual void    set() override;
    virtual bool    set(const char* value) override;
    virtual void    get(StrBase& out) const override;

protected:
    struct          Type;
    Store<T>        _store;
    Store<T>        _default;
};

//------------------------------------------------------------------------------
template <typename T> SettingImpl<T>::SettingImpl(
    const char* name,
    const char* short_desc,
    T default_value)
: SettingImpl<T>(name, short_desc, nullptr, default_value)
{
}

//------------------------------------------------------------------------------
template <typename T> SettingImpl<T>::SettingImpl(
    const char* name,
    const char* short_desc,
    const char* long_desc,
    T default_value)
: Setting(name, short_desc, long_desc, TypeE(Type::id))
{
    _default.value = default_value;
    _store.value = default_value;
}

//------------------------------------------------------------------------------
template <typename T> void SettingImpl<T>::set()
{
    _store.value = T(_default);
}

//------------------------------------------------------------------------------
template <typename T> bool SettingImpl<T>::is_default() const
{
    return _store == _default;
}

//------------------------------------------------------------------------------
template <typename T> T SettingImpl<T>::get() const
{
    return T(_store);
}



//------------------------------------------------------------------------------
template <> struct SettingImpl<bool>::Type        { enum { id = Setting::type_bool }; };
template <> struct SettingImpl<int32>::Type         { enum { id = Setting::type_int }; };
template <> struct SettingImpl<const char*>::Type { enum { id = Setting::type_string }; };

//------------------------------------------------------------------------------
typedef SettingImpl<bool>           SettingBool;
typedef SettingImpl<int32>            SettingInt;
typedef SettingImpl<const char*>    SettingStr;

//------------------------------------------------------------------------------
class SettingEnum
    : public SettingImpl<int32>
{
public:
                       SettingEnum(const char* name, const char* short_desc, const char* values, int32 default_value);
                       SettingEnum(const char* name, const char* short_desc, const char* long_desc, const char* values, int32 default_value);
    virtual bool       set(const char* value) override;
    virtual void       get(StrBase& out) const override;
    const char*        get_options() const;

    using SettingImpl<int32>::get;

protected:
    static const char* next_option(const char* option);
    Str<48>            _options;
};
