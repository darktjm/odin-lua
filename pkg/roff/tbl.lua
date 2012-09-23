#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

odin_log("tbl " .. basename(arg[1]))

runcmd("tbl", {stdin = arg[1], stdout = 'tbl'})
