#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_y, ODIN_yaccid, ODIN_conflictok,
ODIN_sys5, ODIN_gnu, ODIN_yacc, ODIN_flags = unpack(arg)

compiler = getenv("ODIN_YACC")
if ODIN_gnu ~= "" then compiler = 'bison' end
if ODIN_yacc ~= "" then compiler = ODIN_yacc end

flags=getenv("ODIN_YACC_FLAGS")
if ODIN_flags ~= "" then flags = wholefile(ODIN_flags) .. ' ' .. flags end
if ODIN_gnu ~= "" then flags = flags .. ' -y' end

odin_log(compiler .. ' ' .. flags .. ' ' .. apr.filepath_name(ODIN_y))

if runcmd(compiler .. ' ' .. flags, { ODIN_y }) and ODIN_conflictok == "" then
   for l in io.lines('WARNINGS') do
      if string.find(l, 'conflicts') then
	 odin_error(l)
      end
   end
end

if ODIN_yaccid == "" then
   ODIN_yaccid = nil
else
   yy = ODIN_yaccid
   YY = string.upper(ODIN_yaccid)
end

if apr.stat("y.tab.c", 'type') == 'file' then
   c = io.open("c", "w")
   for l in io.lines("y.tab.c") do
      if ODIN_sys5 ~= "" or getenv("ODIN_SYS5") ~= "" then
	 l = string.gsub(l, '^(extern )char( %*malloc\(\))', '%1void%2')
      end
      -- note: these obsolete greps were commented out in sh version as well
      -- if not string.find(l, "yypvt") and
      --    not string.find(l, "yyerrlab") then
	 if ODIN_yaccid then
	    l = string.gsub(l, "yy", yy)
	    l = string.gsub(l, "YY", YY)
	 end
	 c:write(l .. "\n")
      -- end
   end
end
if apr.stat("y.tab.h", 'type') == 'file' then
   apr.file_rename("y.tab.h", "h")
end
if apr.stat("y.output", 'type') == 'file' then
   apr.file_rename("y.output", "yacc.log")
end
