#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_lnlist, ODIN_loglist, ODIN_lnstubs,
ODIN_ln, ODIN_flags, ODIN_ignore,
ODIN_gignore = unpack(arg)

lintflags = ''
if ODIN_flags ~= '' then lintflags = ' ' .. wholefile(ODIN_flags) end
lintflags = lintflags .. ' ' .. getenv('ODIN_LINT_FLAGS')

inputs = wholefile(ODIN_inlist)
if ODIN_lnstubs ~= '' then inputs = inputs .. ' ' .. wholefile(ODIN_lnstubs) end
if ODIN_ln ~= "" then inputs = inputs .. wholefile(ODIN_ln) end

touch('lint')
for f in words(ODIN_loglist) do
   apr.file_append(f, 'lint')
end

odin_log('lint ' .. lintflags .. ' ' .. inputs)

-- parse ignore patterns before running lint so errors cause early abort
re = nil
if ODIN_ignore ~= '' or ODIN_gignore ~= '' then
   rex = require 'rex_posix'
   re = ''
   if ODIN_ignore ~= '' then
      for l in io.lines(ODIN_igore) do
	 ok, msg = pcall(rex.new, l)
	 if not ok then
	    odin_error("Error in ignore pattern '" .. l .. "': " .. msg, 0)
	 end
	 re = re .. '|' .. l
      end
   end
   if ODIN_gignore ~= '' then
      for l in io.lines(ODIN_gignore) do
	 ok, msg = pcall(rex.new, l)
	 if not ok then
	    odin_error("Error in gignore pattern '" .. l .. "': " .. msg, 0)
	 end
	 re = re .. '|' .. l
      end
   end
   re = rex.new(string.sub(re, 2))
end

runcmd('lint ' .. lintflags .. ' ' .. inputs, { stdout = 'lint' })

if re then
   tmp = io.open('tmp', 'w')
   for l in io.lines('lint') do
      if not re:exec(l) then tmp:write(l .. '\n') end
   end
   mv('tmp', 'lint')
end
