#!/usr/bin/env lua

rex = require 'rex_posix'

ODIN_regsub, ODIN_FILE, ODIN_match, ODIN_hide, ODIN_subst,
ODIN_substonly = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. ODIN_regsub .. ' ' .. apr.filepath_name(ODIN_FILE))
end

-- refl = rex.flags()
-- nosub = refl['NOSUB'] + refl['EXTENDED']
-- rex returns screwy results with NOSUB, so just drop it
nosub = nil

match_re = nil
if ODIN_match ~= "" then
   re = ""
   for l in io.lines(ODIN_match) do
      ok, msg = pcall(rex.new, l, nosub)
      if not ok then
	 io.open('ERRORS', 'a'):write("Error in match pattern '" .. l .. "': " .. msg .. "\n")
	 os.exit(0)
      end
      if re == "" then re = l else re = re .. "|" .. l end
   end
   ok, msg = pcall(rex.new, re, nosub)
   if ok then
      match_re = msg
   else
      io.open('ERRORS', 'a'):write("Error in match pattern '" .. re .. "': " .. msg .. "\n")
      os.exit(0)
   end
end

ODIN_hide = ODIN_hide ~= ""
ODIN_substonly = ODIN_substonly ~= ""

re_sub = {}
-- \1 == re, \4 == replacement text (if present)
split_re = rex.new("^(([^\\\\/]|\\\\.)*)(/(.*))?$")
-- non-special text followed by a single special
-- \1 == non-special text, \3 == special (3 total)
one_repl = "(([^\\\\&]|\\\\[^0-9{])*)(&|\\\\[0-9]|\\\\\\{[0-9]+\\})"
one_repl_re = rex.new(one_repl)
-- a replacement string: 0 or more specials followed by non-special (\6)
-- \1 is constructed so it always matches; i.e. it is validity flag
repl_re = rex.new("^((" .. one_repl .. ")*(([^\\\\&]|\\\\[^0-9])*))$")
if ODIN_subst ~= "" then
   for l in io.lines(ODIN_subst) do
      p, ign, ign, s = split_re:match(l)
      if not p then
	 io.open('ERRORS', 'a'):write("Bad substitution pattern '" .. l .. "'\n")
	 os.exit(0)
      end
      ign = nil
      ok, msg = pcall(rex.new, "(" .. p .. ")", nosub)
      if not ok then
	 io.open('ERRORS', 'a'):write("Error in substitution pattern '" .. l .. "': " .. msg .. "\n")
	 os.exit(0)
      end
      sub = {}
      if s then
	 flag, ign, ign, ign, ign, suf = repl_re:match(s)
	 if not flag then
	    io.open('ERRORS', 'a'):write("Error in substitution value '" .. l .. "'\n")
	    os.exit(0)
	 end
	 suf = string.gsub(suf, "\\(.)", "%1")
	 for prefix, ign, repl in rex.gmatch(s, one_repl_re) do
	    prefix = string.gsub(prefix, "\\(.)", "%1")
	    -- should really ensure # <= # of ('s, but that's too hard here
	    -- instead, it's done at subst time
	    -- so no subst means no error message <sigh>
	    if repl == "&" then
	       table.insert(sub, {prefix, 0})
	    elseif #repl == 2 then
	       table.insert(sub, {prefix, tonumber(string.sub(repl, 2))})
	    else
	       table.insert(sub, {prefix, tonumber(string.sub(repl, 3, -2))})
	    end
	 end
      else
	 suf = ""
      end
      table.insert(re_sub, {re = msg, sub = sub, suf = suf, l = l})
   end
end

of = io.open(ODIN_regsub, "w")
for l in io.lines(ODIN_FILE) do
   if not match_re or (match_re:match(l) == nil) == ODIN_hide then
      printit = not ODIN_substonly
      for i, v in ipairs(re_sub) do
	 -- n is a workaround for a bug in Lrexlib: $ is replaced twice
	 -- so, if a null replacement is done at the same location twice
	 -- in a row, it is ignored
	 lastn = -1
	 function n(s, e, o)
	    if s > e then
	       if s == lastn then
		  return false
	       end
	       lastn = s
	    end
	    return true
	 end
	 function do_repl(...)
	    local ss = {...}
	    local rs = ""
	    local i, sv
	    for i, sv in ipairs(v.sub) do
	       local ns = ss[sv[2] + 1]
	       if ns == nil then
		  io.open('ERRORS', 'a'):write("Bad subexpression reference in '" .. v.l .. "'\n")
		  os.exit(0)
	       end
	       -- ns may be false if ()?, ()*, or similar
	       if not ns then ns = "" end
	       rs = rs .. sv[1] .. ns
	    end
	    return rs .. v.suf
	 end
	 l, nsub = rex.gsub(l, v.re, do_repl, n)
	 if nsub > 0 then printit = true end
      end
      if printit then of:write(l .. "\n") end
   end
end
