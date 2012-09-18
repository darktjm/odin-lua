#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

-- hard to do this portably while retaining compatibilty with :run

ODIN_dir, ODIN_cmd = unpack(arg)

if ODIN_cmd == "" then
   odin_error(":run.lua requires a +cmd parameter", 0)
end

run = io.open('run.lua', 'w')
run:write('#!/usr/bin/env lua\n')
run:write('-- in case run from cmd line, grab built-ins\n')
run:write('if not runcmd then\n')
run:write('   dofile(os.getenv("ODINCACHE") .. "/PKGS/odin/odin_builtin.lua")\n')
run:write('end\n')
run:write('apr.filepath_set("' .. ODIN_dir .. '")\n')
for l in io.lines(ODIN_cmd) do
   run:write(l .. '\n')
end
run:close()
apr.file_perms_set('run.lua', 'rwxr-xr-x')
