#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

odin_log("eqn " .. apr.filepath_name(arg[2]))

runcmd("eqn", {stdin = arg[2], stdout = arg[1]})
