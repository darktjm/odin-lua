#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_home = unpack(arg)

odin_log('scan_for_includes ' .. apr.filepath_name(ODIN_FILE))

sis = io.open('so_inc_spec', 'w')
for l in io.lines(ODIN_FILE) do
   m = string.match(l, '^%.so[ \t]*(.*)$')
   if m then
      if not apr.filepath_root(m) then
	 m = apr.filepath_merge(ODIN_home, m, 'native')
	 sis:write(m .. '\n')
      end
   end
end
sis:close()
