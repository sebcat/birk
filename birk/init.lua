local birk = {
  created = "2016-07-17",
  description ="birk init code",
  copyright = "Sebastian Cato",
  license = "ISC License",
}

birk.irc = require "birk.irc"

-- birk-agent registers a userdata object for IPC with the host process. If it
-- doesn't exist we're not started from birk-agent and can't continue.
local ipc = __birkipc
if ipc == nil then
  error("birk IPC not present")
end

-- Initializes a connection in the parent process. Connections are established
-- asyncronously by the parent and the interpreter is notified on success or
-- failure
function birk.connect_tcp(host,port)
  return ipc:connect_tcp(host,port)
end

ipc:ready() -- tell parent we're done loading
return birk
