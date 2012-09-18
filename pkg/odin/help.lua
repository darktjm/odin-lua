#!/usr/bin/env lua

-- Can't generate a lua executable portably, unless it's copied out from
-- something else.  Instead, use odin-expr

-- note that it is impossible to silence the echoing of this command.

of = io.open("odin_help", "w")
of:write [[
$ODINCACHE/PKGS/odin/help.txt>
]]
of:close()
