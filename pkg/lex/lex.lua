#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_l, ODIN_yaccid = unpack(arg)

odin_log("lex " .. basename(ODIN_l))

runcmd("lex", {ODIN_l})

if ODIN_yaccid == "" then
   ODIN_yaccid = nil
else
   yy = ODIN_yaccid
   YY = string.upper(ODIN_yaccid)
end

if is_file("lex.yy.c") then
   c = io.open("c", "w")
   for l in io.lines("lex.yy.c") do
      l = string.gsub(l, '"stdio%.h"', '<stdio.h>')
      -- note: these obsolete greps were commented out in sh version as well
      -- if not string.find(l, "^# line ") and
      --    not string.find(l, "yypvt") and
      --    not string.find(l, "yyerrlab") then
	 if ODIN_yaccid then
	    l = string.gsub(l, "yy", yy)
	    l = string.gsub(l, "YY", YY)
	 end
	 c:write(l .. "\n")
      -- end
   end
   c:close()
end   
