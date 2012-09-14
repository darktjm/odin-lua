-- APR provides portable setenv, basename, and others
apr = require 'apr'

-- shell-style getenv: undef returns empty string
function getenv(v)
   return apr.env_get(v) or ''
end

-- shell-style backtick-cat-as-args: get words from file
function words(fn)
   f = io.open(fn)
   if not f then return nil end
   s = f:read('*a')
   io.close(f)
   return string.gmatch(s, '%S+')
end

-- shell-style backtick-cat-as args for making cmdline
-- grab whole file, and collapse intervening spaces
function wholefile(fn)
   f = io.open(fn)
   if not f then return '' end
   return string.gsub(string.gsub(f:read('*a'), '%s+', ' '), ' $', '')
end

-- running a command portably takes more effort
-- than just building a string and passing it to execute
-- cmd is parsed like shell, and moreargs[array] are appended
-- moreargs also sets options:
--  .ignret = true -> ignore return code (assume success)
--  .chdir = <dir> -> change to <dir> before exec
--  .stdout = true -> capture stdout in MESSAGES
function runcmd(cmd, moreargs)
   args = apr.tokenize_to_argv(cmd)
   for i,v in ipairs(moreargs) do table.insert(args, v) end
   cmd = table.remove(args, 1)
   proc = apr.proc_create(cmd)
   proc:cmdtype_set('program/env/path')
   if moreargs.chdir then
      proc:dir_set(moreargs.chdir)
   end
   wf = apr.file_open('WARNINGS', 'a')
   ret, err = proc:err_set(wf)
   if ret and moreargs.stdout then
      of = apr.file_open('MESSAGES', 'a')
      ret, err = proc:out_set(of)
   end
   if ret then ret, err, ty = proc:exec(args) end
   wf:close()
   if moreargs.stdout then of:close() end
   if ret then
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
      if moreargs.stdout then
         io.open('MESSAGES', 'a'):write(io.open('WARNINGS'):read('*a'))
	 apr.file_rename('MESSAGES', 'WARNINGS')
      end
      apr.file_rename('WARNINGS', 'ERRORS')
      if err then
	 io.open('ERRORS', 'a'):write(err .. '\n')
      end
      io.open('ERRORS', 'a'):write(cmd .. ' failed\n')
   end
   return ret
end
