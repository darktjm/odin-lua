#!/usr/bin/env lua

-- EXEC (oneTex.sh) (:tex) (:label) => (:one_tex_vtd)

otv = io.open('one_tex_vtd', 'w')
for l in io.lines(arg[2]) do
   -- note: original shell script did not have terminating '; this seems
   -- like a bug.
   l = string.gsub(l, '(.*)%.[^.]*$', "%%%1.tex=='" .. arg[1] .. "'")
   otv:write(l .. '\n')
end
otv:close()
