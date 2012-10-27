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

ODIN_lib, ODIN_libsp = unpack(arg)

lib_names = ''
if ODIN_lib ~= "" then lib_names=wholefile(ODIN_lib) end
flags = '' bases = ''
if ODIN_libsp ~= "" then
   flags=(' ' .. wholefile(ODIN_libsp)):gsub(' ', ' -L')
   bases=bases .. wholefile(ODIN_libsp)
end

odin_log('scan_for_libraries (' .. lib_names .. ") in (" .. bases .. ")")

vd = io.open("libraries.view_desc", "w")
    
for lib_name in lib_names:gsub("-L ", "-L"):gmatch("%S+") do
   if lib_name:sub(1, 1) == '/' then
      vd:write("'" .. lib_name .. "'\n" ..
               "=''\n")
      flags = flags .. ' ' .. lib_name
   elseif lib_name:sub(1, 2) == '-L' then
      bases = bases .. ' ' .. lib_name:sub(3)
      flags = flags .. ' ' .. lib_name
   elseif lib_name:sub(1, 1) == '-' then
      flags = flags .. ' ' .. lib_name
   else
      for lib_base in (bases .. ' ' .. getenv("ODIN_LIB_SP")):gmatch("%S+") do
	 for i, ext in ipairs{'so','sl','a'} do
	    lib = lib_base .. '/lib' .. lib_name .. '.' .. ext
	    vd:write("'" .. lib .. "'\n")
	    for file in dir_glob(lib_base, basename(lib) .. '.[0-9]*.[0-9]*') do
	       vd:write("'" .. pathcat(lib_base, file) .. "'\n")
	    end
	 end
      end
      vd:write("='" .. lib_name .. "'\n")
      flags = flags .. " -l" .. lib_name
   end
end
vd:close()

append_line('library_flags', flags)
