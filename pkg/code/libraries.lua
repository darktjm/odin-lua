#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
    dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_lib, ODIN_libsp = unpack(arg)

lib_names = ''
if ODIN_lib ~= "" then lib_names=wholefile(ODIN_lib) end
flags = '' bases = ''
if ODIN_libsp ~= "" then
   flags=string.gsub(' ' .. wholefile(ODIN_libsp), ' ', ' -L')
   bases=bases .. wholefile(ODIN_libsp)
end

odin_log('scan_for_libraries (' .. lib_names .. ") in (" .. bases .. ")")

vd = io.open("libraries.view_desc", "w")
    
for lib_name in string.gmatch(string.gsub(lib_names, "-L ", "-L"), "%S+") do
   if string.sub(lib_name, 1, 1) == '/' then
      vd:write("'" .. lib_name .. "'\n" ..
               "=''\n")
      flags = flags .. ' ' .. lib_name
   elseif string.sub(lib_name, 1, 2) == '-L' then
      bases = bases .. ' ' .. string.sub(lib_name, 3)
      flags = flags .. ' ' .. lib_name
   elseif string.sub(lib_name, 1, 1) == '-' then
      flags = flags .. ' ' .. lib_name
   else
      for lib_base in string.gmatch(bases .. ' ' .. getenv("ODIN_LIB_SP"), "%S+") do
	 for ext in string.gmatch('so sl a', '%S+') do
	    lib = lib_base .. '/lib' .. lib_name .. '.' .. ext
	    vd:write("'" .. lib .. "'\n")
	    for file in apr.glob(lib .. '.[0-9]*.[0-9]*') do
	       vd:write("'" .. file .. "'\n")
	    end
	 end
      end
      vd:write("='" .. lib_name .. "'\n")
      flags = flags .. " -l" .. lib_name
   end
end

io.open("library_flags", "w"):write(flags)
