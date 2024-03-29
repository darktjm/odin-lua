#!/usr/bin/env lua

redo_errs = io.open("redo_errs", "w")
for l in io.lines(arg[1]) do
   nl = l:gsub('^--- <(.*)> generated errors ---$', '%1 !:redo\n')
   if nl ~= l then redo_errs:write(nl) end
end
for l in io.lines(arg[1]) do
   nl = l:gsub('^** Summary of error messages for (.*)', '%1\n')
   if nl ~= l then redo_errs:write(nl) end
end
redo_errs:close()
