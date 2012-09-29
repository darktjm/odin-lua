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

rex = require 'rex_posix'

ODIN_FILE, ODIN_dir,
ODIN_home, ODIN_incsp, ODIN_ignore = unpack(arg)

odin_log('scan_for_includes ' .. basename(ODIN_FILE))

incsp=ODIN_home
if ODIN_incsp ~= "" then incsp=incsp .. ' ' .. wholefile(ODIN_incsp) end
incsp=incsp .. ' ' .. getenv("ODIN_CC_I")
for header in incsp:gmatch('%S+') do
   if not apr.filepath_root(header, 'native') then
      odin_error("Search path entry must be absolute: " .. header)
   end
end

local_i, global_i = "", ""

-- precompile ignore patterns before loop
-- refl = rex.flags()
-- nosub = refl['NOSUB'] + refl['EXTENDED']
-- rex returns screwy results with NOSUB, so just drop it
nosub = nil

ignore_re = nil
if ODIN_ignore ~= "" then
   re = ""
   for l in io.lines(ODIN_ignore) do
      ok, msg = pcall(rex.new, l, nosub)
      if not ok then
	 odin_error("Error in ignore pattern '" .. l .. "': " .. msg, 0)
      end
      if re == "" then re = l else re = re .. "|" .. l end
   end
   ODIN_ignore = re
   ok, msg = pcall(rex.new, re, nosub)
   if ok then
      ignore_re = msg
   else
      odin_error("Error in ignore pattern '" .. re .. "': " .. msg, 0)
   end
end

ODIN_IGNORE = getenv("ODIN_IGNORE")
if ODIN_IGNORE ~= "" then
   ok, msg = pcall(rex.new, ODIN_IGNORE, nosub)
   if not ok then
      odin_error("Error in ignore pattern '" .. ODIN_IGNORE .. "': " .. msg, 0)
   elseif ignore_re then
      ODIN_ignore = ODIN_ignore .. '|' .. ODIN_IGNORE
      ok, msg = pcall(rex.new, ODIN_ignore, nosub)
      if ok then
	 ignore_re = msg
      else
	 odin_error("Error in ignore pattern '" .. ODIN_ignore .. "': " .. msg, 0)
      end
   else
      ignore_re = msg
   end
end

-- may as well use rex_posix for the main search as well
include_re = rex.new('^[ 	]*#[ 	]*include[^"]*"([^"]*)"|' ..
		     '^[ 	]*#[ 	]*include[^<]*<([^>]*)>')

-- and to pre-split the dirs var
dirs = {}
for d in rex.split(incsp, '[ \t\n]') do table.insert(dirs, d) end

vd = io.open("c_inc.view_desc", "w")

for l in io.lines(ODIN_FILE) do
   s, e, m = include_re:tfind(l)
   if s then
      name = nil
      kind = nil
      for i, v in ipairs(m) do if v then name, kind = v, i end end
      if apr.filepath_root(name, 'native') then
	 if not ignore_re or not ignore_re:exec(name) then
	    vd:write("'" .. name .. "'\n=''\n")
	 end
      else
	 didone = nil
	 if kind == 1 then -- local
	    aname = pathcat(ODIN_dir, name)
	    if not ignore_re or not ignore_re:exec(aname) then
	       vd:write("'" .. aname .. "'\n")
	       didone = 1
	    end
	 end
	 for i, header in ipairs(dirs) do
	    aname = pathcat(header, name)
	    if not ignore_re or not ignore_re:exec(aname) then
	       vd:write("'" .. aname .. "'\n")
	       didone = 1
	    end
	 end
	 if didone then vd:write("=''\n") end
      end
   end
end
vd:close()
