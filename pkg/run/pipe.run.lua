#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

-- this is inherently unportable; use pipe.lua_run.lua instead

ODIN_FILE, ODIN_cmd = unpack(arg)

cmd = "cat"
if ODIN_cmd ~= "" then
   cmd = wholefile(ODIN_cmd)
end

odin_log(cmd)

mkdir('output')
runcmd("sh", { "-c", cmd, chdir = 'output', stdin = ODIN_FILE, stdout = 'stdout' })
