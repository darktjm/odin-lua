#!/usr/bin/env lua

-- EXEC (texsp.lua) (:tex.vtd :vir_dir)@ (:dir)@ (+texsearch)
--   => (:texsp);

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$" -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua"))
end

ODIN_vir_dir, ODIN_dir, ODIN_texsearch = unpack(arg)

texsp = io.open('texsp', 'w')
texsp:write(ODIN_vir_dir .. '\n')
if ODIN_texsearch == '' then
   texsp:write(ODIN_dir .. '\n')
elseif ODIN_texsearch ~= '' then
   for d in io.lines(ODIN_texsearch) do
      if not apr.filepath_root(d, 'native') then
	 odin_error(d .. ' must be an absolute path name')
      else
	 texsp:write(d .. '\n')
      end
   end
end
texsp:close()
