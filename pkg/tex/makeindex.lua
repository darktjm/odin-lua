#!/usr/bin/env lua

-- EXEC (makeindex.lua) (:indexntry)& (+index_flags)
--   => (:tex.ind) (:tex.ilg);

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

ODIN_idx, ODIN_flags = unpack(arg)

if is_file(ODIN_idx) then
   ln(ODIN_idx, 'tex.idx')
else
   touch('tex.idx')
end

if ODIN_flags ~= '' then
   flags = ' ' .. wholefile(ODIN_flags)
else
   flags = ''
end

touch('tex.log')

runcmd('makeindex -q' .. flags, {'tex.idx'})

-- note: Eli's message normalization code removed for plain odin
