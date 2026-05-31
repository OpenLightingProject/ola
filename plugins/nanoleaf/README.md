Nanoleaf Plugin
============

This plugin creates a device per IP, each with one output port. Each port
represents a Nanoleaf Aurora controller, with up to 30 panels attached.
This plugin uses the streaming external control effects functionality of
the Aurora OpenAPI.

Currently the [Aurora/Light 
Panels](https://nanoleaf.me/en-US/products/nanoleaf-light-panels/)
and [Shapes](https://nanoleaf.me/en-US/products/nanoleaf-shapes/)
are supported, other products in the range should probably work too
but haven't been tested yet. Please let us know if you do and whether
it works or not.


## Config file: `ola-nanoleaf.conf`

`controller = <ip>`  
The IP of the controller to send to. You can communicate with more than
one controller by adding multiple `controller =` lines

### Per Device Settings

`<controller IP>-port = <int>`
The port to stream to on the controller, range is 1 - 65535. Normally 60221
for v1 (or whatever it returns as part of the setup request) and 60222
for v2.

`<controller IP>-version = [v1 | v2]`
The streaming control version to use, set via the extControlVersion key when
enabling external control, defaults to v1. Canvas only supports v2.

`<controller IP>-panels = [int]`
The list of panel IDs to control, each panel is mapped to three DMX512 slots
(for Red, Green and Blue). There is currently no support for white control
via the v2 protocol.
