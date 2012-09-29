#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$", '') -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua"))
end

ODIN_exe, ODIN_pkgdir, ODIN_pkglst = unpack(arg)

if ODIN_pkglst == "" then
   odin_error("+pkg must be set", 0)
end

if ODIN_pkgdir == "" then
   odin_error("+pkg_dir must be set", 0)
end

odin_log(basename(ODIN_exe) .. ' ' .. ODIN_pkgdir .. ' ' .. ODIN_pkglst .. ' 0')

runcmd(ODIN_exe, { ODIN_pkglst, ODIN_pkgdir, '0', stdout = 'dg.log' })

if is_file("DG.c") then mv("DG.c", "c") end
if is_file("DG") then mv("DG", "dg.tab") end
if is_file("ENV") then mv("ENV", "dg.env") end
