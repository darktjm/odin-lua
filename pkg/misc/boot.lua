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

ODIN_FILE, ODIN_default = unpack(arg)

if ODIN_default == '' then
   odin_error('No default file specified', 0)
end

if is_file(ODIN_FILE) then
   append_line("boot_name", ODIN_FILE)
else
   append_line("boot_name", ODIN_default)
end
