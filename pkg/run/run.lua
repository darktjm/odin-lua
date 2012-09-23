#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

-- uses odinfile instead of shell for a little more portability, I guess
-- really, all uses of run should be replaced with lua_run

ODIN_dir, ODIN_cmd = unpack(arg)

if ODIN_cmd == "" then
   odin_error(":run requires a +cmd parameter", 0)
end
cmd = wholefile(ODIN_cmd)

append_line('run', '@!cd ' .. ODIN_dir .. '\\\n' ..
		   string.gsub(string.gsub(cmd, '\\', '\\\\'), '\n', '\\\n'))
