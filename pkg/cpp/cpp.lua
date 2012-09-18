#!/usr/bin/env lua

ODIN_cpp, ODIN_FILE, ODIN_dir,
ODIN_incsp, ODIN_define = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

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
if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. ODIN_CPP .. flags .. ' ' .. apr.filepath_name(ODIN_FILE))
end

runcmd(ODIN_CPP .. flags, {ODIN_FILE, stdout = true})

apr.file_rename("MESSAGES", ODIN_cpp)
