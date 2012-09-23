#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

-- EXEC (root.lua) (:tex.vtd) (+texroot)
--   NEEDS (:tex.vtd :vir_tgt.list)
--   => (:rootName) (:rootFileName);

ODIN_vtd, ODIN_root = unpack(arg)

if ODIN_root ~= '' and ODIN_root ~= ' ' then
   temp = ODIN_root
else
   temp = ""
   for l in io.lines(ODIN_vtd) do
      l = string.gsub(l, '[ \t]', '')
      nl = string.gsub(l, '^%%(.*)==.*', '%1\n')
      if nl ~= l then
	 temp = temp .. nl .. '\n'
      end
   end
end

if temp ~= "" then
   temp = string.gsub(temp, '.tex\n', '')
   rf = io.open('rootName', 'w')
   rf:write(temp)
   rf:close()
   temp = string.gsub(temp, '\n', '.tex\n')
   rf = io.open('rootFileName', 'w')
   rf:write(temp)
   rf:close()
else 
   odin_error('No document root found')
end     
