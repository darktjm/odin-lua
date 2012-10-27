#!/usr/bin/env lua

-- EXEC (bibtex.lua) (:citations)& (:texsp)
--    NEEDS (:texbasis :extract=:bib) (:texbasis :extract=:bst)
--    => (:tex.bbl);

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

ODIN_cite, ODIN_search = unpack(arg)

got_cit = false
got_dat = false
if is_file(ODIN_cite) then
   for l in io.lines(ODIN_cite) do
      if l:find('\\citation') then got_cit = true end
      if l:find('\\bibdata') then got_dat = true end
   end
end
if not got_cit or not got_dat then
   touch('tex.bbl')
   os.exit(0)
end

tex_progname = 'bibtex'

sp = {}
for d in words(ODIN_search) do table.insert(sp, d) end
bi = {unpack(sp)}
if glib.getenv("BIBINPUTS") then
   for i, v in ipairs(glib.split_path(getenv("BIBINPUTS"))) do
      table.insert(bi, v)
   end
else
   table.insert(bi, '')
end
setenv('BIBINPUTS', build_path(bi))

if getenv('BSTINPUTS') ~= '' then
   bstsrc = 'BSTINPUTS'
else
   bstsrc = 'TEXINPUTS'
end
if glib.getenv(bstsrc) then
   for i, v in ipairs(split_path(getenv(bstsrc))) do
      table.insert(sp, v)
   end
else
   table.insert(sp, '')
end
setenv('BSTINPUTS', build_path(sp))

odin_log('bibtex')

ln(ODIN_cite, 'tex.aux')
runcmd('bibtex tex')

-- cat('tex.blg')
