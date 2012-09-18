#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_other = unpack(arg)

if $ODIN_other == "" then
   odin_error('No comparison file specified', 0)
end

odin_log("diff " .. apr.filepath_name(ODIN_FILE) .. " " .. apr.filepath_name(ODIN_other))

-- note: this differs from the sh version in that errors are always
-- produced if files are different.
-- This is probably meant for interactive only usage anyway, so no big deal
-- After all, the diff output isn't captured
runcmd("diff", {ODIN_FILE, ODIN_other})
