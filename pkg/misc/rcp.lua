#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_fdest, ODIN_ddest = unpack(arg)

if ODIN_fdest ~= "" then
   dest=wholefile($ODIN_fdest)
elseif ODIN_ddest ~= "" then
   dest=apr.filepath_merge(wholefile(ODIN_ddest), apr.filepath_name(ODIN_FILE))
else
   odin_error('Either +f_dest or +d_dest must be specified', 0)
end

odin_log("rcp " .. apr.filepath_name(ODIN_FILE) .. " " .. dest)

runcmd("rcp", {ODIN_FILE, dest})
