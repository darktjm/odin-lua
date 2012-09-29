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
if ODIN_purify ~= "" then
   compiler="purify " .. wholefile(ODIN_purify) .. " " .. compiler
end

args={}
objs=''
for o in apr.glob(pathcat(ODIN_o, "*")) do
   o = basename(o)
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
   odir = apr.dir_open(ODIN_o)
   for endle in odir:entries('type', 'path') do
      if endle.type ~= 'link' then
	 rm(endle.path)
      end
   end
end

if is_exec("exe.exe") then
   -- original renamed to mvtmp first, then to exe; not sure why
   -- seems pointless, so I'm not going to do it.
   mv("exe.exe", "exe")
end
