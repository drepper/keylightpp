Keylight interface
==================

This library and utlilities can be used to control Elgate Keylight lamps.

It is required to configure the device using Elgato's programs to use an
appropriate WiFi network.  After that no further configuration is needed
as long as mDNS traffic is not disrupted.

The command line utility `keylight` can be used to control the lamp(s) with
an applicable invocation.  It uses the `libkeylightpp.a` library underneath
which can be used by other code to get access to this type of devices.


Author:
Ulrich Drepper <drepper@gmail.com>
