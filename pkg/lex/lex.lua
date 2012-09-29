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

ODIN_l, ODIN_yaccid = unpack(arg)

odin_log("lex " .. basename(ODIN_l))

runcmd("lex", {ODIN_l})

if ODIN_yaccid == "" then
   ODIN_yaccid = nil
else
   yy = ODIN_yaccid
   YY = ODIN_yaccid:upper()
end

if is_file("lex.yy.c") then
   c = io.open("c", "w")
   for l in io.lines("lex.yy.c") do
      l = l:gsub('"stdio%.h"', '<stdio.h>')
      -- note: these obsolete greps were commented out in sh version as well
      -- if not l:find("^# line ") and
      --    not l:find("yypvt") and
      --    not l:find("yyerrlab") then
	 if ODIN_yaccid then
	    l = l:gsub("yy", yy)
	    l = l:gsub("YY", YY)
	 end
	 c:write(l .. "\n")
      -- end
   end
   c:close()
end   
