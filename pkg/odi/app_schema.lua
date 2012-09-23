#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_dbdir, ODIN_objs, ODIN_libs = unpack(arg)

path = apr.filepath_list_split(getenv("PATH"));
table.insert(path, getenv("OS_ROOTDIR") .. '/bin')
setenv("PATH", apr.filepath_list_merge(path))
path = apr.filepath_list_split(getenv("LD_LIBRARY_PATH"))
table.insert(path, getenv("OS_ROOTDIR") .. '/lib')
setenv("LD_LIBRARY_PATH", apr.filepath_list_merge(path))

if ODIN_dbdir == "" then
   touch('schema.C')
   os.exit(0)
end
dbdir = ODIN_dbdir

objs = wholefile(ODIN_objs)

libs = ''
if ODIN_libs ~= "" then
   libs = wholefile(ODIN_libs)
end

cmd = 'os_prelink ' .. schema_C .. ' ' .. dbdir .. '/com_schema ' .. dbdir .. '/app_schema ' .. objs .. ' ' .. libs
odin_log(cmd)

runcmd(cmd)
