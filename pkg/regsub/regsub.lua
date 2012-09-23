#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

rex = require 'rex_posix'

ODIN_regsub, ODIN_FILE, ODIN_match, ODIN_hide, ODIN_subst, ODIN_substonly,
ODIN_prefix, ODIN_suffix = unpack(arg)

odin_log(ODIN_regsub .. ' ' .. basename(ODIN_FILE))

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
	 odin_error("Error in match pattern '" .. l .. "': " .. msg, 0)
      end
      if re == "" then re = l else re = re .. "|" .. l end
   end
   ok, msg = pcall(rex.new, re, nosub)
   if ok then
      match_re = msg
   else
      odin_error("Error in match pattern '" .. re .. "': " .. msg, 0)
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
	 odin_error("Bad substitution pattern '" .. l .. "'", 0)
      end
      ign = nil
      ok, msg = pcall(rex.new, "(" .. p .. ")", nosub)
      if not ok then
	 odin_error("Error in substitution pattern '" .. l .. "': " .. msg, 0)
      end
      sub = {}
      if s then
	 flag, ign, ign, ign, ign, suf = repl_re:match(s)
	 if not flag then
	    odin_error("Error in substitution value '" .. l .. "'", 0)
	 end
	 suf = string.gsub(suf, "\\(.)", "%1")
	 for prefix, ign, repl in rex.gmatch(s, one_repl_re) do
	    prefix = string.gsub(prefix, "\\(.)", "%1")
	    -- should really ensure # <= # of ('s, but that's too hard here
	    -- instead, it's done at subst time
	    -- so no subst means no error message <sigh>
	    if repl == "&" then
	       table.insert(sub, {prefix, 1})
	    elseif #repl == 2 then
	       table.insert(sub, {prefix, tonumber(string.sub(repl, 2)) + 1})
	    else
	       table.insert(sub, {prefix, tonumber(string.sub(repl, 3, -2)) + 1})
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
	 -- n is a workaround for a bug in Lrexlib: null matches at end
	 -- match twice.  so, if a null replacement is done at the same
	 -- location as the last end-of-replacment, ignore it.  This is
	 -- still not perfect, but handles most cases I use ($ and .*)
	 lastn = -1
	 function n(s, e, o)
	    if s > e then
	       if e == lastn then
		  return false
	       end
	    end
	    lastn = e
	    return true
	 end
	 function do_repl(...)
	    local ss = {...}
	    local rs = ""
	    local i, sv
	    for i, sv in ipairs(v.sub) do
	       local ns = ss[sv[2]]
	       if ns == nil then
		  odin_error("Bad subexpression reference in '" .. v.l .. "'", 0)
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
      if printit then of:write(ODIN_prefix .. l .. ODIN_suffix .. "\n") end
   end
end
