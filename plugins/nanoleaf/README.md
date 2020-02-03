Nanoleaf Plugin
============

This plugin creates a device per IP, each with one output port. Each port
represents a Nanoleaf Aurora controller, with up to 30 panels attached.
This plugin uses the streaming external control effects functionality of
the Aurora OpenAPI.

Currently only the [Nanoleaf
Aurora](https://nanoleaf.me/en/products/smarter-series/nanoleaf-light-panels-smarter-kit/)
is supported.

The Nanoleaf Aurora must be in "ExternalStreaming" mode UDP Enabled via Nanoleaf API:

`{"write": { "command": "display", "animType": "extControl" }}`

## Config file: `ola-nanoleaf.conf`

`controller = <ip>`  
The IP of the controller to send to. You can communicate with more than
one controller by adding multiple `controller =` lines

### Per Device Settings

`<controller IP>-port = <int>`
The port to stream to on the controller, range is 1 - 65535.

`<controller IP>-panels = [int]`
The list of panel IDs to control, each panel is mapped to three DMX512 slots 
(for Red, Green and Blue). 
Panel IDs are comma separated eg: `321,789,123`
