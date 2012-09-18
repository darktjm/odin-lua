#!/usr/bin/env lua

-- FIXME: this is UNIX-specific
-- Then again, only UNIX people seem to ever use command-line debuggers...

ODIN_exe, ODIN_core, ODIN_gnu = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

debugger = "dbx"
if ODIN_gnu ~= "" then debugger = "gdb" end

dbx = io.open("dbx", "w")
dbx:write("#!/bin/sh\n")
dbx:write(debugger .. " " .. ODIN_exe .. " " .. ODIN_core .. "\n")
dbx:close()
apr.file_perms_set("dbx", "rwxr-xr-x")
