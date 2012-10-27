#!/usr/bin/env lua

-- EXEC (texauxin.lua) (:rootName) (:texauxout)& (:tex.bbl) (:tex.ind)
--   => (:texauxin.vtd);

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

ODIN_root, ODIN_aux, ODIN_bbl, ODIN_ind = unpack(arg)
vtd = io.open('texauxin.vtd', 'w')

if is_dir(ODIN_aux) then
   for f in glib.dir(ODIN_aux) do
      vtd:write('%' .. f .. '==' .. pathcat(ODIN_aux, f) .. '\n')
   end
end

vtd:write('%' .. wholefile(ODIN_root) .. '.bbl==' .. ODIN_bbl .. '\n')
vtd:write('%' .. wholefile(ODIN_root) .. '.ind==' .. ODIN_ind .. '\n')
