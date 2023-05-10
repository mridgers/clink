// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "settings.h"
#include "str.h"
#include "str_tokeniser.h"

//------------------------------------------------------------------------------
static Setting* g_setting_list = nullptr;



namespace settings
{

//------------------------------------------------------------------------------
Setting* first()
{
    return g_setting_list;
}

//------------------------------------------------------------------------------
Setting* find(const char* name)
{
    Setting* next = first();
    do
    {
        if (stricmp(name, next->get_name()) == 0)
            return next;
    }
    while (next = next->next());

    return nullptr;
}

//------------------------------------------------------------------------------
bool load(const char* file)
{
    // Open the file.
    FILE* in = fopen(file, "rb");
    if (in == nullptr)
        return false;

    // Buffer the file.
    fseek(in, 0, SEEK_END);
    int32 size = ftell(in);
    fseek(in, 0, SEEK_SET);

    if (size == 0)
    {
        fclose(in);
        return false;
    }

    Str<4096> buffer;
    buffer.reserve(size);

    char* data = buffer.data();
    fread(data, size, 1, in);
    fclose(in);
    data[size] = '\0';

    // Reset settings to default.
    for (auto* iter = settings::first(); iter != nullptr; iter = iter->next())
        iter->set();

    // Split at new lines.
    Str<256> line;
    StrTokeniser lines(buffer.c_str(), "\n\r");
    while (lines.next(line))
    {
        char* line_data = line.data();

        // Skip line's leading whitespace.
        while (isspace(*line_data))
            ++line_data;

        // Comment?
        if (line_data[0] == '#')
            continue;

        // 'key = value'?
        char* value = strchr(line_data, '=');
        if (value == nullptr)
            continue;

        *value++ = '\0';

        // Trim whitespace.
        char* key_end = value - 2;
        while (key_end >= line_data && isspace(*key_end))
            --key_end;
        key_end[1] = '\0';

        while (*value && isspace(*value))
            ++value;

        // Find the Setting and set its value.
        if (Setting* s = settings::find(line_data))
            s->set(value);
    }

    return true;
}

//------------------------------------------------------------------------------
bool save(const char* file)
{
    // Open settings file.
    FILE* out = fopen(file, "wt");
    if (out == nullptr)
        return false;

    // Iterate over each Setting and write it out to the file.
    for (const auto* iter = settings::first(); iter != nullptr; iter = iter->next())
    {
        // Don't write out settings that aren't modified from their defaults.
        if (iter->is_default())
            continue;

        fprintf(out, "# name: %s\n", iter->get_short_desc());

        // Write out the Setting's type.
        int32 type = iter->get_type();
        const char* type_name = nullptr;
        switch (type)
        {
        case Setting::type_bool:   type_name = "boolean"; break;
        case Setting::type_int:    type_name = "integer"; break;
        case Setting::type_string: type_name = "string";  break;
        case Setting::type_enum:   type_name = "enum";    break;
        }

        if (type_name != nullptr)
            fprintf(out, "# type: %s\n", type_name);

        // Output an enum-type Setting's options.
        if (type == Setting::type_enum)
        {
            const SettingEnum* as_enum = (SettingEnum*)iter;
            fprintf(out, "# options: %s\n", as_enum->get_options());
        }

        Str<> value;
        iter->get(value);
        fprintf(out, "%s = %s\n\n", iter->get_name(), value.c_str());
    }

    fclose(out);
    return true;
}

} // namespace settings



//------------------------------------------------------------------------------
Setting::Setting(
    const char* name,
    const char* short_desc,
    const char* long_desc,
    TypeE type)
: _name(name)
, _short_desc(short_desc)
, _long_desc(long_desc ? long_desc : "")
, _type(type)
{
    Setting* insert_at = nullptr;
    for (auto* i = g_setting_list; i != nullptr; insert_at = i, i = i->next())
        if (stricmp(name, i->get_name()) < 0)
            break;

    if (insert_at == nullptr)
    {
        _prev = nullptr;
        _next = g_setting_list;
        g_setting_list = this;
    }
    else
    {
        _next = insert_at->_next;
        _prev = insert_at;
        insert_at->_next = this;
    }

    if (_next != nullptr)
        _next->_prev = this;
}

//------------------------------------------------------------------------------
Setting::~Setting()
{
    if (_prev != nullptr)
        _prev->_next = _next;
    else
        g_setting_list = _next;

    if (_next != nullptr)
        _next->_prev = _prev;
}

//------------------------------------------------------------------------------
Setting* Setting::next() const
{
    return _next;
}

//------------------------------------------------------------------------------
Setting::TypeE Setting::get_type() const
{
    return _type;
}

//------------------------------------------------------------------------------
const char* Setting::get_name() const
{
    return _name.c_str();
}

//------------------------------------------------------------------------------
const char* Setting::get_short_desc() const
{
    return _short_desc.c_str();
}

//------------------------------------------------------------------------------
const char* Setting::get_long_desc() const
{
    return _long_desc.c_str();
}



//------------------------------------------------------------------------------
template <> bool SettingImpl<bool>::set(const char* value)
{
    if (stricmp(value, "true") == 0)  { _store.value = 1; return true; }
    if (stricmp(value, "false") == 0) { _store.value = 0; return true; }

    if (*value >= '0' && *value <= '9')
    {
        _store.value = !!atoi(value);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
template <> bool SettingImpl<int32>::set(const char* value)
{
    if ((*value < '0' || *value > '9') && *value != '-')
        return false;

    _store.value = atoi(value);
    return true;
}

//------------------------------------------------------------------------------
template <> bool SettingImpl<const char*>::set(const char* value)
{
    _store.value = value;
    return true;
}



//------------------------------------------------------------------------------
template <> void SettingImpl<bool>::get(StrBase& out) const
{
    out = _store.value ? "True" : "False";
}

//------------------------------------------------------------------------------
template <> void SettingImpl<int32>::get(StrBase& out) const
{
    out.format("%d", _store.value);
}

//------------------------------------------------------------------------------
template <> void SettingImpl<const char*>::get(StrBase& out) const
{
    out = _store.value.c_str();
}



//------------------------------------------------------------------------------
SettingEnum::SettingEnum(
    const char* name,
    const char* short_desc,
    const char* options,
    int32 default_value)
: SettingEnum(name, short_desc, nullptr, options, default_value)
{
}

//------------------------------------------------------------------------------
SettingEnum::SettingEnum(
    const char* name,
    const char* short_desc,
    const char* long_desc,
    const char* options,
    int32 default_value)
: SettingImpl<int32>(name, short_desc, long_desc, default_value)
, _options(options)
{
    _type = type_enum;
}

//------------------------------------------------------------------------------
bool SettingEnum::set(const char* value)
{
    int32 i = 0;
    for (const char* option = _options.c_str(); *option; ++i)
    {
        const char* next = next_option(option);

        int32 option_len = int32(next - option);
        if (*next)
            --option_len;

        if (_strnicmp(option, value, option_len) == 0)
        {
            _store.value = i;
            return true;
        }

        option = next;
    }

    return false;
}

//------------------------------------------------------------------------------
void SettingEnum::get(StrBase& out) const
{
    int32 index = _store.value;
    if (index < 0)
        return;

    const char* option = _options.c_str();
    for (int32 i = 0; i < index && *option; ++i)
        option = next_option(option);

    if (*option)
    {
        const char* next = next_option(option);
        if (*next)
            --next;

        out.clear();
        out.concat(option, int32(next - option));
    }
}

//------------------------------------------------------------------------------
const char* SettingEnum::get_options() const
{
    return _options.c_str();
}

//------------------------------------------------------------------------------
const char* SettingEnum::next_option(const char* option)
{
    while (*option)
        if (*option++ == ',')
            break;

    return option;
}
