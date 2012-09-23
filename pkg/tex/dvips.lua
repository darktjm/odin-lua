#! /bin/sh

-- EXEC (dvips.sh) (:dvi) (:texsp) (+dvips_flags)
--    NEEDS (:texbasis :extract=:ps)
--    => (:texps);

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_dvi, ODIN_search, ODIN_flags = unpack(arg)

flags = ''
if ODIN_flags ~= '' then
   flags = wholefile(ODIN_flags)
end

sp = ''
for d in words(ODIN_search) do sp = apr.filepath_list_merge(sp, d) end
sp = apr.filepath_list_merge(sp, getenv("TEXINPUTS"))
setenv('TEXINPUTS', sp)

runcmd('dvips ' .. flags, {'-o', 'texps', ODIN_dvi})
