#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

sn = io.open(arg[1])
vo = io.open('vw.of', 'w')
for l in io.lines(arg[2]) do
   vo:write('%' .. basename(sn:read()) .. ' == ' .. l .. '\n')
end
sn:close()
vo:close()
