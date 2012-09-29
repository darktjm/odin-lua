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

odin_log("tbl " .. basename(arg[1]))

runcmd("tbl", {stdin = arg[1], stdout = 'tbl'})
