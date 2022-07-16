
Description: simple multithreading GUI folder copier.
Technologies: Qt 5.14, C++17, GoogleTests, filesystem, thread
Version: 1.0.0

July 2022, Sidelnikov Dmitry (c)

=== Known bugs ===

1) SIGABORT from filesystem::copy function
It is known a bug/crash from sigabort for many files from origin to write rpotected dir/destination
If we copy small number of files no sigabort happen. It is needed investigation.
Possible solution: SIGABORT can be caught by sig_handler fun