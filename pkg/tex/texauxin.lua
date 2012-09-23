#!/usr/bin/env lua

-- EXEC (texauxin.lua) (:rootName) (:texauxout)& (:tex.bbl) (:tex.ind)
--   => (:texauxin.vtd);

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_root, ODIN_aux, ODIN_bbl, ODIN_ind = unpack(arg)
vtd = io.open('texauxin.vtd', 'w')

if is_dir(ODIN_aux) then
   for f in apr.glob(pathcat(ODIN_aux, '*')) do
      f = basename(f)
      vtd:write('%' .. f .. '==' .. pathcat(ODIN_aux, f) .. '\n')
   end
end

vtd:write('%' .. wholefile(ODIN_root) .. '.bbl==' .. ODIN_bbl .. '\n')
vtd:write('%' .. wholefile(ODIN_root) .. '.ind==' .. ODIN_ind .. '\n')
