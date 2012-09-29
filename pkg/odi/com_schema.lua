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

ODIN_schemas, ODIN_schemadirs,
ODIN_dbdir, ODIN_incsp, ODIN_standin = unpack(arg)

path = apr.filepath_list_split(getenv("PATH"));
table.insert(path, getenv("OS_ROOTDIR") .. '/bin')
setenv("PATH", apr.filepath_list_merge(path))
path = apr.filepath_list_split(getenv("LD_LIBRARY_PATH"))
table.insert(path, getenv("OS_ROOTDIR") .. '/lib')
setenv("LD_LIBRARY_PATH", apr.filepath_list_merge(path))

if ODIN_schemas ~= '' then
   touch('com_schema_standin')
   os.exit(0)
end
schemas = wholefile(ODIN_schemas)

if ODIN_dbdir == "" then
   odin_error("+schema_dir must be specified", 0)
end
dbdir = ODIN_dbdir

incsp = ''
for sp in words(ODIN_schemadirs) do
   incsp = incsp .. ' -I' .. sp
end
if ODIN_incsp ~= '' then
   for sp in words(ODIN_incsp) do
      incsp = incsp .. ' -I' .. sp
   end
end

cmd = 'OSCC -batch_schema ' .. dbdir .. '/com_schema ' .. incsp .. ' ' .. schemas
odin_log(cmd)

if not runcmd(cmd) then
   for l in io.lines('WARNINGS') do
      if l:find('Out of virtual memory') then
	 cat('ERRORS')
	 mv('ERRORS', 'WARNINGS')
	 os.exit(1)
      end
   end
end

if not is_file(ODIN_standin) then
   num = 0
else
   si = io.open(ODIN_standin)
   num = si:read('*n')
end
append_line('com_schema_standin', tostring(num + 1))
