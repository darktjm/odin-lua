#!/usr/bin/env lua

rex = require 'rex_posix'

ODIN_o, ODIN_lib, ODIN_ptr,
ODIN_define, ODIN_incsp, ODIN_gnu,
ODIN_purify, ODIN_debug, ODIN_prof,
ODIN_eprof, ODIN_cxx, ODIN_flags = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

path = apr.filepath_list_split(getenv("PATH"));
if getenv("ODIN_CXX_HOME") ~= "" then
   table.insert(path, getenv("ODIN_CXX_HOME"))
   apr.env_set("PATH", apr.filepath_list_merge(path))
end

if getenv("ODIN_CXX_LD_LIBRARY_PATH") ~= "" then
   LD_LIBRARY_PATH = apr.filepath_list_split(getenv("LD_LIBRARY_PATH"))
   for i,v in ipairs(apr.filepath_list_split(getenv("ODIN_CXX_LD_LIBRARY_PATH")) do
      table.insert(LD_LIBRARY_PATH, v)
   end
   apr.env_set("LD_LIBRARY_PATH", apr.filepath_list_merge(LD_LIBRARY_PATH))
end

compiler = getenv("ODIN_CXX")
if ODIN_cc ~= "" then compiler = ODIN_cc end
if ODIN_gnu ~= "" then compiler = "g++" end
if ODIN_purify ~= "" then
   compiler="purify " .. wholefile(ODIN_purify) .. " " .. compiler
end

flags=""
if getenv("ODIN_CXX_PTR") == "1" and ODIN_gnu == "" then
   flags = flags .. " -ptr" .. ODIN_ptr
end
if ODIN_define ~= "" then
   for def in words(ODIN_define) do
      flags = flags .. " -D" .. def
   end
end
if ODIN_incsp ~= "" then
   for sp in words(ODIN_incsp) do
      flags = flags .. " -I" .. sp
   end
end
if ODIN_debug ~= "" then flags = flags .. " " .. getenv("ODIN_CXX_DEBUGF") end
if ODIN_prof ~= "" then flags = flags .. " -pg" end
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end
flags=flags .. " " .. getenv("ODIN_CXX_FLAGS")

args={}
objs=''
for o in apr.glob(apr.filepath_merge(ODIN_o, "*")) do
   o = apr.filepath_name(o)
   objs = objs .. ' ' .. o
   table.insert(args, o)
end

libs=""
if ODIN_lib ~= "" then libs=wholefile(ODIN_lib) end

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. compiler .. flags .. " " .. objs .. " " .. libs .. " -o exe\n")
end

cwd = apr.filepath_get(1)
exe=apr.filepath_merge(cwd, "exe", 'native')
table.insert(args, '-o')
table.insert(args, exe)
args.stdout = 1
args.chdir = ODIN_o
if not runcmd(compiler .. flags .. libs, args) then
   ignerr = getenv("ODIN_CXX_IGNORE_ERR")
   if ignerr ~= "" then
      ok, msg = pcall(rex.new, ignerr)
      if ok then
	 for l in io.open("ERRORS"):lines() do
	    if msg:exec(l) then
	       apr.file_rename("ERRORS", "WARNINGS")
	       break
	    end
	 end
      end
   end
end

msg = io.open("MESSAGES")
if msg then io.write(msg:read('*a')) end

if ODIN_purify ~= "" then
   odir = apr.dir_open(ODIN_o)
   for endle in odir:entries('type', 'path') do
      if endle.type ~= 'link' then
	 apr.file_remove(endle.path)
      end
   end
end

xst = apr.stat("exe.exe", "protection")
if xst then xst = string.sub(xst, 3, 1) end
if xst == 'x' then
   apr.file_rename("exe.exe", "exe")
end
