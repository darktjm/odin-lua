#!/usr/bin/env lua

-- EXEC (tex.sh) (:fmtcmd) (:rootName) (:texsp) (:tex.vtd :texauxin.vtd :vir_dir)&
--    NEEDS (:texbasis :extract=:tex) (:texbasis :extract=:sty)
--      (:texbasis :extract=:cls)
--      (:tex.vtd :texauxin.vtd :vir_tgt.list)&
--    => (:dvi) (:texpdf) (:tex.log) (:citations) (:indexntry) (:texauxout);

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_fmtcmd, ODIN_root, ODIN_search, ODIN_aux = unpack(args)
mkdir('texauxout')

sp = ""
for d in words(ODIN_search) do sp = apr.filepath_list_merge(sp, d) end
sp = apr.filepath_list_merge('.', apr.filepath_list_merge(sp, getenv("TEXINPUTS")))
setenv('TEXINPUTS', sp)

if is_dir(ODIN_aux) then
   for f in apr.glob(pathcat(ODIN_aux, '*')) do
      -- original had unsafe 'chmod +w' instead of chmod 'u+w'
      cp(f, pathcat('texauxout', basename(f)), 'rw-r--r--')
   end
end

runcmd(wholefile(ODIN_fmtcmd), {wholefile(ODIN_root) .. '.tex', chdir = 'texauxout'})

c = io.open("citations", "w")
for f in apr.glob(pathcat('texauxout', '*')) do
   b, e = basename(f, true)
   if e == '.dvi' then mv(f, 'dvi') end
   if e == '.pdf' then mv(f, 'texpdf') end
   if e == '.log' then mv(f, 'tex.log') else touch('tex.log') end
   if e == '.aux' then
      for l in io.lines(f) do
	 if string.find(l, '\\citation') or
	    string.find(l, '\\bibstyle') or
	    string.find(l, '\\bibdata') then
	    c:write(l .. '\n')
	 end
      end
   end
   if e == '.idx' then cp(f, 'indexntry') else touch('indexntry')
end
c:close()

if is_dir(ODIN_aux) and not cmp(ODIN_aux, 'texauxout') then
   rm('dvi') rm('texpdf')
end

for f in apr.glob(pathat('texauxout', '*.bbl')) do
   rm(f)
end
for f in apr.glob(pathat('texauxout', '*.ind')) do
   rm(f)
end
