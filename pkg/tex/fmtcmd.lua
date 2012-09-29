#!/usr/bin/env lua

-- EXEC (fmtcmd.lua) (:rootFileName +texsppt=(:texsp) :texfiles :ls) (+tex) (+latex)
--      (+usepdf)
--    => (:fmtcmd);

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

ODIN_rootFile, ODIN_tex, ODIN_latex,
ODIN_pdf = unpack(arg)

rootFile = wholefile(ODIN_rootFile)
cmd = 'tex'
if rootFile == '' then
   odin_error('The specified root file is not on the search path')
elseif is_empty(rootFile) then
   odin_error('The root file is empty')

elseif ODIN_latex ~= '' then
   cmd = 'latex'
elseif ODIN_tex == '' then
   for l in io.lines(rootFile) do
      if l:find('^\\documentstyle') or l:find('^\\documentclass') then
	 cmd = 'latex'
	 break
      end
   end
end

if ODIN_pdf ~= '' then
   cmd = 'pdf' .. cmd
end

append_line('fmtcmd', cmd)
