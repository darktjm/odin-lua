#!/usr/bin/env lua

-- Note: this is not a lua translation of the old c.h.sh.  That script was
-- broken in so many ways that it seemed pointless to copy it.
-- Instead of a broken sed script, this uses cproto.

ODIN_FILE, ODIN_dir, ODIN_incsp, ODIN_define = unpack(arg)

-- in case run from cmd line, grab built-ins
if not runcmd then
    dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

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

if getenv("ODINVERBOSE") ~= "" then
   print(getenv("ODINRBSHOST") .. "cproto" .. flags .. " " .. apr.filepath_name(ODIN_FILE))
end

-- the following simulates the existing Func.hh files:
-- -f2 removes parameter names (types only)
-- -e adds "extern" to each prototype
runcmd("cproto" .. flags, {"-q", "-f2", "-e", ODIN_FILE, stdout = true})
apr.file_rename('MESSAGES', 'c.h')

