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

ODIN_idl, ODIN_hdir, ODIN_a = unpack(arg)

name = basename(ODIN_idl, true)
hdir = wholefile(ODIN_hdir)
a = wholefile(ODIN_a)

it = io:open('idl_targets', 'w')
it:write(name .. ".h == " .. pathcat(hdir, name .. '.h') .. '\n')
it:write(name .. "C.H == " .. pathcat(hdir, name .. "C.H") .. '\n')
it:write(name .. "S.H == " .. pathcat(hdir, name .. "S.H") .. '\n')
it:write('lib' .. name .. '.a == %lib' .. name .. '.a\n')
it:write("%lib" .. name .. '.a == ' .. a .. '\n')
