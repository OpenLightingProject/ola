USB DMX Plugin Developer information
====================================

This plugin uses [libusb](http://libusb.info/) to communicate with USB devices.

libusb supports synchronous and asynchronous transfers. The initial version of
this plugin used the synchronous interface, and spawned a thread for every USB
device present.

The new version of the plugin uses the asynchronous mode of operation and a
single thread for the libusb completion handling. This allows us to support
hotplug.

You can opt-out of the new asynchronous mode by passing the
`--no-use-async-libusb` flag to olad. Assuming we don't find any problems, at
some point the synchronous implementation will be removed.

The rest of this file explains how the plugin is constructed and is aimed at
developers wishing to add support for a new USB Device. It assumes the reader
has an understanding of libusb.

## Terminology

*USB Device*, is the physical USB device attached to the host.

*DMX512 Interface*, the physical socket on the USB Device the user plugs the
DMX512 cable into. Currently all the USB Devices supported by the plugin
contain a single DMX512 Interface.

*libusb\_device*, is the libusb structure that represents a USB device.

*Widget*, is OLA's internal object representation of a USB Device. Widgets use
the corresponding libusb\_device to communicate with the USB Device.

*Port*, the software representation of a DMX512 Interface. In OLA, users
associate a Port with a Universe.

*Device*, the software representation of a USB Device. Contains one or more
Ports.


## Code Concepts & Structure

USB Devices are represented as Widgets, this allows us to de-couple the Widget
code from OLA's Port representation (remember, prefer composition over
inheritance). Since the majority of USB devices we support so far have a single
DMX512 interface, each specific Widget (e.g. AnymauDMX) derives from the
`Widget` class. This isn't strictly necessary, it just means we can avoid code
duplication by using the `GenericDevice` and `GenericPort` classes.
Multi-interface USB devices which are supported, shouldn't inherit from the
Widget class (see the Nodle U1 for example, they will need to use the `Device`
class instead).

`GenericPort` wraps a Widget into a `Port` object, so it can show up in olad.
`GenericDevice` creates a Device with a single `GenericPort`.

For each type of USB Device, we create a common base class, e.g. `AnymauDMX`.
This enables the `WidgetObserver` class (see below) to know what
name it should give the resultant Device.

Then for each type of USB Device, we create a synchronous and asynchronous
version of the Widget. So you end up with something like:

* Widget
  * AnymauDMX
    * SynchronousAnymaWidget
    * AsynchronousAnymaWidget
  * Sunlite
    * SynchronousSunliteWidget
    * AsynchronousSunliteWidget

Each type of USB Device has an associated factory. When a new libusb\_device is
discovered, each factory has the opportunity to claim the new device. Typically
factories will check the vendor and product ID and, if they recognize the
device, create a Widget to represent it.

If a factory creates a new Widget, it needs to notify the WidgetObserver. The
WidgetObserver can then go ahead and use the newly created Widget to setup an
OLA Device & Port.


## Adding Support for a new USB Device

Adding support for a new USB device should be reasonably straightforward. This
guide assumes the new USB Device has a single DMX512 Interface.

1. Create the `FooWidget.{h,cpp}` files.
 - Create the `FooWidget` base class, and a synchronous and asynchronous
   implementation of the Foo Widget.
2. Create the `FooWidgetFactory.{h,cpp}` files.
 - Write the `DeviceAdded()` method, to detect the new USB Device and create
   either a synchronous or asynchronous widget, depending on the
   `--use-async-libusb` flag.
3. Extend the `SyncronizedWidgetObserver` with new `NewWidget()` and
   `WidgetRemoved()` removed methods for the new FooWidget.
4. Implement the new `NewWidget()` and `WidgetRemoved()` methods in both the
   SyncPluginImpl and AsyncPluginImpl.