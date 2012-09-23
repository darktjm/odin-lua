#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_FILE, ODIN_cmd, ODIN_cmdfile = unpack(arg)

cmd = "cat(io.input())"
if ODIN_cmd ~= "" then
   cmd = wholefile(ODIN_cmd)
end
if ODIN_cmdfile ~= "" then
   fcmd = " dofile(" .. wholefile(ODIN_cmdfile) .. ")"
   if ODIN_cmd ~= "" then
      cmd = cmd .. fcmd
   else
      cmd = fcmd
   end
end

odin_log("lua!" .. cmd)

io.input(ODIN_FILE)
io.output('lua_stdout')
mkdir('lua_output')
chdir('lua_output')

if ODIN_cmd ~= "" then
   st, msg = pcall(dofile, ODIN_cmd)
   if not st then
      odin_error(msg, 0)
   end
end
if ODIN_cmdfile ~= "" then
   for f in io.lines(ODIN_cmdfile) do
      st, msg = pcall(dofile, f)
      if not st then
	 odin_error(msg, 0)
      end
   end
end
if ODIN_cmd == "" and ODIN_cmdfile == "" then
   st, msg = pcall(cat, io.input())
   if not st then
      odin_error(msg, 0)
   end
end
