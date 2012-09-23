#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_sccs, ODIN_date, ODIN_rev = unpack(arg)

-- need to ensure that name starts with s.
file = basename(ODIN_sccs)
if string.sub(file, 1, 2) ~= 's.' then
   file = 's.' .. file
   -- original did soft link, but that's not portable
   cp(ODIN_sccs, file)
else
   file = ODIN_sccs
end

flag = ""
if ODIN_date ~= "" then flag = flag .. " -c" .. ODIN_date end
if ODIN_rev ~= "" then flag = flag .. ' -r' .. ODIN_rev end

odin_log('sccs get -lp' .. flag .. ' ' .. basename(file))

runcmd('sccs get -lp' .. flag, {file, stdout = 'sccs.log'})
