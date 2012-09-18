#!/usr/bin/env lua

ODIN_src, ODIN_o = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

o_f = io.open(ODIN_o)
o_of = io.open("o_of", "w")
for src in io.lines(ODIN_src) do
   o = o_f:read();
   o_of:write('%' .. apr.filepath_name(src, true) .. '.o == '..o..'\n')
end
o_f:close()
o_of:close()
