-- APR provides portable setenv, basename, and others
apr = require 'apr'

-- shell-style getenv: undef returns empty string
function getenv(v)
   return apr.env_get(v) or ''
end

-- shell-style backtick-cat-as-args: get words from file
function words(fn)
   local f = io.open(fn)
   if not f then return nil end
   local s = f:read('*a')
   io.close(f)
   return string.gmatch(s, '%S+')
end

-- shell-style backtick-cat-as args for making cmdline
-- grab whole file, and collapse intervening spaces
function wholefile(fn)
   local f = io.open(fn)
   if not f then return '' end
   return string.gsub(string.gsub(f:read('*a'), '%s+', ' '), ' $', '')
end

-- append a message plus a newline to a file
function append_line(f, msg)
    local fd = io.open(f, "a")
    fd:write(msg .. "\n")
    fd:close()
end

-- simple echo to error file (and maybe exit)
function odin_error(msg, ret)
    append_line("ERRORS", msg)
    if ret ~= nil then os.exit(ret) end
end

-- running a command portably takes more effort
-- than just building a string and passing it to execute
-- cmd is parsed like shell, and moreargs[array] are appended
-- moreargs also sets options:
--  .ignret = true -> ignore return code (assume success)
--  .chdir = <dir> -> change to <dir> before exec
--  .stdout = <name> -> capture stdout in file <name>
--  .stdin = <name> -> input from file <name>
function runcmd(cmd, moreargs)
   local args = apr.tokenize_to_argv(cmd)
   local i, v
   for i,v in ipairs(moreargs) do table.insert(args, v) end
   local cmd = table.remove(args, 1)
   local proc = apr.proc_create(cmd)
   proc:cmdtype_set('program/env/path')
   if moreargs.chdir then
      proc:dir_set(moreargs.chdir)
   end
   local wf = apr.file_open('WARNINGS', 'a')
   local ret, err = proc:err_set(wf)
   local of, inf
   if ret and moreargs.stdout then
      of = apr.file_open(moreargs.stdout, 'a')
      ret, err = proc:out_set(of)
   end
   if ret and moreargs.stdin then
      inf = apr.file_open(moreargs.stdin)
      ret, err = proc:in_set(inf)
   end
   local ty
   if ret then ret, err, ty = proc:exec(args) end
   wf:close()
   if moreargs.stdout then of:close() end
   if moreargs.stdin then inf:close() end
   if ret then
      local why, val
      repeat
         ret, why, val, err = proc:wait(1)
      until ret
      if why == 'signal' then val = -1 end
      if not ret and not err then
	 err = '*** Aborted'
      end
      if ret and val ~= 0 and not moreargs.ignret then ret = nil end
   end
   if not ret then
      -- on error, merge all captured output into single 'ERRORS' file
      if moreargs.stdout then
	 of = io.open(moreargs.stdout, 'a')
	 wf = io.open("WARNINGS")
	 of:write(wf:read('*a'))
	 of:close()
	 wf:close()
	 apr.file_rename(moreargs.stdout, 'WARNINGS')
      end
      apr.file_rename('WARNINGS', 'ERRORS')
      if err then
	 odin_error(err)
      end
      odin_error(cmd .. ' failed')
   end
   return ret
end

-- simplify logging messages
ODINVERBOSE = getenv("ODINVERBOSE") ~= ""
ODINRBSHOST = getenv("ODINRBSHOST")
function odin_log(msg)
    if ODINVERBOSE then
	io.write(ODINRBSHOST)
	print(msg)
    end
end

-- for in-line +cmd strings
-- e.g. +cmd='... trim([['(%files)']]) .... '
function trim(s)
    return string.gsub(string.gsub(s, "^[\t\n ]*", ""), "[ \t\n]*$", "")
end

-- 5.2 compatibility
if unpack == nil then unpack = table.unpack end
