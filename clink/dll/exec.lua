--
-- Copyright (c) 2012 Martin Ridgers
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.
--

--------------------------------------------------------------------------------
local dos_commands = {
    "assoc", "break", "call", "cd", "chcp", "chdir", "cls", "color", "copy",
    "date", "del", "dir", "diskcomp", "diskcopy", "echo", "endlocal", "erase",
    "exit", "for", "format", "ftype", "goto", "graftabl", "if", "md", "mkdir",
    "mklink", "more", "move", "path", "pause", "popd", "prompt", "pushd", "rd",
    "rem", "ren", "rename", "rmdir", "set", "setlocal", "shift", "start",
    "time", "title", "tree", "type", "ver", "verify", "vol"
}

--------------------------------------------------------------------------------
local function get_environment_paths()
    local paths = clink.split(clink.get_env("PATH"), ";")

    -- We're expecting absolute paths and as ';' is a valid path character
    -- there maybe unneccessary splits. Here we resolve them.
    local paths_merged = { paths[1] }
    for i = 2, #paths, 1 do
        if not paths[i]:find("^[a-zA-Z]:") then
            local t = paths_merged[#paths_merged];
            paths_merged[#paths_merged] = t..paths[i]
        else
            table.insert(paths_merged, paths[i])
        end
    end

    -- Append slashes.
    for i = 1, #paths_merged, 1 do
        table.insert(paths, paths_merged[i].."\\")
    end

    return paths
end

--------------------------------------------------------------------------------
local function exec_match_generator(text, first, last)
    -- If match style setting is < 0 then consider executable matching disabled.
    local match_style = clink.get_setting_int("exec_match_style")
    if match_style < 0 then
        return false
    end

    -- We're only interested in exec completion if this is the first word of the
    -- line, or the first word after a command separator.
    if clink.get_setting_int("space_prefix_match_files") > 0 then
        if first > 1 then
            return false
        end
    else
        local leading = rl_state.line_buffer:sub(1, first - 1)
        local is_first = leading:find("^%s*\"*$")
        if not is_first then
            return false
        end
    end

    -- Strip off any path components that may be on text
    local prefix = ""
    local i = text:find("[\\/:][^\\/:]*$")
    if i then
        prefix = text:sub(1, i)
    end

    -- Extract any possible extension that maybe on the text being completed.
    local ext = nil
    local dot = text:find("%.[^.]*")
    if dot then
        ext = text:sub(dot):lower()
    end

    local suffices = clink.split(clink.get_env("pathext"), ";")
    for i = 1, #suffices, 1 do
        local suffix = suffices[i]

        -- Does 'text' contain some of the suffix (i.e. "cmd.e")? If it does
        -- then merge them so we get "cmd.exe" rather than "cmd.*.exe".
        if ext and suffix:sub(1, #ext):lower() == ext then
            suffix = ""
        end

        suffices[i] = text.."*"..suffix
    end

    -- First step is to match executables in the environment's path.
    if not text:find("[\\/:]") then
        local paths = get_environment_paths()
        for _, suffix in ipairs(suffices) do
            for _, path in ipairs(paths) do
                clink.match_files(path..suffix, false)
            end
        end

        -- If the terminal is cmd.exe check it's commands for matches.
        if clink.get_host_process() == "cmd.exe" then
            clink.match_words(text, dos_commands)
        end

        -- Lastly add console aliases as matches.
        local aliases = clink.get_console_aliases()
        clink.match_words(text, aliases)
    elseif match_style < 1 then
        -- 'text' is an absolute or relative path. If we're doing Bash-style
        -- matching should now consider directories.
        match_style = 2
    end

    -- Optionally include executables in the cwd (or absolute/relative path).
    if clink.match_count() == 0 or match_style >= 1 then
        for _, suffix in ipairs(suffices) do
            clink.match_files(suffix)
        end
    end

    -- Lastly we may wish to consider directories too.
    if clink.match_count() == 0 or match_style >= 2 then
        clink.match_files(text.."*", true, clink.find_dirs)
    end

    clink.matches_are_files()
    return true
end

--------------------------------------------------------------------------------
clink.register_match_generator(exec_match_generator, 50)


local function history_match_generator(text, first, last)
    leading = rl_state.line_buffer:sub(1, last):lower()
    if not leading:find("^!") then
        return false
    end

    idx = tonumber(leading:sub(2))
    line = clink.get_history_at(idx)
    clink.add_match(line)

    return true
end
--------------------------------------------------------------------------------
clink.register_match_generator(history_match_generator, 1)

-- vim: expandtab
