#!/usr/bin/env lua

ODIN_src, ODIN_o = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end


cmdargs = { }
objs =
d = apr.dir_open(arg[1])
for f in d:entries("name") do
   table.insert(cmdargs, f)
   objs = objs .. " " .. f
end
d:close()

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. 'ar qcv out.a' .. objs)
end

cmdargs.stdout = true
goterr = not runcmd('ar qcv a', cmdargs)

if not goterr then
   ranlib = getenv("ODIN_RANLIB")
   if ranlib ~= "" then
      if getenv("ODINVERBOSE") ~= "" then
	 print(getenv("ODINRBSHOST") .. ranlib .. ' out.a')
      end
      goterr = not runcmd(ranlib, {'a', stdout = true})
   end
end

if not goterr then
   io.write(io.open("MESSAGES"):read('*a'))
end
