#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   d = os.getenv("ODINCACHE")
   if d and d ~= '' then
      d = d .. '/PKGS'
   else
      d = arg[0]:gsub("[/\\][^/\\]*[/\\][^/\\]*$" -- strip 2 path elts
   end
   dofile(d .. "/odin/odin_builtin.lua"))
end

-- Note: this is not a lua translation of the old c.h.sh.  That script was
-- broken in so many ways that it seemed pointless to copy it.
-- Instead of a broken sed script, this uses cproto.

ODIN_FILE, ODIN_dir, ODIN_incsp, ODIN_define = unpack(arg)

flags = ""
if ODIN_define ~= "" then
   for def in words(ODIN_define) do
      flags = flags .. " -D" .. def
   end
end
flags = flags .. " -I" .. ODIN_dir
if ODIN_incsp ~= "" then
   for sp in words(ODIN_incsp) do
      flags = flags .. " -I" .. sp
   end
end

odin_log("cproto" .. flags .. " " .. basename(ODIN_FILE))

-- the following simulates the existing Func.hh files:
-- -f2 removes parameter names (types only)
-- -e adds "extern" to each prototype
runcmd("cproto" .. flags, {"-q", "-f2", "-e", ODIN_FILE, stdout = 'c.h'})

