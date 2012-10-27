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

ODIN_o, ODIN_lib, ODIN_home,
ODIN_gnu, ODIN_purify, ODIN_debug,
ODIN_prof, ODIN_eprof, ODIN_cc, ODIN_flags = unpack(arg)

path = split_path(getenv("PATH"));
if getenv("ODIN_CC_HOME") ~= "" then
   table.insert(path, getenv("ODIN_CC_HOME"))
   setenv("PATH", build_path(path))
end
if ODIN_home ~= "" then
   table.insert(path, ODIN_home)
   setenv("PATH", build_path(path))
end

compiler = getenv("ODIN_CC")
if ODIN_cc ~= "" then compiler = ODIN_cc end
if ODIN_gnu ~= "" then compiler = "gcc" end
if ODIN_purify ~= "" then
   compiler="purify " .. wholefile(ODIN_purify) .. " " .. compiler
end

args={}
objs=''
for o in glib.dir(ODIN_o) do
   objs = objs .. ' ' .. o
   table.insert(args, o)
end

flags=""
if ODIN_debug ~= "" then flags = flags .. " " .. getenv("ODIN_CC_DEBUGF") end
if ODIN_prof ~= "" then flags = flags .. " -pg" end
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end
flags=flags .. " " .. getenv("ODIN_CC_FLAGS")

libs=""
if ODIN_lib ~= "" then libs=wholefile(ODIN_lib) end

odin_log(compiler .. flags .. " " .. objs .. " " .. libs .. " -o exe")

exe=pathcat(getcwd(), "exe")
table.insert(args, '-o')
table.insert(args, exe)
args.chdir = ODIN_o
runcmd(compiler .. flags .. libs, args)

if ODIN_purify ~= "" then
   for endle in glib.dir(ODIN_o) do
      if is_link(pathcat(ODIN_o, endle)) then
	 rm(pathcat(ODIN_o, endle))
      end
   end
end

if is_exec("exe.exe") then
   -- original renamed to mvtmp first, then to exe; not sure why
   -- seems pointless, so I'm not going to do it.
   mv("exe.exe", "exe")
end
