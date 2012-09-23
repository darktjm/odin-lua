#!/usr/bin/env lua

-- EXEC (bibtex.sh) (:citations)& (:texsp)
--    NEEDS (:texbasis :extract=:bib) (:texbasis :extract=:bst)
--    => (:tex.bbl);

ODIN_cite, ODIN_search = unpack(arg)

got_cit = false
got_dat = false
for l in io.lines(ODIN_cite) do
   if string.find(l, '\\citation') then got_cit = true end
   if string.find(l, '\\bitdata') then got_dat = true end
end
if not got_cit or not got_dat then
   touch('tex.bbl')
   os.exit(0)
end

sp = ""
for d in words(ODIN_search) do sp = apr.filepath_list_merge(sp, d) end
setenv('BIBINPUTS', pr.filepath_list_merge(sp, getenv("BIBINPUTS")))

if getenv('BSTINPUTS') ~= '' then
   setenv('BSTINPUTS', pr.filepath_list_merge(sp, getenv("BSTINPUTS")))
else
   setenv('BSTINPUTS', pr.filepath_list_merge(sp, getenv("TEXINPUTS")))
end

ln(ODIN_cite, 'tex.aux')
runcmd('bibtex tex')
