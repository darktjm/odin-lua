#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_default = unpack(arg)

if ODIN_default == '' then
   odin_error('No default file specified', 0)
end

if apr.stat(ODIN_FILE, 'type') == 'file' then
   append_line("boot_name", ODIN_FILE)
else
   append_line("boot_name", ODIN_default)
end
