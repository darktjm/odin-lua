#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_dir, ODIN_cmd = unpack(arg)

if ODIN_cmd == "" then
   odin_error(":run.lua requires a +cmd parameter", 0)
end

-- unlike :run, this retains all formatting

cf = io.open(ODIN_cmd)
cmd = cf:read('*a')
cf:close()

append_line('run.lua', "@!lua!chdir('" .. ODIN_dir .. "')\\\n" ..
		   string.gsub(string.gsub(cmd, '\\', '\\\\'), '\n', '\\\n'))
