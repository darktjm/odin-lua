#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_exe, ODIN_pkgdir, ODIN_pkglst = unpack(arg)

if ODIN_pkglst == "" then
   odin_error("+pkg must be set", 0)
end

if ODIN_pkgdir == "" then
   odin_error("+pkg_dir must be set", 0)
end

odin_log(apr.filepath_name(ODIN_exe) .. ' ' .. ODIN_pkgdir .. ' ' .. ODIN_pkglst .. ' 0')

runcmd(ODIN_exe, { ODIN_pkglst, ODIN_pkgdir, '0', stdout = 'dg.log' })

if io.open("DG.c") then apr.file_rename("DG.c", "c") end
if io.open("DG") then apr.file_rename("DG", "dg.tab") end
if io.open("ENV") then apr.file_rename("ENV", "dg.env") end
