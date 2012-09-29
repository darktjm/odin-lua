#!/usr/bin/env lua

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

ODIN_src, ODIN_o = unpack(arg)

cmdargs = { }
objs =
d = apr.dir_open(arg[1])
for f in d:entries("name") do
   table.insert(cmdargs, f)
   objs = objs .. " " .. f
end
d:close()

odin_log('ar qcv out.a' .. objs)

cmdargs.stdout = 'MESSAGES'
goterr = not runcmd('ar qcv a', cmdargs)

if not goterr then
   ranlib = getenv("ODIN_RANLIB")
   if ranlib ~= "" then
      odin_log(ranlib .. ' out.a')
      goterr = not runcmd(ranlib, {'a', stdout = true})
   end
end

if not goterr and not is_empty("MESSAGES") then
   cat("MESSAGES")
   rm("MESSAGES")
end
