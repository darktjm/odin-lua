#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_exe, ODIN_core, ODIN_gnu = unpack(arg)

debugger = "dbx"
if ODIN_gnu ~= "" then debugger = "gdb" end

-- using odincmd instead of shell script should be more portable..

append_line('dbx', '@!' .. debugger .. " " .. ODIN_exe .. " " .. ODIN_core)
