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

ODIN_c, ODIN_dir, ODIN_incsp,
ODIN_define, ODIN_flags, ODIN_ignore = unpack(arg)

flags = getenv("ODIN_LINT_PASS1_FLAG")
if ODIN_define ~= "" then
   for def in words(ODIN_define) do
      flags = flags .. " -D" .. def
   end
end
flags = flags .. " -I" .. ODIN_dir
if ODIN_incsp ~= "" then
   for sp in words(ODIN_incsp) do
      flags = flags .. " -I" .. sp
   end
end
if ODIN_flags ~= "" then flags = flags .. " " .. wholefile(ODIN_flags) end
flags=flags .. " " .. getenv("ODIN_LINT_FLAGS")

odin_log('lint ' .. flags .. basename(ODIN_c))

runcmd('lint ' .. flags, {ODIN_c, stdout = 'lint1.log'})

-- parse ignore patterns before running lint so errors cause early abort
re = nil
rex = require 'rex_posix'
re = 'possible pointer alignment'
if ODIN_ignore ~= '' then
   if ODIN_ignore ~= '' then
      for l in io.lines(ODIN_igore) do
	 ok, msg = pcall(rex.new, l)
	 if not ok then
	    odin_error("Error in ignore pattern '" .. l .. "': " .. msg, 0)
	 end
	 re = re .. '|' .. l
      end
   end
end
re = rex.new(re)

input = basename(ODIN_c, true)
if is_file(input .. '.ln') then mv(input .. '.ln', 'ln') end

tmp = io.open('tmp', 'w')
for l in lines('lint1.log') do
   if not re:match(l) then
      tmp:write(l .. '\n')
   end
end
mv('tmp', 'lint1.log')
