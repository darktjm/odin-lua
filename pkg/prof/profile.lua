#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_exe, ODIN_data, ODIN_flags = unpack(arg)

flags = ""
if ODIN_flags ~= "" then flags = " " .. wholefile(ODIN_flags) end

if ODIN_data == "" then
   odin_error("At least one +prof_data parameter must be specified.", 0)
end
data = wholefile(ODIN_data)

odin_log("gprof" .. flags .. " " .. basename(ODIN_exe) .. " " .. data)

runcmd("gprof" .. flags .. " " .. ODIN_exe .. " " .. data, { stdout = 'profile'})
