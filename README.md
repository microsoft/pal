# pal [![Build Status](https://travis-ci.org/Microsoft/pal.svg?branch=master)](https://travis-ci.org/Microsoft/pal)

The PAL is a Platform Abstraction Layer that is used in a variety of
projects. The PAL allows for easy compatibility between many different
flavors of UNIX/Linux, including AIX 6.1 and later, HP/UX 11.31 and
later, Solaris 5.10 and later, and most versions of Linux as far back
as RedHat 5.0, SuSE 10.1, and Debian 5.0.

The PAL has two primary components:

- [SCXCoreLib](#scxcorelib)
- [SCXSystemLib](#scxsystemlib)

Testing strategy is discussed [here](#testing-strategy).

### SCXCoreLib

SCXCoreLib provides portable services for:

- atomic handling
- UNIX conditions
- directory lookup
- file handling (reading/writing/modifying)
- ref-counted handle mechanism to release on last use
- common logging framework
- marshalling/unmarshalling
- DNS name resolution
- pocess launching/controlling
- regular expressions
- singletons
- threading
- time handling
- other various functions

The above list is not intended to be all-inclusive. Features are added
to the PAL as necessary.

### SCXSystemLib

SCXSystemLib primarily returns statistical information for products
like [SCXcore][] and [SCXcm][], and other [omi][] providers. This
subsystem primarily follows the CIM module (enumerate a set of
instances, get detailed information on a specific instance).

[SCXcore]: https://github.com/Microsoft/SCXcore
[SCXcm]: https://github.com/Microsoft/SCXcm
[omi]: https://github.com/Microsoft/omi

Primary enumeration information is supported for:

- bios
- computer system
- cpu statistics
- cpu property information
- disk information
- file system information
- installed software
- memory information
- network configuration
- network routing information
- operating system information
- process information
- processor information

This component tends to be highly system specific, and may run on
fewer systems than [SCXCoreLib](#scxcorelib), above.

### Testing Strategy

The PAL has a fair number of unit tests. Unit tests were written with
both [dependency injection][], and to a lesser extent, [mocking][] in
mind. By using dependency injection, we can control system
dependencies well and specifically test code fragments even if unit
tests are run on a system without necessary hardware configuration.

Examples for dependency injection include:

- [CPU handling][]: See class CPUPALTestDependencies and how it provides
special /proc/cpuinfo files for dependency injection purposes, regardless
of how the physical machine is actually configured.
- [Disk handling][]: See `bug*` files and how associated code handles those files.

There is also a very simplistic example of [dependency injection][] in
the bash script [GetLinuxOS.sh][], tested by [getlinuxos_test.cpp][].
In this example, [GetLinuxOS.sh][] is tested for proper platform handling
regardless of the actual platform that the test is running on.

[dependency injection]: https://en.wikipedia.org/wiki/Dependency_injection
[mocking]: https://en.wikipedia.org/wiki/Mock_object

[CPU handling]: https://github.com/Microsoft/pal/tree/master/test/code/scxsystemlib/cpu
[Disk handling]: https://github.com/Microsoft/pal/tree/master/test/code/scxsystemlib/disk

[GetLinuxOS.sh]: https://github.com/Microsoft/pal/blob/master/source/code/scxsystemlib/common/GetLinuxOS.sh
[getlinuxos_test.cpp]: https://github.com/Microsoft/pal/blob/master/test/code/scxsystemlib/common/getlinuxos_test.cpp

### Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct]
(https://opensource.microsoft.com/codeofconduct/).  For more
information see the [Code of Conduct FAQ]
(https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any
additional questions or comments.
