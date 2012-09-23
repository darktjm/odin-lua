#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_names, ODIN_ops = unpack(arg)

odin_log('apply_ops ' .. basename(ODIN_names) .. ' ' .. basename(ODIN_ops))

curop = ""
names = io.open(ODIN_names)
vn = io.open('view.names', 'w')
for op in io.lines(ODIN_ops) do
   if string.sub(op, -1) == "\\" then
      curop = curop .. op .. '\n'
   else
      vn:write(names:read() .. curop .. op .. '\n')
      curop = ""
   end
end
names:close()
vn:close()
