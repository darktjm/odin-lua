#!/usr/bin/env lua

-- EXEC (tex.lua) (:fmtcmd) (:rootName) (:texsp) (:tex.vtd :texauxin.vtd :vir_dir)&
--    NEEDS (:texbasis :extract=:tex) (:texbasis :extract=:sty)
--      (:texbasis :extract=:cls)
--      (:tex.vtd :texauxin.vtd :vir_tgt.list)&
--    => (:dvi) (:texpdf) (:tex.log) (:citations) (:indexntry) (:texauxout);

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

ODIN_fmtcmd, ODIN_root, ODIN_search, ODIN_aux = unpack(arg)
mkdir('texauxout')

tex_progname = wholefile(ODIN_fmtcmd)
dofile(pathcat(pathcat(pathcat(getenv("ODINCACHE"), 'PKGS'), 'tex'), 'path.lua'))

sp = {'.'}
for d in words(ODIN_search) do table.insert(sp, d) end
for i, v in ipairs(apr.filepath_list_split(getenv("TEXINPUTS"))) do
   table.insert(sp, v)
end
-- kpathsea tacks on built-in path if final element is empty
-- but apr.filepath_list_merge doesn't support empty elements
-- table.insert(sp, 'zZzZ')
-- setenv('TEXINPUTS', apr.filepath_list_merge(sp):gsub('zZzZ', ''))
setenv('TEXINPUTS', apr.filepath_list_merge(sp))

if is_dir(ODIN_aux) then
   for f in apr.glob(pathcat(ODIN_aux, '*')) do
      -- original had unsafe 'chmod +w' instead of chmod 'u+w'
      cp(f, pathcat('texauxout', basename(f)), 'rw-r--r--')
   end
end

odin_log(wholefile(ODIN_fmtcmd) .. ' ' .. wholefile(ODIN_root) .. '.tex')

runcmd(wholefile(ODIN_fmtcmd), {wholefile(ODIN_root) .. '.tex', chdir = 'texauxout'})

touch('tex.log')
touch('indexntry')
c = io.open("citations", "w")
for f in apr.glob(pathcat('texauxout', '*')) do
   b, e = basename(f, true)
   if e == '.dvi' then mv(f, 'dvi') end
   if e == '.pdf' then mv(f, 'texpdf') end
   if e == '.log' then mv(f, 'tex.log') end
   if e == '.aux' then
      for l in io.lines(f) do
	 if l:find('\\citation') or 
	    l:find('\\bibstyle') or
	    l:find('\\bibdata') then
	    c:write(l .. '\n')
	 end
      end
   end
   if e == '.idx' then cp(f, 'indexntry') end
end
c:close()

if is_dir(ODIN_aux) and not cmp(ODIN_aux, 'texauxout') then
   rm('dvi') rm('texpdf')
end

for f in apr.glob(pathcat('texauxout', '*.bbl')) do
   rm(f)
end
for f in apr.glob(pathcat('texauxout', '*.ind')) do
   rm(f)
end
