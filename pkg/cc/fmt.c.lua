#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. "indent " .. apr.filepath_name(arg[1]))
end

-- indent is brain-damaged. always returns non-zero status
-- tjm: original version did not have -o, but GNU indent requires it
-- tjm: also, GNU indent returns a valid status, but ignoring it's OK
runcmd("indent", {arg[1], "-o", 'fmt', ignret = 1})
apr.file_rename('WARNINGS', 'ERRORS');
