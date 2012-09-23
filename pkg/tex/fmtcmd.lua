#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

-- EXEC (fmtcmd.lua) (:rootFileName +texsppt=(:texsp) :texfiles :ls) (+tex) (+latex)
--      (+usepdf)
--    => (:fmtcmd);

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
      if string.find(l, '^\\documentstyle') or
         string.find(l, '^\\documentclass') then
	 cmd = 'latex'
	 break
      end
   end
end

if ODIN_pdf ~= '' then
   cmd = 'pdf' .. cmd
end

append_line('fmtcmd', cmd)
