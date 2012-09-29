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

ODIN_y, ODIN_yaccid, ODIN_conflictok,
ODIN_sys5, ODIN_gnu, ODIN_yacc, ODIN_flags = unpack(arg)

compiler = getenv("ODIN_YACC")
if ODIN_gnu ~= "" then compiler = 'bison' end
if ODIN_yacc ~= "" then compiler = ODIN_yacc end

flags=getenv("ODIN_YACC_FLAGS")
if ODIN_flags ~= "" then flags = wholefile(ODIN_flags) .. ' ' .. flags end
if ODIN_gnu ~= "" then flags = flags .. ' -y' end

odin_log(compiler .. ' ' .. flags .. ' ' .. basename(ODIN_y))

if runcmd(compiler .. ' ' .. flags, { ODIN_y }) and ODIN_conflictok == "" and
   not is_empty('WARNINGS') then
   for l in io.lines('WARNINGS') do
      if l:find('conflicts') then
	 odin_error(l)
      end
   end
end

if ODIN_yaccid == "" then
   ODIN_yaccid = nil
else
   yy = ODIN_yaccid
   YY = ODIN_yaccid:upper()
end

if is_file("y.tab.c") then
   c = io.open("c", "w")
   for l in io.lines("y.tab.c") do
      if ODIN_sys5 ~= "" or getenv("ODIN_SYS5") ~= "" then
	 l = l:gsub('^(extern )char( %*malloc\(\))', '%1void%2')
      end
      -- note: these obsolete greps were commented out in sh version as well
      -- if not l:find("yypvt") and
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
if is_file("y.tab.h") then
   mv("y.tab.h", "h")
end
if is_file("y.output") then
   mv("y.output", "yacc.log")
end
