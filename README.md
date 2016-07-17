# birk

birk is an IRC agent with security in mind.

The main focus is to run birk as an IRC bot. It should however be possible to
also run birk as a client if needed.

## Project status

This project is not even close to be finished

## Dependencies

+ [lua](https://www.lua.org/download.html)
+ [lpeg](http://www.inf.puc-rio.br/~roberto/lpeg/)
+ [libseccomp](https://github.com/seccomp/libseccomp) (Linux only)

It is recommended to use your package manager to install these dependencies

Fedora 24:

````
dnf install lua-devel lua-lpeg libseccomp-devel
````

FreeBSD:

````
pkg install lua52 lua52-lpeg
````
