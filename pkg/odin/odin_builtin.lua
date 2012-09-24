-- APR provides portable setenv, basename, and others
apr = require 'apr'

-- some convenient aliases for often-used functions

basename = apr.filepath_name
function pathcat(d, f)
    return apr.filepath_merge(d, f, 'native')
end
function getcwd()
    return apr.filepath_get(true)
end
chdir = apr.filepath_set
mkdir = apr.dir_make_recursive
mv = apr.file_rename
cp = apr.file_copy
rm = apr.file_remove

-- 5.2 compatibility
if unpack == nil then unpack = table.unpack end

-- common uses of stat
function exists(f)
    return apr.stat(f, 'type') ~= nil
end

function is_dir(f)
    return apr.stat(f, 'type') == 'directory'
end

function is_file(f)
    return apr.stat(f, 'type') == 'file'
end

function is_exec(f)
    local p = apr.stat(f, 'protection')
    if p == nil then return false end
    return string.sub(p, 3, 1) == 'x'
end

function is_empty(f)
   local t, s = apr.stat(f, 'type', 'size')
   return not t or (t == 'file' and s == 0)
end

-- touch the way it's supposed to be
function touch(f, ...)
    if not apr.stat(f, 'type') then
	local fd = io.open(f, 'w')
	if fd then fd:close() end
    end
    apr.file_mtime_set(f, ...)
end

-- shell-style getenv: undef returns empty string
function getenv(v)
   return apr.env_get(v) or ''
end

setenv = apr.env_set

-- for subsequent functions: return a file descriptor for file/name arg
function fopen(f, mode)
    if io.type(f) == 'file' then
	return f
    else
	return io.open(f, mode)
    end
end

-- close f if f was opened above
function fclose(f, n)
    if io.type(n) ~= 'file' and f ~= nil then
	f:close()
    end
end

-- recursive file comparison
function cmp(f1, f2)
    if is_dir(f1) then
	if not is_dir(f2) then return false end
	for p in apr.glob(pathcat(f1, '*')) do
	    if not cmp(p, pathcat(f2, basename(p))) then
		return false
	    end
	end
	return true
    end
    if not is_file(f1) or not is_file(f2) then
	return false
    end
    local f1d, f2d
    f1d = io.open(f1, 'rb')
    f2d = io.open(f2, 'rb')
    if not f1d or not f2d then return false end
    local f1b, f2b
    repeat
	f1b = f1d:read(16384)
	f2b = f2d:read(16384)
    until f1b ~= f2b or f1b == nil
    f1d:close()
    f2d:close()
    return f1b == f2b
end

-- recursive file copy (cp -RpL)
-- note that specials (sockets, pipes, etc.) can't possibly be copied portably
-- if plus_w is true, add user write permission
-- if d_to_d is true, assume src and dest are existing directories
-- returns true on success, or (nil, err) on error
function cp_RpL(src, dest, plus_w, d_to_d)
   local t, m
   local st, msg
   if not d_to_d then
      local t, m = apr.stat(src, 'type', 'protection')
      if plus_w then
	 m = string.sub(m, 1, 1) .. 'w' .. string.sub(m, 3)
      end
      if t == 'directory' then
	 st, msg = mkdir(dest, m)
	 if not st then
	    return nil, "Error writing " .. dest .. ": " .. msg
	 end
      elseif t == 'file' then
	 st, msg = cp(src, dest, m)
	 if not st then
	    return nil, "Error writing " .. dest .. ": " .. msg
	 end
	 return true
      else
	 return nil, 'Copy attempt on special ' .. src
      end
   end
   local dir = apr.dir_open(src)
   local p, n
   -- can't use 'type' here because soft links show up as link
   for p, n in dir:entries('path', 'name') do
      -- this dereferences any link (-L)
      t, m = apr.stat(p, 'type', 'protection')
      if plus_w then
	 m = string.sub(m, 1, 1) .. 'w' .. string.sub(m, 3)
      end
      local d = pathcat(dest, n)
      if t == 'directory' then
	 st, msg = mkdir(d, m)
	 if not st then
	    return nil, "Error writing " .. d .. ": " .. msg
	 end
	 st, msg = cp_RpL(p, d, plus_w, true)
	 if not st then return st, msg end
      elseif t == 'file' then
	 st, msg = cp(p, d, m)
	 if not st then
	    return nil, "Error writing " .. d .. ": " .. msg
	 end
      else
	 return nil, 'Copy attempt on special ' .. p
      end
   end
   return true
end

has_posix, posix = pcall(require, 'posix')
if not has_posix then posix = nil end

-- there is no portable ln -s, so try hard link then copy
function ln(s, d)
   if has_posix then
      return posix.link(s, d, true)
   end
   if not apr.file_link or not apr.file_link(s, d) then
      cp_RpL(s, d)
   end
end

-- shell-style backtick-cat-as-args: get words from file (iterator)
-- could probably be improved by not sucking entire file into memory
function words(fn)
   local f = fopen(fn)
   if not f then return nil end
   local s = f:read('*a')
   fclose(f, fn)
   return string.gmatch(s, '%S+')
end

-- shell-style backtick-cat-as args for making cmdline
-- grab whole file, and collapse intervening spaces
function wholefile(fn)
   local f = fopen(fn)
   if not f then return '' end
   local s = string.gsub(string.gsub(f:read('*a'), '%s+', ' '), ' $', '')
   fclose(f, fn)
   return s
end

-- append a message plus a newline to a file
function append_line(f, msg)
    local fd = fopen(f, 'a')
    if fd ~= nil then
	fd:write(msg .. '\n')
    end
    fclose(fd, f)
end

-- simple echo to error file (and maybe exit)
function odin_error(msg, ret)
    append_line("ERRORS", msg)
    if ret ~= nil then os.exit(ret) end
end

-- print a file to standard output without reading entire file into mem
function cat(f)
    local fd = fopen(f)
    if fd ~= nil then
	local s = fd:read(4096)
	while s ~= nil do
	    io.write(s)
	    s = fd:read(4096)
	end
    end
end

-- running a command portably takes more effort
-- than just building a string and passing it to execute
-- plus there are common post-processing steps to do:
--   write stderr to WARNINGS if return code == 0
--   write stdout/stderr to ERRORS instead if return code ~= 0 (unless .ignret)
-- cmd is parsed like shell, and moreargs[array] are appended
-- moreargs also sets options:
--  .ignret = true -> ignore return code (assume success)
--  .chdir = <dir> -> change to <dir> before exec
--  .stdout = <name> -> capture stdout in file <name>
--  .stdin = <name> -> input from file <name>
function runcmd(cmd, moreargs)
   if moreargs == nil then moreargs = {} end
   local args = apr.tokenize_to_argv(cmd)
   local i, v
   for i,v in ipairs(moreargs) do table.insert(args, v) end
   local cmd = table.remove(args, 1)
   local proc = apr.proc_create(cmd)
   proc:cmdtype_set('program/env/path')
   if moreargs.chdir then
      proc:dir_set(moreargs.chdir)
   end
   local ret, err
   local of, inf, wf
   ret = apr.file_open('WARNINGS', 'a')
   if ret then
      wf = ret
      ret, err = proc:err_set(wf)
   end
   if ret then
      ret = apr.file_open("MESSAGES", 'a')
   end
   if ret then
      of = ret
      ret, err = proc:out_set(of)
   end
   if ret and moreargs.stdin then
      inf = apr.file_open(moreargs.stdin)
   else
      -- this seems to be the only way to close input
      -- other than detach, which doesn't work quite right
      ret, err = apr.pipe_create()
      if ret then
	 inf = ret
	 err:close()
      end
   end
   if ret then
      ret, err = proc:in_set(inf)
   end
   local ty
   if ret then ret, err, ty = proc:exec(args) end
   if wf then wf:close() end
   if of then of:close() end
   if inf then inf:close() end
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
      apr.file_append('WARNINGS', 'MESSAGES')
      rm('WARNINGS')
      mv('MESSAGES', 'ERRORS')
      if err then
	 odin_error(err)
      end
      odin_error(cmd .. ' failed')
   elseif moreargs.stdout then
      if moreargs.stdout ~= 'MESSAGES' then
	 apr.file_append('MESSAGES', moreargs.stdout)
	 rm('MESSAGES')
      end
   else
      cat('MESSAGES')
      rm('MESSAGES')
   end
   -- reduce clutter a tiny bit
   if is_empty('WARNINGS') then
      rm('WARNINGS')
   end
   return ret
end

-- simplify logging messages
ODINVERBOSE = getenv("ODINVERBOSE") ~= ""
ODINRBSHOST = getenv("ODINRBSHOST")
function odin_log(msg)
    if ODINVERBOSE then
	io.write(ODINRBSHOST .. msg .. '\n')
	io.flush()
    end
end

-- for in-line +cmd strings
-- e.g. +cmd='... trim([['(%files)']]) .... '
function trim(s)
    return string.gsub(string.gsub(s, "^[\t\n ]*", ""), "[ \t\n]*$", "")
end
