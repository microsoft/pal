C:\"Program Files\Microsoft Visual Studio 8\Team Tools\Performance Tools"\vsinstr -coverage ..\target\windows_32_debug\testrunner.exe /EXCLUDE:CppUnit::* /EXCLUDE:std::*
C:\"Program Files\Microsoft Visual Studio 8\Team Tools\Performance Tools"\vsperfcmd -start:coverage -output:..\target\windows_32_debug\testcov.coverage
..\target\windows_32_debug\testrunner.exe
C:\"Program Files\Microsoft Visual Studio 8\Team Tools\Performance Tools"\vsperfcmd -shutdown