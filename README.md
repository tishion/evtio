# evtio = Event Based IO Library

This is just a toy project.

evtio is a simple wrapper of 
- epoll - linux
- kqueue - macOS
- IOCP - Windows

Because the wrapper is Reactor mode but IOCP is proactor mode so it doesn't provide send ability on Windows for IOCP