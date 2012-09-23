#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_roff, ODIN_dir, ODIN_mp = unpack(arg)

mflag = ''
if ODIN_mp ~= "" then mflag=' -m' .. wholefile(ODIN_mp) end

odin_log('nroff' .. mflag .. ' ' .. basename(ODIN_roff))

runcmd('nroff' .. mflag, {stdin = ODIN_roff, stdout = 'nroff', chdir = ODIN_dir})
