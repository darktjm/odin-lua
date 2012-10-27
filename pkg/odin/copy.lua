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

ODIN_OBJECT, ODIN_dest = unpack(arg)

if ODIN_dest == "" then
   odin_error(':copy must have a +copy_dest parameter', 0)
end

if is_dir(ODIN_OBJECT) then
   odin_log("** Copying up-to-date value into " .. ODIN_dest)
   if not is_dir(ODIN_dest) then
      mkdir(ODIN_dest)
   end
   if not is_dir(ODIN_dest) then
      odin_error('Directory cannot be made at destination of copy: ' .. ODIN_dest, 0)
   end
   -- the chmod -R +w `basename ODIN_dest` was seriously wrong (affects
   -- too many files and gives too many permissions); limiting it to
   -- ODIN_dest and making it u+w only is OK, though
   local st, msg = cp_RpL(ODIN_OBJECT, ODIN_dest, true)
   if not st then
      -- assumes all errors are transient, but actually
      -- attempts to copy dir to file or file to dir or specials are not
      print(msg)
      os.exit(1)
   end
   os.exit(0)
end

if is_dir(ODIN_dest) then
   odin_error('Directory already exists at destination of copy: ' .. ODIN_dest, 0)
end

in_f = io.open(ODIN_OBJECT, 'rb')
if not in_f then
   odin_error('Source file not found: ' .. ODIN_OBJECT, 0)
end
if cmp(ODIN_OBJECT, ODIN_dest) then
   odin_log("** Verified up-to-date: " .. ODIN_dest)
   os.exit(0)
end

rm(ODIN_dest)
m = glib.stat(ODIN_OBJECT, 'perm')
-- bug fix wrt original: chmod u+w for copied out files
m = m + 128 -- 0200 == 2 * 8^2 == 2 * 64
st, msg = cp(ODIN_OBJECT, ODIN_dest, m)
if not st then
   -- assumes all errors are transient, but actually
   -- attempts to copy dir to file or file to dir or specials are not
   print("Error writing " .. ODIN_dest .. ": " .. msg)
   os.exit(1)
end
odin_log("** Copied up-to-date value into: " .. ODIN_dest)
