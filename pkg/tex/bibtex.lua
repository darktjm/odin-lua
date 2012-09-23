#!/usr/bin/env lua

-- EXEC (bibtex.lua) (:citations)& (:texsp)
--    NEEDS (:texbasis :extract=:bib) (:texbasis :extract=:bst)
--    => (:tex.bbl);

ODIN_cite, ODIN_search = unpack(arg)

got_cit = false
got_dat = false
if is_file(ODIN_cite) then
   for l in io.lines(ODIN_cite) do
      if string.find(l, '\\citation') then got_cit = true end
      if string.find(l, '\\bibdata') then got_dat = true end
   end
end
if not got_cit or not got_dat then
   touch('tex.bbl')
   os.exit(0)
end

tex_progname = 'bibtex'
dofile(pathcat(pathcat(pathcat(getenv("ODINCACHE"), 'PKGS'), 'tex'), 'path.lua'))

sp = {}
for d in words(ODIN_search) do table.insert(sp, d) end
bi = sp
for i, v in ipairs(apr.filepath_list_split(getenv("BIBINPUTS"))) do
   table.insert(bi, v)
end
setenv('BIBINPUTS', apr.filepath_list_merge(bi))

if getenv('BSTINPUTS') ~= '' then
   bstsrc = 'BSTINPUTS'
else
   bstsrc = 'TEXINPUTS'
end
for i, v in ipairs(apr.filepath_list_split(getenv(bstsrc))) do
   table.insert(sp, v)
end
setenv('BSTINPUTS', apr.filepath_list_merge(sp))

odin_log('bibtex')

ln(ODIN_cite, 'tex.aux')
runcmd('bibtex tex')

-- cat('tex.blg')
