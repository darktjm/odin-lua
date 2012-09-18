#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

-- This is inherently non-portable, since it needs to generate an
-- executable.  For now it still uses the old shell script.

ODIN_dir, ODIN_cmd = unpack(arg)

if ODIN_cmd == "" then
   odin_error(":run requires a +cmd parameter", 0)
end
cmd = wholefile(ODIN_cmd)

run = io.open('run', 'w')
run:write('#!/bin/sh\n')
run:write('cd ' .. ODIN_dir .. '\n')
run:write(cmd .. '\n')
run:close()
apr.file_perms_set('run', 'rwxr-xr-x')
