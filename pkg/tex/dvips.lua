#!/usr/bin/env lua

-- EXEC (dvips.lua) (:dvi) (:texsp) (+dvips_flags)
--    NEEDS (:texbasis :extract=:ps)
--    => (:texps);

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

ODIN_dvi, ODIN_search, ODIN_flags = unpack(arg)

flags = ''
if ODIN_flags ~= '' then
   flags = wholefile(ODIN_flags)
end

tex_progname = 'dvips'

sp = {}
for d in words(ODIN_search) do table.insert(sp, d) end
if glib.getenv("TEXINPUTS") then
    for i, v in ipairs(split_path(getenv("TEXINPUTS"))) do
	table.insert(sp, v)
    end
else
    table.insert(sp, '')
end
setenv('TEXINPUTS', build_path(sp))

runcmd('dvips ' .. flags, {'-o', 'texps', ODIN_dvi})
