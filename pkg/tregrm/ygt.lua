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

ODIN_ygi, ODIN_yaccid, ODIN_tregrm = unpack(arg)

odin_log(basename(ODIN_tregrm) .. ' ' .. basename(ODIN_ygi))

runcmd(ODIN_tregrm, { stdin = ODIN_ygi, stdout = 'ygi.log' })

if ODIN_yaccid == "" then
   ODIN_yaccid = nil
else
   yy = ODIN_yaccid
   YY = ODIN_yaccid:upper()
end

if is_file("LEX_TAB") then
   if ODIN_yaccid then
      th = io.open("tok.h", 'w')
      for l in io.lines("LEX_TAB") do
	 l = l:gsub("yy", yy)
	 l = l:gsub("YY", YY)
	 th:write(l .. '\n')
      end
      th:close()
   else
      mv("LEX_TAB", "tok.h")
   end
end
if is_file("GRM_TAB") then
   mv("GRM_TAB", 'y')
end
if is_file("NOD_TAB") then
   if ODIN_yaccid then
      nh = io.open("nod.h", 'w')
      for l in io.lines("NOD_TAB") do
	 l = l:gsub("yy", yy)
	 l = l:gsub("YY", YY)
	 nh:write(l .. '\n')
      end
      nh:close()
   else
      mv("NOD_TAB", "nod.h")
   end
end
