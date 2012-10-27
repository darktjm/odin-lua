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

ODIN_view, ODIN_viewsp = unpack(arg)

odin_log('scanning_for_view ' .. basename(ODIN_view))

viewsp = ""
if ODIN_viewsp ~= "" then viewsp = wholefile(ODIN_viewsp) end

vss = io.open('view.source.spec', 'w')
do_rcs = getenv("ODIN_VC_RCS") == 'yes'
do_sccs = getenv("ODIN_VC_SCCS") == 'yes'
fpcat = pathcat
for name in io.lines(ODIN_view) do
   file = basename(name)
   -- lua_apr has no way to get at raw dir part (filepath_parent tacks on cwd)
   dir = name:sub(1, -#file - 1)
   vss:write(name .. '\n')
   if do_rcs then
      vss:write(fpcat(fpcat(dir, 'RCS'), file .. ',v') .. '\n')
      vss:write(fpcat(dir, file .. ',v') .. '\n')
   end
   if do_sccs then
      vss:write(fpcat(fpcat(dir, 'SCCS'), 's.' .. file) .. '\n')
      vss:write(fpcat(dir, 's.' .. file) .. '\n')
   end
   if not glib.path_is_absolute(name) then
      for base in viewsp:gmatch('%S+') do
	 vss:write(fpcat(base, name) .. '\n')
	 if do_rcs then
	    vss:write(fpcat(fpcat(fpcat(base, dir), 'RCS'), file .. ',v') .. '\n')
	    vss:write(fpcat(fpcat(base, dir), file .. ',v') .. '\n')
	 end
	 if do_sccs then
	    vss:write(fpcat(fpcat(fpcat(base, dir), 'SCCS'), 's.' .. file) .. '\n')
	    vss:write(fpcat(fpcat(base, dir), 's.' .. file) .. '\n')
	 end
      end
   end
   vss:write("='" .. name .. "'\n")
end
vss:close()
