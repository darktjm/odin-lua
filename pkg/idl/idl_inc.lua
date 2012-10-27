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

ODIN_FILE, ODIN_dir, ODIN_home,
ODIN_incsp, ODIN_ignore = unpack(arg)

odin_log('scan_for_includes ' .. basename(ODIN_FILE))

incsp=ODIN_home
if ODIN_incsp ~= "" then incsp=incsp .. ' ' .. wholefile(ODIN_incsp) end
incsp=incsp .. ' ' .. getenv("ODIN_IDL_I")
for header in incsp:gmatch('%S+') do
   if not glib.path_is_absolute(header) then
      odin_error("Search path entry must be absolute: " .. header)
   end
end

local_i, global_i = "", ""

-- precompile ignore patterns before loop
ignore_re = nil
if ODIN_ignore ~= "" then
   re = ""
   for l in io.lines(ODIN_ignore) do
      ok, msg = glib.regex_new(l)
      if not ok then
	 odin_error("Error in ignore pattern '" .. l .. "': " .. msg, 0)
      end
      if re == "" then re = l else re = re .. "|" .. l end
   end
   ODIN_ignore = re
   ignore_re, msg = glib.regex_new(re)
   if not ignore_re then
      odin_error("Error in ignore pattern '" .. re .. "': " .. msg, 0)
   end
end

ODIN_IGNORE = getenv("ODIN_IGNORE")
if ODIN_IGNORE ~= "" then
   ok, msg = glib.regex_new(ODIN_IGNORE)
   if not ok then
      odin_error("Error in ignore pattern '" .. ODIN_IGNORE .. "': " .. msg, 0)
   elseif ignore_re then
      ODIN_ignore = ODIN_ignore .. '|' .. ODIN_IGNORE
      ignore_re, msg = pcall(glib.regex_new, ODIN_ignore)
      if not ignore_re then
	 odin_error("Error in ignore pattern '" .. ODIN_ignore .. "': " .. msg, 0)
      end
   else
       ignore_re = ok
   end
end

include_re = glib.regex_new('^[ \t]*#[ \t]*include[^"]*"([^"]*)"|' ..
		            '^[ \t]*#[ \t]*include[^<]*<([^>]*)>|' ..
		            'import[ \t]*"(.*)"[^"]*$')

dirs = {}
for i, d in ipairs(glib.regex_new('[ \t\n]'):split(incsp)) do
    table.insert(dirs, d)
end

vd = io.open("idl_inc.view_desc", "w")

for l in io.lines(ODIN_FILE) do
   s, e, m = include_re:tfind(l)
   if s then
      name = nil
      kind = nil
      for i, v in ipairs(m) do if v then name, kind = v, i end end
      if kind == 3 then
	 kind = 1
	 name = name:gsub('[ \t]*,[ \t]*"', ' ')
      end
      if glib.path_is_absolute(name) then
	 if not ignore_re or not ignore_re:find(name) then
	    vd:write("'" .. name .. "'\n=''\n")
	 end
      else
	 didone = nil
	 if kind == 1 then -- local
	    aname = pathcat(ODIN_dir, name)
	    if not ignore_re or not ignore_re:find(aname) then
	       vd:write("'" .. aname .. "'\n")
	       didone = 1
	    end
	 end
	 for i, header in ipairs(dirs) do
	    aname = pathcat(header, name)
	    if not ignore_re or not ignore_re:find(aname) then
	       vd:write("'" .. aname .. "'\n")
	       didone = 1
	    end
	 end
	 if didone then vd:write("=''\n") end
      end
   end
end
vd:close()
