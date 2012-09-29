#!/usr/bin/env lua

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

ODIN_names, ODIN_ops = unpack(arg)

odin_log('apply_ops ' .. basename(ODIN_names) .. ' ' .. basename(ODIN_ops))

curop = ""
names = io.open(ODIN_names)
vn = io.open('view.names', 'w')
for op in io.lines(ODIN_ops) do
   if op:sub(-1) == "\\" then
      curop = curop .. op .. '\n'
   else
      vn:write(names:read() .. curop .. op .. '\n')
      curop = ""
   end
end
names:close()
vn:close()
