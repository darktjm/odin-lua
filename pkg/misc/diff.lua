#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$", '') -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_other = unpack(arg)

if $ODIN_other == "" then
   odin_error('No comparison file specified', 0)
end

odin_log("diff " .. basename(ODIN_FILE) .. " " .. basename(ODIN_other))

-- note: this differs from the sh version in that errors are always
-- produced if files are different.
-- This is probably meant for interactive only usage anyway, so no big deal
-- After all, the diff output isn't captured
runcmd("diff", {ODIN_FILE, ODIN_other})
