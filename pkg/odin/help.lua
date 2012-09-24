#!/usr/bin/env lua

-- Can't generate a lua executable portably.  Instead, use odin commands

of = io.open("odin_help", "w")
of:write [[
@$ODINCACHE/PKGS/odin/help.txt>
]]
of:close()
