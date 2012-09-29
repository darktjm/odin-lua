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

ODIN_FILE, ODIN_home = unpack(arg)

odin_log('scan_for_includes ' .. basename(ODIN_FILE))

sis = io.open('so_inc_spec', 'w')
for l in io.lines(ODIN_FILE) do
   m = l:match('^%.so[ \t]*(.*)$')
   if m then
      if not apr.filepath_root(m, 'native') then
	 m = pathcat(ODIN_home, m)
	 sis:write(m .. '\n')
      end
   end
end
sis:close()
