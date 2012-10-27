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

ODIN_lnlist, ODIN_loglist, ODIN_lnstubs,
ODIN_ln, ODIN_flags, ODIN_ignore,
ODIN_gignore = unpack(arg)

lintflags = ''
if ODIN_flags ~= '' then lintflags = ' ' .. wholefile(ODIN_flags) end
lintflags = lintflags .. ' ' .. getenv('ODIN_LINT_FLAGS')

inputs = wholefile(ODIN_inlist)
if ODIN_lnstubs ~= '' then inputs = inputs .. ' ' .. wholefile(ODIN_lnstubs) end
if ODIN_ln ~= "" then inputs = inputs .. wholefile(ODIN_ln) end

lint = io.open('lint', 'w')
for f in words(ODIN_loglist) do
   cp(f, lint)
end
lint:close()

odin_log('lint ' .. lintflags .. ' ' .. inputs)

-- parse ignore patterns before running lint so errors cause early abort
re = nil
if ODIN_ignore ~= '' or ODIN_gignore ~= '' then
   re = ''
   if ODIN_ignore ~= '' then
      for l in io.lines(ODIN_igore) do
	 ok, msg = glib.regex_new(l)
	 if not ok then
	    odin_error("Error in ignore pattern '" .. l .. "': " .. msg, 0)
	 end
	 re = re .. '|' .. l
      end
   end
   if ODIN_gignore ~= '' then
      for l in io.lines(ODIN_gignore) do
	 ok, msg = glib.regex_new(l)
	 if not ok then
	    odin_error("Error in gignore pattern '" .. l .. "': " .. msg, 0)
	 end
	 re = re .. '|' .. l
      end
   end
   re = glib.regex_new(re:sub(2))
end

runcmd('lint ' .. lintflags .. ' ' .. inputs, { stdout = 'lint' })

if re then
   tmp = io.open('tmp', 'w')
   for l in io.lines('lint') do
      if not re:find(l) then tmp:write(l .. '\n') end
   end
   tmp:close()
   mv('tmp', 'lint')
end
