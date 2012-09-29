#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$" -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua"))
end

ODIN_v,
ODIN_date, ODIN_rev, ODIN_state, ODIN_who = unpack(args)

-- need to ensure that name ends with ,v
file = basename(ODIN_v)
if file:sub(-3) ~= ',v' then
   file = file .. ',v'
   ln(ODIN_v, file)
else
    file = ODIN_v
end

flag = ""
if ODIN_date ~= "" then flag = flag .. " -d" .. ODIN_date end
if ODIN_rev ~= "" then flag = flag .. ' -r' .. ODIN_rev end
if ODIN_state ~= "" then flag = flag .. ' -s' .. ODIN_state end
if ODIN_who ~= "" then flag = flag .. ' -w' .. ODIN_who end

odin_log('co -p -q' .. flag .. ' ' .. basename(file))

runcmd('co -p -q' .. flag, { file, stdout = 'co' })

if file ~= ODIN_v then rm(file) end
