#!/usr/bin/env lua

redo = io.open("redo", "w")
for l in io.lines(arg[1]) do
   redo:write(l .. "!:redo\n")
end
for l in io.lines(arg[1]) do
   redo:write(l .. "\n")
end
redo:close()
