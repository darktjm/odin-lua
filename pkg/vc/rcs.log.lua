#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_v,
ODIN_date, ODIN_rev, ODIN_state, ODIN_who = unpack(args)

-- need to ensure that name ends with ,v
file = basename(ODIN_v)
if string.sub(file, -3) ~= ',v' then
   file = file .. ',v'
   -- original did soft link, but that's not portable
   cp(ODIN_v, file)
else
   file = ODIN_v
end

-- bug fix from original sh: parms are :first, not a file
flag = ""
if ODIN_date ~= "" then flag = flag .. " -d" .. ODIN_date end
if ODIN_rev ~= "" then flag = flag .. ' -r' .. ODIN_rev end
if ODIN_state ~= "" then flag = flag .. ' -s' .. ODIN_state end
if ODIN_who ~= "" then flag = flag .. ' -w' .. ODIN_who end

odin_log('rlog' .. flag .. ' ' .. basename(file))

-- fix from sh version: output file is log, not rcs
runcmd('rlog' .. flag, { file, stdout = 'log' })

if file ~= ODIN_v then rm(file) end
