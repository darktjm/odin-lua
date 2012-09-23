#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_src, ODIN_o = unpack(arg)

o_f = io.open(ODIN_o)
o_of = io.open("o_of", "w")
for src in io.lines(ODIN_src) do
   o = o_f:read();
   o_of:write('%' .. basename(src, true) .. '.o == '..o..'\n')
end
o_f:close()
o_of:close()
