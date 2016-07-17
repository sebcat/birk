local irc = {
  created = "2016-07-17",
  description ="IRC client protocol implementation",
  copyright = "Sebastian Cato",
  license = "ISC License",
}

-- Grammar for RFC1459 2.3.1 (-ish...)
local lpeg = require "lpeg"
local crlf = lpeg.P("\r")^0 * "\n"
local SPACE = lpeg.P(" ")^1
local nonwhite = (1 - lpeg.S(" \0\r\n"))
local letter = lpeg.R("az") + lpeg.R("AZ")
local number = lpeg.R("09")
local trailing = lpeg.C((1 - lpeg.S("\0\r\n"))^0)
local middle = lpeg.C(nonwhite^1)
local params = (SPACE * ":" * trailing) + (SPACE * middle)
params = lpeg.Cg(lpeg.Ct(params^1), "params")
local command = lpeg.Cg((letter^1 + number^3), "cmd")
local name = lpeg.Cg((1-lpeg.S(" \0\r\n!"))^1, "name")
local user = lpeg.Cg((1-lpeg.S(" \0\r\n@"))^1, "user")
local host = lpeg.Cg(nonwhite^1, "host")
local prefix = name * (lpeg.P("!") * user)^0 * (lpeg.P("@") * host)^0
local message = (lpeg.P(":") * prefix * SPACE)^0 * command * params^0 * crlf^0
message = lpeg.Ct(message)

function irc.message(str)
  return message:match(str)
end

return irc
