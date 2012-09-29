#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$" -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua"))
end

ODIN_f, ODIN_debug, ODIN_prof,
ODIN_home, ODIN_f77, ODIN_flags = unpack(arg)

if ODIN_home ~= "" then
   path = apr.filepath_list_split(getenv("PATH"));
   table.insert(path, ODIN_home)
   apr.env_set("PATH", apr.filepath_list_merge(path))
end

compiler = "f77"
if ODIN_f77 ~= "" then compiler = ODIN_f77 end

flags = ""
if ODIN_debug ~= "" then flags = flags .. " -g" end
if ODIN_prof ~= "" then flags = flags .. " -pg" end
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end

odin_log(compiler .. flags .. " -c " .. basename(ODIN_f))

-- emulate old unsafe behavior for cmd line options, but quote ODIN_f
-- also, use -o instead of post-compile mv to rename output
runcmd(compiler .. flags , {'-c', ODIN_f, '-o', pathcat(getcwd(), 'o')})
