#!/usr/bin/env lua

ODIN_exe, ODIN_pkgdir, ODIN_pkglst = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

if ODIN_pkglst == "" then
   io.open("ERRORS", "w"):write("+pkg must be set\n")
   os.exit(0)
end

if ODIN_pkgdir == "" then
   io.open("ERRORS", "w"):write("+pkg_dir must be set\n")
   os.exit(0)
end

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. apr.filepath_name(ODIN_exe) .. ' ' .. ODIN_pkgdir .. ' ' .. ODIN_pkglst .. ' 0')
end

runcmd(ODIN_exe, { ODIN_pkglst, ODIN_pkgdir, '0', stdout = true })

if io.open("DG.c") then apr.file_rename("DG.c", "c") end
if io.open("DG") then apr.file_rename("DG", "dg.tab") end
if io.open("ENV") then apr.file_rename("ENV", "dg.env") end
apr.file_rename("MESSAGES", "dg.log")
