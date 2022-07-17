
Description: simple multithreading GUI folder copier.
Technologies: Qt 5.14, C++17, GoogleTests, filesystem, thread
Version: 1.0.0
Date: July 2022, Sidelnikov Dmitry (c)

The SimpleCopier v 1.0.0 program is a simple multithreaded GUI directory copier. I think the code is far from perfect. The development time is 3 days. It is unlikely that there are no bugs in it and I doubt that all corner cases are taken into account. Threads for copying are created as many as there are available cores on your CPU. Only errors occurring in threads are logged, higher-level errors are output to the GUI via messageboxes. Googletest's created in the latest version of Microsoft Visual Studio. It supports them just out of the box very conveniently. Unit tests do not cover all the functions that are needed (7 out of 9) from the file copylib.cpp. 9 unit tests were added. There is something to work on in the next version of the program. :) All comments in the source code are in English. I tried to apply the best practices known to me. The libraries filesystem, thread, C++17 standard are used. Atomics, mutexes are used, the singleton pattern is used. You can improve the quality of the code: attract a testing team, add more unit tests, use static code analyzer, add a code review procedure, increase development time. I am open to any comments. :)
