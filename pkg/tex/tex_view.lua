#!/usr/bin/env lua

-- EXEC (tex_view.sh) (:texnames) (+texsppt) => (:tex.view_desc);

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_files, ODIN_search = unpack(arg)
tvd = io.open('tex.view_desc', 'w')
if ODIN_search == '' or ODIN_search == ' ' then
   tvd:close()
   os.exit(0)
end

for f in words(ODIN_files) do
   for d in words(ODIN_search) do
      tvd:write(pathcat(d, f) .. '\n')
   end
   tvd:write("=''\n")
end
tvd:close()


