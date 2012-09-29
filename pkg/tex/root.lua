#!/usr/bin/env lua

-- EXEC (root.lua) (:tex.vtd) (+texroot)
--   NEEDS (:tex.vtd :vir_tgt.list)
--   => (:rootName) (:rootFileName);

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

ODIN_vtd, ODIN_root = unpack(arg)

if ODIN_root ~= '' and ODIN_root ~= ' ' then
   temp = ODIN_root
else
   temp = ""
   for l in io.lines(ODIN_vtd) do
      l = l:gsub('[ \t]', '')
      nl = l:gsub('^%%(.*)==.*', '%1\n')
      if nl ~= l then
	 temp = temp .. nl .. '\n'
      end
   end
end

if temp ~= "" then
   temp = temp:gsub('.tex\n', '')
   rf = io.open('rootName', 'w')
   rf:write(temp)
   rf:close()
   temp = temp:gsub('\n', '.tex\n')
   rf = io.open('rootFileName', 'w')
   rf:write(temp)
   rf:close()
else 
   odin_error('No document root found')
end     
