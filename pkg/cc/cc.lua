#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_c, ODIN_dir, ODIN_incsp,
ODIN_home, ODIN_gnu, ODIN_debug,
ODIN_prof, ODIN_optimize, ODIN_define,
ODIN_cc, ODIN_flags = unpack(arg)

path = apr.filepath_list_split(getenv("PATH"));
if getenv("ODIN_CC_HOME") ~= "" then
   table.insert(path, getenv("ODIN_CC_HOME"))
   setenv("PATH", apr.filepath_list_merge(path))
end
if ODIN_home ~= "" then
   table.insert(path, ODIN_home)
   setenv("PATH", apr.filepath_list_merge(path))
end

compiler = getenv("ODIN_CC")
if ODIN_cc ~= "" then compiler = ODIN_cc end
if ODIN_gnu ~= "" then compiler = "gcc" end

flags = ""
if ODIN_debug ~= "" then flags = flags .. " " .. getenv("ODIN_CC_DEBUGF") end
if ODIN_prof ~= "" then flags = flags .. " -pg" end
if ODIN_optimize ~= "" then flags = flags .. " -O" .. ODIN_optimize end
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
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end
flags=flags .. " " .. getenv("ODIN_CC_FLAGS")

odin_log(compiler .. flags .. " -c " .. basename(ODIN_c))

-- emulate old unsafe behavior for cmd line options, but quote ODIN_c
-- also, eliminate final 'mv' step using -o option
runcmd(compiler .. flags , {'-c', '-o', pathcat(getcwd(), 'o'), ODIN_c})
