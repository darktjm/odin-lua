#!/usr/bin/env lua

ODIN_o, ODIN_lib, ODIN_home,
ODIN_gnu, ODIN_purify, ODIN_debug,
ODIN_prof, ODIN_eprof, ODIN_cc, ODIN_flags = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

path = apr.filepath_list_split(getenv("PATH"));
if getenv("ODIN_CC_HOME") ~= "" then
   table.insert(path, getenv("ODIN_CC_HOME"))
   apr.env_set("PATH", apr.filepath_list_merge(path))
end
if ODIN_home ~= "" then
   table.insert(path, ODIN_home)
   apr.env_set("PATH", apr.filepath_list_merge(path))
end

compiler = getenv("ODIN_CC")
if ODIN_cc ~= "" then compiler = ODIN_cc end
if ODIN_gnu ~= "" then compiler = "gcc" end
if ODIN_purify ~= "" then
   compiler="purify " .. wholefile(ODIN_purify) .. " " .. compiler
end

args={}
objs=''
for o in apr.glob(apr.filepath_merge(ODIN_o, "*")) do
   o = apr.filepath_name(o)
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

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. compiler .. flags .. " " .. objs .. " " .. libs .. " -o exe\n")
end

cwd = apr.filepath_get(1)
exe=apr.filepath_merge(cwd, "exe", 'native')
table.insert(args, '-o')
table.insert(args, exe)
args.stdout = 1
args.chdir = ODIN_o
runcmd(compiler .. flags .. libs, args)
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
   -- original renamed to mvtmp first, then to exe; not sure why
   -- seems pointless, so I'm not going to do it.
   apr.file_rename("exe.exe", "exe")
end
