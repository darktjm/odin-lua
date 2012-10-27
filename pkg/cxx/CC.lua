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

ODIN_source, ODIN_ptr,
ODIN_dir, ODIN_incsp, ODIN_gnu,
ODIN_debug, ODIN_prof, ODIN_optimize,
ODIN_define, ODIN_cxx, ODIN_flags,
ODIN_abort = unpack(arg)

path = split_path(getenv("PATH"));
if getenv("ODIN_CXX_HOME") ~= "" then
   table.insert(path, getenv("ODIN_CXX_HOME"))
   setenv("PATH", build_path(path))
end

if getenv("ODIN_CXX_LD_LIBRARY_PATH") ~= "" then
   LD_LIBRARY_PATH = split_path(getenv("LD_LIBRARY_PATH"))
   for i,v in ipairs(split_path(getenv("ODIN_CXX_LD_LIBRARY_PATH"))) do
      table.insert(LD_LIBRARY_PATH, v)
   end
   setenv("LD_LIBRARY_PATH", build_path(LD_LIBRARY_PATH))
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

odin_log(compiler .. flags .. " -c " .. basename(ODIN_source))

-- emulate old unsafe behavior for cmd line options, but quote ODIN_source
-- also, use -o to rename output instead of post-compile mv
if not runcmd(compiler .. flags , {'-c', ODIN_source, '-o', pathcat(getcwd(), 'o')}) then
   -- old code did some message mangling that's highly compiler dependent
   -- old code used abort_msgs file; I'm just doing it in-line
   for l in io.lines("ERRORS") do
      if l:find("Out of virtual memory") or
         l:find("virtual memory exhausted") then
	 errs = io.open("ERRORS")
	 io.write(errs:read('*a'))
	 errs:close()
	 rm("ERRORS")
	 os.exit(1)
      end
   end
else
   -- yet more message mangling that's highly compiler dependent
   mv("WARNINGS", "MSGS")
   wrn = io.open("WARNINGS", "w")
   for l in io.lines("MSGS") do
      if l:find("arning") then
	 if not l:find("& before array or function: ignored") then
	    wrn:write(l)
	 end
      else
	 io.write(l)
      end
   end
   wrn:close()
end

