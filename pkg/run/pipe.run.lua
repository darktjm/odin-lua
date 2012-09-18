#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_cmd = unpack(arg)

cmd = "cat"
if ODIN_cmd ~= "" then
   cmd = wholefile(ODIN_cmd)
end

odin_log(cmd)

apr.dir_make('output')
runcmd("sh", { "-c", cmd, chdir = 'output', stdin = ODIN_FILE, stdout = 'stdout' })
