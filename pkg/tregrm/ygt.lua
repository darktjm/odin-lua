#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_ygi, ODIN_yaccid, ODIN_tregrm = unpack(arg)

odin_log(apr.filepath_name(ODIN_tregrm) .. ' ' .. apr.filepath_name(ODIN_ygi))

runcmd(ODIN_tregrm, { stdin = ODIN_ygi, stdout = 'ygi.log' })

if ODIN_yaccid == "" then
   ODIN_yaccid = nil
else
   yy = ODIN_yaccid
   YY = string.upper(ODIN_yaccid)
end

if apr.stat("LEX_TAB", 'type') == 'file' then
   if ODIN_yaccid then
      th = io.open("tok.h", 'w')
      for l in io.lines("LEX_TAB") do
	 l = string.gsub(l, "yy", yy)
	 l = string.gsub(l, "YY", YY)
	 th:write(l .. '\n')
      end
      th:close()
   else
      apr.file_rename("LEX_TAB", "tok.h")
   end
end
if apr.stat("GRM_TAB", 'type') == 'file' then
   apr.file_rename("GRM_TAB", 'y')
end
if apr.stat("NOD_TAB", 'type') == 'file' then
   if ODIN_yaccid then
      nh = io.open("nod.h", 'w')
      for l in io.lines("NOD_TAB") do
	 l = string.gsub(l, "yy", yy)
	 l = string.gsub(l, "YY", YY)
	 nh:write(l .. '\n')
      end
      nh:close()
   else
      apr.file_rename("NOD_TAB", "nod.h")
   end
end
