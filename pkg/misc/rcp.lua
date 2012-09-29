#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$", '') -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua")
end

ODIN_FILE, ODIN_fdest, ODIN_ddest = unpack(arg)

if ODIN_fdest ~= "" then
   dest=wholefile($ODIN_fdest)
elseif ODIN_ddest ~= "" then
   dest=pathcat(wholefile(ODIN_ddest), basename(ODIN_FILE))
else
   odin_error('Either +f_dest or +d_dest must be specified', 0)
end

odin_log("rcp " .. basename(ODIN_FILE) .. " " .. dest)

runcmd("rcp", {ODIN_FILE, dest})
