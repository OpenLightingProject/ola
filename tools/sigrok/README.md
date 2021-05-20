This works with any logic analyser and the digital part of mixed signal devices
supported by Sigrok:
https://sigrok.org/wiki/Supported_hardware#Logic_analyzers
https://sigrok.org/wiki/Supported_hardware#Mixed-signal_devices

In order to compile the Sigrok RDM sniffer, you need libsigrok, there is
probably one in your package system, or there is a version available from:
https://sigrok.org/wiki/Libsigrok

If it isn't in the normal place, you will also need to set the include path to your libsigrok. Example:

```
./configure CPPFLAGS="-I/path/to/your/libsigrok/include"
```

The configure script should output something like this:

```
checking for libsigrok... yes
```
