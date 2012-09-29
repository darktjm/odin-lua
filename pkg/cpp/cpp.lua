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

ODIN_cpp, ODIN_FILE, ODIN_dir,
ODIN_incsp, ODIN_define = unpack(arg)

flags = ""
if ODIN_define ~= "" then
   for def in words(ODIN_define) do
      flags = flags .. " -D" .. def
   end
end
flags = flags .. " -I" .. ODIN_dir
if ODIN_incsp ~= "" then
   for sp in words(ODIN_incsp) do
      flags = flags .. " -I" .. sp
   end
end
flags=flags .. " " .. getenv("ODIN_CPP_FLAGS")

ODIN_CPP = getenv("ODIN_CPP")
odin_log(ODIN_CPP .. flags .. ' ' .. basename(ODIN_FILE))

runcmd(ODIN_CPP .. flags, {ODIN_FILE, stdout = ODIN_cpp})
