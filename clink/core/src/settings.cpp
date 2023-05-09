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
    int size = ftell(in);
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
        int type = iter->get_type();
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
: m_name(name)
, m_short_desc(short_desc)
, m_long_desc(long_desc ? long_desc : "")
, m_type(type)
{
    Setting* insert_at = nullptr;
    for (auto* i = g_setting_list; i != nullptr; insert_at = i, i = i->next())
        if (stricmp(name, i->get_name()) < 0)
            break;

    if (insert_at == nullptr)
    {
        m_prev = nullptr;
        m_next = g_setting_list;
        g_setting_list = this;
    }
    else
    {
        m_next = insert_at->m_next;
        m_prev = insert_at;
        insert_at->m_next = this;
    }

    if (m_next != nullptr)
        m_next->m_prev = this;
}

//------------------------------------------------------------------------------
Setting::~Setting()
{
    if (m_prev != nullptr)
        m_prev->m_next = m_next;
    else
        g_setting_list = m_next;

    if (m_next != nullptr)
        m_next->m_prev = m_prev;
}

//------------------------------------------------------------------------------
Setting* Setting::next() const
{
    return m_next;
}

//------------------------------------------------------------------------------
Setting::TypeE Setting::get_type() const
{
    return m_type;
}

//------------------------------------------------------------------------------
const char* Setting::get_name() const
{
    return m_name.c_str();
}

//------------------------------------------------------------------------------
const char* Setting::get_short_desc() const
{
    return m_short_desc.c_str();
}

//------------------------------------------------------------------------------
const char* Setting::get_long_desc() const
{
    return m_long_desc.c_str();
}



//------------------------------------------------------------------------------
template <> bool SettingImpl<bool>::set(const char* value)
{
    if (stricmp(value, "true") == 0)  { m_store.value = 1; return true; }
    if (stricmp(value, "false") == 0) { m_store.value = 0; return true; }

    if (*value >= '0' && *value <= '9')
    {
        m_store.value = !!atoi(value);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
template <> bool SettingImpl<int>::set(const char* value)
{
    if ((*value < '0' || *value > '9') && *value != '-')
        return false;

    m_store.value = atoi(value);
    return true;
}

//------------------------------------------------------------------------------
template <> bool SettingImpl<const char*>::set(const char* value)
{
    m_store.value = value;
    return true;
}



//------------------------------------------------------------------------------
template <> void SettingImpl<bool>::get(StrBase& out) const
{
    out = m_store.value ? "True" : "False";
}

//------------------------------------------------------------------------------
template <> void SettingImpl<int>::get(StrBase& out) const
{
    out.format("%d", m_store.value);
}

//------------------------------------------------------------------------------
template <> void SettingImpl<const char*>::get(StrBase& out) const
{
    out = m_store.value.c_str();
}



//------------------------------------------------------------------------------
SettingEnum::SettingEnum(
    const char* name,
    const char* short_desc,
    const char* options,
    int default_value)
: SettingEnum(name, short_desc, nullptr, options, default_value)
{
}

//------------------------------------------------------------------------------
SettingEnum::SettingEnum(
    const char* name,
    const char* short_desc,
    const char* long_desc,
    const char* options,
    int default_value)
: SettingImpl<int>(name, short_desc, long_desc, default_value)
, m_options(options)
{
    m_type = type_enum;
}

//------------------------------------------------------------------------------
bool SettingEnum::set(const char* value)
{
    int i = 0;
    for (const char* option = m_options.c_str(); *option; ++i)
    {
        const char* next = next_option(option);

        int option_len = int(next - option);
        if (*next)
            --option_len;

        if (_strnicmp(option, value, option_len) == 0)
        {
            m_store.value = i;
            return true;
        }

        option = next;
    }

    return false;
}

//------------------------------------------------------------------------------
void SettingEnum::get(StrBase& out) const
{
    int index = m_store.value;
    if (index < 0)
        return;

    const char* option = m_options.c_str();
    for (int i = 0; i < index && *option; ++i)
        option = next_option(option);

    if (*option)
    {
        const char* next = next_option(option);
        if (*next)
            --next;

        out.clear();
        out.concat(option, int(next - option));
    }
}

//------------------------------------------------------------------------------
const char* SettingEnum::get_options() const
{
    return m_options.c_str();
}

//------------------------------------------------------------------------------
const char* SettingEnum::next_option(const char* option)
{
    while (*option)
        if (*option++ == ',')
            break;

    return option;
}
