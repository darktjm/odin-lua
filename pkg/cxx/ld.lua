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

ODIN_o, ODIN_lib, ODIN_ptr,
ODIN_define, ODIN_incsp, ODIN_gnu,
ODIN_purify, ODIN_debug, ODIN_prof,
ODIN_eprof, ODIN_cxx, ODIN_flags = unpack(arg)

path = split_path(getenv("PATH"));
if getenv("ODIN_CXX_HOME") ~= "" then
   table.insert(path, getenv("ODIN_CXX_HOME"))
   setenv("PATH", build_path(path))
end

if getenv("ODIN_CXX_LD_LIBRARY_PATH") ~= "" then
   LD_LIBRARY_PATH = split_path(getenv("LD_LIBRARY_PATH"))
   for i,v in ipairs(split_path(getenv("ODIN_CXX_LD_LIBRARY_PATH")) do
      table.insert(LD_LIBRARY_PATH, v)
   end
   setenv("LD_LIBRARY_PATH", build_path(LD_LIBRARY_PATH))
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
for o in glib.dir(ODIN_o) do
   o = basename(o)
   objs = objs .. ' ' .. o
   table.insert(args, o)
end

libs=""
if ODIN_lib ~= "" then libs=wholefile(ODIN_lib) end

odin_log(compiler .. flags .. " " .. objs .. " " .. libs .. " -o exe")

exe=pathcat(getcwd(), "exe")
table.insert(args, '-o')
table.insert(args, exe)
args.stdout = 'MESSAGES'
args.chdir = ODIN_o
if not runcmd(compiler .. flags .. libs, args) then
   ignerr = getenv("ODIN_CXX_IGNORE_ERR")
   if ignerr ~= "" then
      ignerr, msg = glib.regex_new(ignerr)
      if ignerr then
	 for l in io.lines("ERRORS") do
	    if ignerr:find(l) then
	       mv("ERRORS", "WARNINGS")
	       break
	    end
	 end
      end
   end
end

msg = io.open("MESSAGES")
if msg then io.write(msg:read('*a')); msg:close() end

if ODIN_purify ~= "" then
   for endle in glib.dir(ODIN_o) do
      if is_link(pathcat(ODIN_o, endle)) then
	 rm(pathcat(ODIN_o, endle))
      end
   end
end

if is_exec("exe.exe") then
   mv("exe.exe", "exe")
end
