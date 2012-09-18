#!/usr/bin/env lua

ODIN_source, ODIN_ptr,
ODIN_dir, ODIN_incsp, ODIN_gnu,
ODIN_debug, ODIN_prof, ODIN_optimize,
ODIN_define, ODIN_cxx, ODIN_flags,
ODIN_abort = unpack(arg)

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
if ODIN_cxx ~= "" then compiler = ODIN_cxx end
if ODIN_gnu ~= "" then compiler = "g++" end

flags = ""
if getenv("ODIN_CXX_PTR") == "1" and ODIN_gnu == "" then
   flags = flags .. " -ptr" .. ODIN_ptr
end
if ODIN_debug ~= "" then flags = flags .. " " .. getenv("ODIN_CXX_DEBUGF") end
if ODIN_prof ~= "" then flags = flags .. " -pg" end
if ODIN_optimize ~= "" then flags = flags .. " -O" .. ODIN_optimize end
if ODIN_define ~= "" then
   for def in words(ODIN_define) do
      flags = flags .. " -D" .. def
   end
end
flags = flags .. " -I" .. ODIN_dir
if ODIN_incsp ~= "" then
   for sp in words(ODIN_incsp) do
      flags = flags .. " -I" .. sp
   end
end
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end
flags=flags .. " " .. getenv("ODIN_CXX_FLAGS")

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. compiler .. flags .. " -c " .. apr.filepath_name(ODIN_source))
end

-- emulate old unsafe behavior for cmd line options, but quote ODIN_source
if not runcmd(compiler .. flags , {'-c', ODIN_source}) then
   -- old code did some message mangling that's highly compiler dependent
   -- old code used abort_msgs file; I'm just doing it in-line
   for l in io.lines("ERRORS") do
      if string.find(l, "Out of virtual memory") or
         string.find(l, "virtual memory exhausted") then
	 errs = io.open("ERRORS")
	 io.write(errs:read('*a'))
	 errs:close()
	 apr.file_remove("ERRORS")
	 os.exit(1)
      end
   end
else
   -- yet more message mangling that's highly compiler dependent
   apr.file_rename("WARNINGS", "MSGS")
   wrn = io.open("WARNINGS", "w")
   for l in io.lines("MSGS") do
      if string.find(l, "arning") then
	 if not string.find(l, "& before array or function: ignored") then
	    wrn:write(l)
	 end
      else
	 io.write(l)
      end
   end
end

input = apr.filepath_name(ODIN_source, true)
if apr.stat(input .. '.o', 'type') == 'file' then apr.file_rename(input .. '.o', 'o') end
