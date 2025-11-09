# Orientprompt

A simple application to watch for device orientation changes and to popup a button on the display

If clicked, the orientation of the display is changed to match the device.

If ignored for 5 seconds the button disappears again

## Requirements

`wlr-randr` is called directly from this project, so it is a hard requirement. This project uses GTK4, gtk4-layer-shell and python
