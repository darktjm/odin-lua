#!/usr/bin/env lua

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

ODIN_regsub, ODIN_FILE, ODIN_match, ODIN_hide, ODIN_subst, ODIN_substonly,
ODIN_prefix, ODIN_suffix = unpack(arg)

odin_log(ODIN_regsub .. ' ' .. basename(ODIN_FILE))

match_re = nil
if ODIN_match ~= "" then
   re = ""
   for l in io.lines(ODIN_match) do
      ok, msg = glib.regex_new(l)
      if not ok then
	 odin_error("Error in match pattern '" .. l .. "': " .. msg, 0)
      end
      if re == "" then re = l else re = re .. "|" .. l end
   end
   match_re, msg = glib.regex_new(re)
   if not match_re then
      odin_error("Error in match pattern '" .. re .. "': " .. msg, 0)
   end
end

ODIN_hide = ODIN_hide ~= ""
ODIN_substonly = ODIN_substonly ~= ""

-- glib's substitution strings are OK, but they're missing &
-- I could just replace any unescaped & with \0, I guess, but since I
-- already had to do this for lrexlib, I'm keeping it as is.
re_sub = {}
-- \1 == re, \2 == replacement text (if present)
split_re = glib.regex_new("^((?:[^\\\\/]|\\\\.)*)(?:/(.*))?$")
-- non-special text (needs single level of \ stripped out, though)
non_sp = "(?:[^\\\\&]|\\\\[^0-9{])*"
-- non-special text followed by a single special
-- \1 == non-special text, \2 == special (&/\n/\{n|name}); \3 == name
one_repl = "(" .. non_sp .. ")(&|\\\\[0-9]|\\\\\\{(?:[0-9]+|([^0-9][^}]+))\\})"
one_repl_re = glib.regex_new(one_repl)
-- a replacement string: 0 or more specials (\1\2) followed by non-special (\3)
repl_re = glib.regex_new("^(?:" .. one_repl .. ")*(" .. non_sp .. ")$")
if ODIN_subst ~= "" then
   for l in io.lines(ODIN_subst) do
      p, s = split_re:match(l)
      if not p then
	 odin_error("Bad substitution pattern '" .. l .. "'", 0)
      end
      -- parens around p make subexpr 0 into subexpr 1
      p, msg = glib.regex_new("(" .. p .. ")")
      if not p then
	 odin_error("Error in substitution pattern '" .. l .. "': " .. msg, 0)
      end
      sub = {}
      if s then
	 nsub = p:get_capture_count()
	 flag, ign, ign, suf = repl_re:match(s)
	 if flag == nil then
	    odin_error("Error in substitution value '" .. l .. "'", 0)
	 end
	 suf = suf:gsub("\\(.)", "%1")
	 for prefix, repl, name in one_repl_re:gmatch(s) do
	    prefix = prefix:gsub("\\(.)", "%1")
	    if repl == "&" then
	       n = 1
	    elseif #repl == 2 then
	       n = tonumber(repl:sub(2)) + 1
	    elseif not name then
	       n = tonumber(repl:sub(3, -2)) + 1
	    else
	       n = p:get_string_number(name)
	       if n < 1 then
		  odin_error("Unknown subexpression name '" .. repl .. "' in '" .. l .. "'", 0)
	       end
	    end
	    if n > nsub then
	       odin_error("Subexpression refererence '" .. repl .. "' out of range in '" .. l .. "'", 0)
	    end
	    table.insert(sub, {prefix, n})
	 end
      else
	 suf = ""
      end
      table.insert(re_sub, {re = p, sub = sub, suf = suf, l = l})
   end
end

of = io.open(ODIN_regsub, "w")
for l in io.lines(ODIN_FILE) do
   if not match_re or (not match_re:find(l)) == ODIN_hide then
      printit = not ODIN_substonly
      for i, v in ipairs(re_sub) do
	 function do_repl(...)
	    local ss = {...}
	    local rs = ""
	    local i, sv
	    for i, sv in ipairs(v.sub) do
	       -- repl may be false if subexpr did not match
	       local s = ss[sv[2]] or ''
	       rs = rs .. sv[1] .. s
	    end
	    return rs .. v.suf
	 end
	 l, nsub = v.re:gsub(l, do_repl)
	 if nsub > 0 then printit = true end
      end
      if printit then of:write(ODIN_prefix .. l .. ODIN_suffix .. "\n") end
   end
end
