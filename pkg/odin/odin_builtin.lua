-- GLib provides portable setenv, basename, and others
glib = require 'glib'

-- some convenient aliases for often-used functions

pathcat = glib.build_filename
getcwd = glib.get_current_dir
chdir = glib.chdir
mkdir = glib.mkdir_with_parents
mv = glib.rename
exists = glib.exists
is_dir = glib.is_dir
is_file = glib.is_file
is_exec = glib.is_exec
is_link = glib.is_symlink

-- 5.2 compatibility
if unpack == nil then unpack = table.unpack end

--- inverse of pathcat
dir_separator_charclass = '[' .. glib.regex_escape_string(glib.dir_separator) .. ']')
dir_separator_regex = glib.regex_new(dir_separator_charclass)
dir_separator_all_regex = glib.regex_new('^' .. dir_separator_charclass .. '+$')
--dir_separator_end_regex = glib.regex_new('^((?!' ..  dir_separator_charclass .. ').*?)' ..
--					dir_separator_charclass .. '+$')
function split_filename(f)
   local res = {}
   local r
   r, f = glib.path_split_root(f)
   if r and dir_separator_all_regex:match(r) then
      r = glib.dir_separator:sub(1, 1)
   end
   -- not sure if trimming trailing dir_separators from r is safe, so don't.
   -- if r then dir_separator_end_regex:gsub(r, '\\1') end
   local i, e, laste
   for i, e in ipairs(dir_separator_regex:split(f)) do
      if e ~= '' then table.insert(res, e) end
      laste = e 
   end
   if laste == '' then table.insert(res, '') end
   return r, res
end

-- split/merge path lists
function build_path(a1, ...)
   local i, v
   local ret = ''
   local p
   if type(a1) == 'table' then
      p = a1
   else
      p = {a1, ...}
   end
   local sep = glib.searchpath_separator
   for i, v in ipairs(p) do
      if i > 1 then ret = ret .. sep end
      ret = ret .. v
   end
   return ret
end

function split_path(p)
    return glib.regex_new(glib.regex_escape_string(glib.searchpath_separator)):split(p)
end

function glob_to_regex(g)
   local escall = glib.regex_escape_string(g)
   local s, e, pos, re, brnest, brloc, skip, com, incc
   local dirsep = glib.regex_escape_string(glib.dir_separator)
   pos = 1
   re = ''
   brnest = 0
   com = {}
   brloc = {}
   for s, e, what in glib.regex_new('\\\\(.)'):gfind(escall) do
      if s > pos then
	 local unesc = escall:sub(pos, s - 1)
	 if brnest > 0 then
	    local anycom
	    if skip then
	       local comsub
	       comsub, anycom = unesc:sub(2):gsub(',', '|')
	       unesc = unesc:sub(1, 1) .. comsub
	    else
	       unesc, anycom = unesc:gsub(',', '|')
	    end
	    com[brnest] = com[brnest] + anycom
	 end
	 re = re .. unesc
	 skip = false
      end
      pos = e + 1
      if skip then
	 skip = false
      elseif not incc and what == '*' then
	 re = re .. '[^' .. dirsep .. ']*'
      elseif not incc and what == '?' then
	 re = re .. '[^' .. dirsep .. ']'
      elseif not incc and what == '[' then
	 re = re .. '(?![' .. dirsep .. '])['
	 incc = true
      elseif what == ']' then
	 re = re .. what
	 incc = false
      elseif not incc and what == '{' then
	 brnest = brnest + 1
	 com[brnest] = 0
	 brloc[brnest] = #re
	 re = re .. '(?:'
      elseif not incc and brnest > 0 and what == '}' then
	 if com[brnest] == 0 then
	    re = re:sub(1, brloc[brnest]) .. '\\{' .. re:sub(brloc[brnest] + 4)
	    re = re .. '\\}'
	 else
	    re = re .. ')'
	 end
	 brnest = brnest - 1
      else
	 re = re .. '\\' .. what
      end
   end
   re = re .. escall:sub(pos)
   while brnest > 0 do
      re = re:sub(1, brloc[brnest]) .. '\\{' .. re:sub(brloc[brnest] + 4)
      brnest = brnest - 1
   end
   -- let regex library detect mismatched []
   return '^' .. re .. '$'
end

-- this only applies a pattern to files in one directory
function dir_regex(dir, regex)
   if regex == nil then
      regex = dir
      dir = glib.get_current_dir()
   end
   if type(regex) == 'string' then
      local cf
      if glib.os ~= 'unix' then
	 -- probably unsafe to assume all non-UNIX are insensitive
	 -- but how else can it be determined w/o running tests?
	 cf = {'caseless'}
      end
      local msg
      regex, msg = glib.regex_new(regex, cf)
      if not regex then
	 return function() return nil end
      end
   end
   local d = glib.dir(dir)
   return function()
      while true do
	 local n = d()
	 if n == nil then return nil end
	 if regex:find(n) then
	    return n
	 end
      end
   end
end

-- this only applies a pattern to files in one directory
function dir_glob(dir, glob)
   if glob == nil then
      glob = dir
      dir = glib.get_current_dir()
   end
   return dir_regex(dir, glob_to_regex(glob))
end

-- this only works for relative paths, but allows pat to have directory
-- separators
-- there is no way to do absolute path patterns on Windows in any sane
-- portable manner
-- also, it's too much trouble to efficiently handle non-glob path elements
-- properly (i.e., access them directly instead of scanning the directory)
function relative_glob(path, pat)
   if not pat then
      pat = path
      path = glib.get_current_dir()
   end
   local cf
   if glib.os ~= 'unix' then
      -- probably unsafe to assume all non-UNIX are insensitive
      -- but how else can it be determined w/o running tests?
      cf = {'caseless'}
   end
   local rx = glib.regex_new(glob_to_regex(pat), cf, {'partial'})
   return coroutine.wrap(function()
      local function process_dir(p, match)
         local de
	 local di = glib.dir(p)
	 if di then
	    for de in di do
	       local fm
	       if match then fm = pathcat(match, de) else fm = de end
	       local m = rx:find(fm)
	       if m then coroutine.yield(fm) end
	       if m ~= nil and rx:find(fm .. glib.dir_separator:sub(1,1)) ~= nil then
		  process_dir(pathcat(p, de), fm)
	       end
	    end
	 end
      end
      process_dir(path, nil)
   end)
end

-- just expand curly braces in a shell-like manner
function shell_expand_braces(pat)
   local s
   local pref = {''}
   local entries = {{}}
   local com = {{0}}
   local nest = 1
   local o, c
   -- for each brace or comma, and its preceeding text...
   for o, c in glib.regex_new('((?:[^\\\\{},[]|\\\\.|\\[(?:[^\\\\\\]]|\\\\.)*\\])*)' ..
			      '([{},])'):gmatch(pat) do
      if c == '{' then
	 -- open brace: save prefix and start a new group
	 nest = nest + 1
	 entries[nest] = {}
	 pref[nest] = o
	 com[nest] = {0}
      elseif nest > 1 and c == ',' then
	 -- comma: tack prefix to any text accumulated for this group
	 -- entry so far.  This may be nothing, or more than one entry if
	 -- it contained nested brace expansion
	 local ocom = com[nest][1]
	 local ent = entries[nest]
	 local nent = #ent
	 if ocom ~= nent then
	    local i
	    for i = ocom + 1, nent do
	       ent[i] = ent[i] .. o
	    end
	 else
	    nent = nent + 1
	    ent[ocom + 1] = o
	 end
	 table.insert(com[nest], 1, nent)
      elseif nest > 1 and c == '}' then
	 -- end brace: tack prefix to any text accumulated for this group
	 -- entry so far.  This may be nothing, or more than one entry if
	 -- it contained nested brace expansion
	 local ocom = com[nest][1]
	 local ent = entries[nest]
	 local nent = #ent
	 local i
	 if ocom ~= nent then
	    for i = ocom + 1, nent do
	       ent[i] = ent[i] .. o
	    end
	 else
	    nent = nent + 1
	    ent[ocom + 1] = o
	 end
	 -- if there were no commas, restore open and close brace for all
	 -- text so far (due to above, at least one piece exists)
	 if ocom == 0 then
	    for i = 1, nent do
	       ent[i] = '{' .. ent[i] .. '}'
	    end
	 end
	 -- now, prefix all text with {'s prefix and then append to
	 -- all elements in parent group entry
	 local pre = pref[nest]
	 -- for easier debugging, clear out no-long-needed entries
	 entries[nest] = nil
	 pref[nest] = nil
	 com[nest] = nil
	 nest = nest - 1
	 local oent = entries[nest]
	 ocom = com[nest][1] + 1
	 local noent = #oent
	 if noent < ocom then
	    noent = noent + 1
	    oent[noent] = ''
	 end
	 for i = ocom, noent do
	    local j
	    local otxt = oent[ocom] .. pre
	    table.remove(oent, ocom)
	    for j = 1, nent do
	       table.insert(oent, otxt .. ent[j])
	    end
	 end
      end
   end
   -- now deal with unmatched open braces
   while nest > 1 do
      local ent = entries[nest]
      local nent = #ent
      local i
      local ocom = com[nest]
      local ncom = #ocom - 1
      -- prepend stripped { and its prefix
      local pre = pref[nest] .. '{'
      if ncom > 0 then
	 -- for comma entries, also tack on stripped comma
	 for i = 1, ocom[ncom] do
	    ent[i] = pre .. ent[i] .. ','
	 end
	 for i = ocom[ncom] + 1, ocom[1] do
	    ent[i] = ent[i] .. ','
	 end
	 -- next, merge comma groups
	 for i = 1, ncom do
	    local j, j1, k, k1, km
	    j1 = ocom[i + 1] + 1
	    k1 = ocom[i] + 1
	    km = #ent
	    -- create new last group by merging all text in
	    -- 2nd-to-last group (j1 .. k1 - 1) with last group (k1 .. km)
	    -- at same time, remove 2nd-to-last group
	    if km >= k1 then
	       for j = j1, ocom[i] do
		  for k = k1, km do
		     table.insert(ent, ent[j1] .. ent[k])
		  end
		  table.remove(ent, j1)
		  k1 = k1 - 1
		  km = km - 1
	       end
	       -- remove old last group
	       for k = k1, km do
		  table.remove(ent, k1)
	       end
	    end
	 end
      elseif nent > 0 then
	 for i = 1, nent do
	    ent[i] = pre .. ent[i]
	 end
      else
	 ent[1] = pre
      end
      -- now that all commas and braces are restored, merge with parent as
      -- if a close brace had occurred (but ignore prefix, since it's already
      -- been merged)
      entries[nest] = nil
      pref[nest] = nil
      com[nest] = nil
      nest = nest - 1
      local oent = entries[nest]
      ocom = com[nest][1] + 1
      local noent = #oent
      if noent < ocom then
	 noent = noent + 1
	 oent[noent] = ''
      end
      for i = ocom, noent do
	 local j
	 local otxt = oent[ocom]
	 table.remove(oent, ocom)
	 for j = 1, #ent do
	    table.insert(oent, otxt .. ent[j])
	 end
      end
   end
   -- finally, append any non-brace text at end to all entries and return
   local app = glib.regex_new('(?:[^\\\\{},[]|\\\\.|\\[(?:[^\\\\\\]]|\\\\.)*\\])*$'):match(pat)
   local ret = entries[1]
   if #ret == 0 then
      ret = {app}
   else
      local i
      for i = 1, #ret do
	 ret[i] = ret[i] .. app
      end
   end
   return ret
end	 
	    

-- here's one way to handle the const path issue: split off paths until a
-- potentially special char is found.  No special chars are allowed in
-- the "root" prefix (not enforced, but it won't work otherwise).
-- note that although it expands the braces shell-like, it still rejects
-- non-glob paths that do not exist
function glob(pat)
   local ent = shell_expand_braces(pat)
   local i, v
   return coroutine.wrap(function()
      for i, v in ipairs(ent) do
	 local s = 1
	 while true do
	    s = v:find('[*?{}%[%]\\]', s)
	    if not s or v:sub(s, s) ~= '\\' then break end
	    s = s + 2
	 end
	 if not s then
	    if exists(v) then
	       coroutine.yield(v)
	    end
	 else
	    local const = v:sub(1, s - 1):gsub('\\(.)', '%1')
	    local dir = glib.path_get_dirname(const)
	    local de
	    local var
	    if v:sub(s - 1, s - 1):match("[" .. glib.dir_separator .. "]") then
	       var = v:sub(s)
	    else
	       var = basename(v:sub(1, s - 1)) .. v:sub(s)
	    end
	    for de in relative_glob(const, var) do
	       coroutine.yield(pathcat(const, de))
	    end
	 end
      end
   end)
end

-- common uses of stat
function is_empty(f)
   local t, s = glib.stat(f, 'type', 'size')
   return not t or (t == 'file' and s == 0)
end

-- recursive delete
function rm(f)
   local ok, msg, err, gmsg
   if not is_link(f) and is_dir(f) then
      local e
      for e in glib.dir(f) do
	 ok, msg = rm(pathcat(f, e))
	 if not ok and not err then
	    err = true
	    gmsg = msg
	 end
      end
   end
   ok, msg = glib.remove(f)
   if not ok and not err then
      err = true
      gmsg = msg
   end
   if err then
      return false, gmsg
   else
      return true
   end
end

-- touch the way it's supposed to be
function touch(f, date)
   if not exists(f) then
      local fd = io.open(f, 'w')
      if fd then fd:close() end
   end
   glib.utime(f, nil, date)
end

-- apr-style basename
function basename(n, split)
   local ret = glib.path_get_basename(n)
   if not split then
      return ret
   end
   return glib.regex_new('^(.+?)(|\\.[^.]*)$'):match(ret)
end

-- shell-style getenv: undef returns empty string
-- that way you don't have to check for both blank and nil
function getenv(v)
   return glib.getenv(v) or ''
end

function setenv(name, val)
   return glib.setenv(name, val, true)
end

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
      return f:close()
   else
      return true
   end
end

local builtin_io_bufsize = 16384

-- copy a file
-- note that although Lua will eventually garbage collect files, it's
-- better to close them as soon as possible
function cp(src, dest, mode)
   local s = fopen(src, 'rb')
   if not s then
      return false, "Can't open " .. src
   end
   local d = fopen(dest, 'wb')
   if not d then
      fclose(s, src)
      return false, "Can't open " .. dest
   end
   local buf
   while true do
      buf = s:read(builtin_io_bufsize)
      if not buf then break end
      if not d:write(buf) then
	 fclose(s, src)
	 fclose(d, dest)
	 return false, "I/O error writing to " .. tostring(dest)
      end
   end
   fclose(s, src)
   if not fclose(d, dest) then
      return false, "I/O error writing to " .. tostring(dest)
   end
   if not mode and io.type(src) == 'file' then
      mode = glib.stat(src, 'perm')
   end
   if mode and io.type(dest) ~= 'file' then
      glib.chmod(dest, mode)
   end
   return true
end

-- recursive file comparison
function cmp(f1, f2)
   if is_dir(f1) then
      if not is_dir(f2) then return false end
      for p in glib.dir(f1) do
	 if not cmp(pathcat(f1, p), pathcat(f2, p)) then
	    return false
	 end
      end
      for p in glib.dir(f2) do
	 if not exists(pathcat(f1, p)) then
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
      f1b = f1d:read(builtin_io_bufsize)
      f2b = f2d:read(builtin_io_bufsize)
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
      -- this dereferences any link (-L)
      local t, m = glib.stat(src, 'type', 'perm')
      if plus_w then
	 m = m + 128 -- 0200 == 2 * 8^2 == 2 * 64
      end
      if t == 'dir' then
	 st, msg = mkdir(dest, m)
	 if not st then
	    return nil, "Error writing " .. dest .. ": " .. msg
	 end
	 -- drop through to d_to_d code
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
   local n
   for n in glib.dir(src) do
      local p = pathcat(src, n)
      -- this dereferences any link (-L)
      t, m = glib.stat(p, 'type', 'perm')
      if plus_w then
	 m = m + 128 -- 0200 == 2 * 8^2 == 2 * 64
      end
      local d = pathcat(dest, n)
      if t == 'dir' then
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

-- there is no portable ln -s, so try hard link then copy
function ln(s, d)
   if not glib.link(s, d, true) and not glib.link(s, d, false) then
      return cp_RpL(s, d)
   end
end

-- shell-style backtick-cat-as-args: get words from file (iterator)
-- could probably be improved by not sucking entire file into memory
function words(fn)
   local f = fopen(fn)
   if not f then return nil end
   local s = f:read('*a')
   fclose(f, fn)
   return s:gmatch('%S+')
end

-- shell-style backtick-cat-as args for making cmdline
-- grab whole file, and collapse intervening spaces
function wholefile(fn)
   local f = fopen(fn)
   if not f then return '' end
   local s = f:read('*a'):gsub('%s+', ' '):gsub(' $', '')
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
   local args = glib.shell_parse_argv(cmd)
   local i, v
   for i,v in ipairs(moreargs) do table.insert(args, v) end
   args.chdir = moreargs.chdir
   args.stderr = 'WARNINGS'
   args.stdout = 'MESSAGES'
   args.stdin = moreargs.stdin
   local ret, err
--   io.write('running')
--   for i, v in ipairs(args) do
--       io.write(' ' .. glib.shell_quote(v))
--   end
   io.write('\n')
   ret, err = glib.spawn(args)
   if ret then
      ret = ret:wait()
   else
      append_line('WARNINGS', err)
      ret = -1
   end
   if ret ~= 0 then
      -- on error, merge all captured output into single 'ERRORS' file
      local mf = io.open('MESSAGES', 'a')
      cp('WARNINGS', mf)
      mf:close()
      rm('WARNINGS')
      mv('MESSAGES', 'ERRORS')
      odin_error(args[1] .. ' failed')
   elseif moreargs.stdout then
      if moreargs.stdout ~= 'MESSAGES' then
	 mv('MESSAGES', moreargs.stdout)
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
    return s:gsub("^[\t\n ]*", ""):gsub("[ \t\n]*$", "")
end

-- return a string escaped as an Odin token
function odin_quote(s)
    return "'" .. s:gsub("'", "'\\''") .. "'"
end

-- return a string with parts between slashes escaped
function odin_quote_file(s, virt)
    if virt then
	return (s:gsub("[^/%]+", odin_quote))
    else
	return (s:gsub("[^/]+", odin_quote))
    end
end

-- configuration support
-- parse output from a configuration program as ldflags
-- any options other than -L and -l are returned in +ld_flags
function parse_ldflags(fl)
    local a = glib.shell_parse_argv(fl)
    local i, v, ret
    ret = ''
    for i, v in ipairs(a) do
	if v:sub(1, 2) == 'L' then
	    ret = ret .. "+lib_sp=" .. odin_quote(v:sub(3))
	elseif v:sub(1, 2) == '-l' then
	    ret = ret .. "+lib=" .. odin_quote(v:sub(3))
	else
	    ret = ret .. "+ld_flags=" .. odin_quote(v)
	end
    end
    return ret
end

-- parse output from a configuration program as cflags
-- any options other than -I and -D are returned in +cc_flags
-- -O and -g are ignored
function parse_cflags(fl)
    local a = glib.shell_parse_argv(fl)
    local i, v, ret
    ret = ''
    for i, v in ipairs(a) do
	if v:sub(1, 2) == '-I' then
	    ret = ret .. "+inc_sp=" .. odin_quote(v:sub(3))
	elseif v:sub(1, 2) == '-D' then
	    ret = ret .. "+define=" .. odin_quote(v:sub(3))
	elseif v:sub(1, 2) ~= '-O' and v:sub(1, 2) ~= '-g' then
	    ret = ret .. "+cc_flags=" .. odin_quote(v)
	end
    end
    return ret
end

-- there is no portable way to find pkg-config, so just use an env var
-- if not in path
pkgconfig = getenv("PKGCONFIG")
if pkgconfig == '' then
   pkgconfig = glib.find_program_in_path('pkg-config') or 'pkg-config'
end

-- run pkg-config for pkg; optionally requiring min/max version
-- arg is usually --cflags or --libs
function get_pkgconfig(arg, pkg, vermin, vermax)
    local cmd = pkgconfig .. ' ' .. arg
    if vermin then
	cmd = cmd .. ' --atleast-version=' .. vermin
    end
    if vermax then
	cmd = cmd .. ' --max-version=' .. vermax
    end
    cmd = cmd .. ' ' .. pkg
    local p, msg = glib.spawn{cmd = cmd, stderr = true}
    if not p then return p, msg end
    local ret, stdout, stderr = p:wait()
    if ret ~= 0 then return nil, stderr end
    return stdout
end

-- run pkg-config for ldflags
pkgconfig_libs = '--libs'
function pkg_libs(...)
    local fl = get_pkgconfig(pkgconfig_libs, ...)
    if not fl then return nil end
    return parse_ldflags(fl)
end

-- run pkg-config for cflags *and* ldflags
-- this is done with two runs so generic flags are separated properly
pkgconfig_cflags = '--cflags'
function pkg_cflags(...)
    local fl = get_pkgconfig(pkgconfig_cflags, ...)
    if not fl then return nil end
    return parse_cflags(fl) .. pkg_libs(...)
end
