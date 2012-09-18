#!/usr/bin/env lua

-- in case run from cmd line, grab built-ins
if not runcmd then
   dofile(string.gsub(arg[0], "[/\\][^/\\]*[/\\][^/\\]*$", "/odin/odin_builtin.lua"))
end

ODIN_OBJECT, ODIN_dest = unpack(arg)

if ODIN_dest == "" then
   odin_error(':copy must have a +copy_dest parameter', 0)
end

if apr.stat(ODIN_OBJECT, 'type') == 'directory' then
   odin_log("** Copying up-to-date value into " .. ODIN_dest)
   if apr.stat(ODIN_dest, 'type') ~= 'directory' then
      apr.dir_make_recursive(ODIN_dest)
   end
   if apr.stat(ODIN_dest, 'type') ~= 'directory' then
      odin_error('Directory cannot be made at destination of copy: ' .. ODIN_dest, 0)
   end
   -- emulate cp -RL
   -- note that specials (sockets, pipes, etc.) can't possibly be copied
   -- portably
   -- the chmod -R +w `basename ODIN_dest` was seriously wrong (affects
   -- too many files and gives too many permissions); limiting it to
   -- ODIN_dest and making it u+w only is OK, though
   function cp_RL(src, dest)
      local dir = apr.dir_open(src)
      local p, n
      -- can't use 'type' here because soft links show up as link
      for p, n in dir:entries('path', 'name') do
	 -- this dereferences any link (-L)
	 local t, m = apr.stat(p, 'type', 'protection')
	 -- chmod +w is too much; just do u+w
	 m = string.sub(m, 1, 1) .. 'w' .. string.sub(m, 3)
	 local d = apr.filepath_merge(dest, n)
	 if t == 'directory' then
	    st, msg = apr.dir_make(d, m)
	    if not st then
	       print("Error writing " .. d .. ": " .. msg)
	       os.exit(1)
	    end
	    cp_RL(p, d)
	 elseif t == 'file' then
	    st, msg = apr.file_copy(p, d, m)
	    if not st then
	       print("Error writing " .. d .. ": " .. msg)
	       os.exit(1)
	    end
	 else
	    io.open("ERRORS", "a"):write('Ignoring copy attempt on special ' .. p .. '\n')
	 end
      end
   end
   cp_RL(ODIN_OBJECT, ODIN_dest)
   os.exit(0)
end

if apr.stat(ODIN_dest, 'type') == 'directory' then
   io.open("ERRORS", "w"):write('Directory already exists at destination of copy: ' .. ODIN_dest .. '\n')
   os.exit(0)
end

in_f = io.open(ODIN_OBJECT, 'rb')
if not in_f then
   io.open("ERRORS", "w"):write('Source file not found: ' .. ODIN_OBJECT .. '\n')
   os.exit(0)
end
out_f = io.open(ODIN_dest, 'rb')
if out_f then
   repeat
      il = in_f:read(16384)
      ol = out_f:read(16384)
   until il ~= ol or il == nil
   if il == ol then
      odin_log("** Verified up-to-date: " .. ODIN_dest)
      os.exit(0)
   end
end

apr.file_remove(ODIN_dest)
m = apr.stat(ODIN_OBJECT, 'protection')
-- bug fix wrt original: chmod u+w for copied out files
m = string.sub(m, 1, 1) .. 'w' .. string.sub(m, 3)
st, msg = apr.file_copy(ODIN_OBJECT, ODIN_dest, m)
if not st then
   print("Error writing " .. ODIN_dest .. ": " .. msg)
   os.exit(1)
end
odin_log("** Copied up-to-date value into: " .. ODIN_dest)
