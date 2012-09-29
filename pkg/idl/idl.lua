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

ODIN_idl, ODIN_dir, ODIN_incsp, ODIN_flags = unpack(arg)

if getenv("ODIN_IDL_HOME") ~= "" then
   path = apr.filepath_list_split(getenv("PATH"))
   table.insert(path, getenv("ODIN_IDL_HOME"))
   setenv("PATH", apr.filepath_list_merge(path))
end

flags = " -I" .. ODIN_dir
if ODIN_incsp ~= "" then
   for sp in words(ODIN_incsp) do
      flags = flags .. " -I" .. sp
   end
end
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end
flags=flags .. " " .. getenv("ODIN_IDL_FLAGS")

odin_log(getenv("ODIN_IDL") .. flags .. " " .. basename(ODIN_idl))

runcmd(getenv("ODIN_IDL") .. flags, { ODIN_idl })

input = basename(ODIN_idl, true)
if is_file(input .. '_cstub.c') then mv(input .. '_cstub.c', 'cstub.c') end
if is_file(input .. '_sstub.c') then mv(input .. '_sstub.c', 'sstub.c') end
if is_file(input .. 'C.C') then mv(input .. 'C.C', 'client.C') end
if is_file(input .. 'E.C') then mv(input .. 'E.C', 'epv.C') end

mkdir('idl_h_dir')
for f in apr.glob('*.[hH]') do
   mv(f, pathcat('idl_h_dir', basename(f)))
end
