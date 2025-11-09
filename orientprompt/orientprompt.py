from ctypes import CDLL

CDLL("libgtk4-layer-shell.so")
import sys
import gi
import logging
gi.require_version("Gtk", "4.0")
gi.require_version("Gtk4LayerShell", "1.0")
from gi.repository import Gtk, GLib, Gio, Gtk4LayerShell
from sh import wlr_randr

log = logging.getLogger(__name__)
class OrientPrompt(Gtk.Application):
    """
    Application
    """

    def __init__(self):
        """
        Create a GTK app, prepare fields to use for later
        """
        super().__init__(application_id="io.github.trigg.orientprompt")
        self.image = Gtk.Picture() # The widget to draw
        self.window = None # The window to draw
        self.last_set = "normal" # The previously set display orientation.
        self.proxy = None # The DBUS proxy of physical orientation
        self.blank_timeout = None # A GLib source for timeout events

        click = Gtk.GestureClick.new()
        click.connect("released", self.make_it_so)
        self.image.add_controller(click) # React to click/touch

        self.connect("activate", self.activate)

    def get_screen(self):
        """
        Get the monitor for this device. If more than one exists, this app should temporarily disable
        """
        monitors = self.window.get_display().get_monitors()
        if monitors.get_n_items() == 1:
            return monitors.get_item(0).get_connector()
        return None


    def get_orientation(self):
        """
        Get physical orientation from DBUS
        """
        if not self.proxy:
            return None
        for value in self.proxy.get_cached_property_names():
            if value == "AccelerometerOrientation":
                return self.proxy.get_cached_property(value).get_string()
        return None
        
    def make_it_so(self, _a=None, _b=None, _c=None, _d=None):
        """
        Change display orientation to the current physical orientation
        """
        if not self.proxy:
            return
        monitor = self.get_screen()
        if not monitor:
            return
        # TODO Less stupid version of this
        value = self.get_orientation()
        self.last_set = value
        self.window.hide()
        self.unset_timer()
        if value == "normal":
            wlr_randr("--output",
                    monitor,
                    "--transform",
                    "normal")
                    
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.RIGHT, True)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.BOTTOM, True)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.LEFT, False)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.TOP, False)
        elif value == "right-up":
            wlr_randr("--output",
                    monitor,
                    "--transform",
                    "270")
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.RIGHT, True)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.BOTTOM, False)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.LEFT, False)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.TOP, True)
        elif value == "left-up":
            wlr_randr("--output",
                    monitor,
                    "--transform",
                    "90")
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.RIGHT, False)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.BOTTOM, True)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.LEFT, True)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.TOP, False)
        elif value == "bottom-up":
            wlr_randr("--output",
                    monitor,
                    "--transform",
                    "180")
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.RIGHT, False)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.BOTTOM, False)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.LEFT, True)
            Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.TOP, True)
        else:
            log.error("Unknown orientation: %s", orient)

    def activate(self, _app=None, _arg=None):
        """
        "App has started" callback
        """
        self.window = Gtk.Window()
        self.add_window(self.window)
        self.window.set_child(self.image)
        if not Gtk4LayerShell.is_supported():
            sys.exit(1)
        self.window.set_size_request(100,100)
        paintable = Gtk.IconTheme.get_for_display(self.window.get_display()).lookup_icon("rotation-allowed-symbolic",None, 128, 1, Gtk.TextDirection.LTR,0)
        self.image.set_paintable(paintable)
        Gtk4LayerShell.init_for_window(self.window)
        Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.RIGHT, True)
        Gtk4LayerShell.set_anchor(self.window, Gtk4LayerShell.Edge.BOTTOM, True)
        Gtk4LayerShell.set_layer(self.window, Gtk4LayerShell.Layer.OVERLAY)

        Gio.bus_watch_name(Gio.BusType.SYSTEM,
            "net.hadess.SensorProxy",
            Gio.BusNameWatcherFlags.NONE,
            self.name_appeared,
            self.name_vanished)

    def device_oriented(self, direction):
        """
        Physical orientation change callback
        """
        if not direction:
            return
        monitor = self.get_screen()
        if not monitor:
            self.window.hide()
            log.error("Orientation changed with incorrect monitor configuration")
            return
        value = self.get_orientation()
        if self.last_set == direction:
            self.window.hide()
            self.unset_timer()
            return
        
        self.window.present()
        self.window.show()
        self.set_timer()

    def unset_timer(self):
        """
        Unset timer callback
        """
        if self.blank_timeout:
            GLib.source_remove(self.blank_timeout)
            self.blank_timeout = None

    def set_timer(self):
        """
        Set timer callback, unset previous if still running
        """
        self.unset_timer()
        self.blank_timeout = GLib.timeout_add_seconds(5, self.timer_elapsed)

    def timer_elapsed(self):
        """
        Timer elapsed callback
        """
        self.window.hide()
        self.blank_timeout = None

    def name_appeared(self, conn, name, owner ):
        """
        Orientation device appeared callback
        """
        proxy = Gio.DBusProxy.new_sync(
            conn,
            Gio.DBusProxyFlags.NONE,
            None,
            name,
            "/net/hadess/SensorProxy",
            "net.hadess.SensorProxy",
            None
        )
        proxy.call_sync("ClaimAccelerometer", None, Gio.DBusCallFlags.NONE, -1, None)
        
        self.proxy_changed(proxy, None, None)
        
        proxy.connect("g-properties-changed", self.proxy_changed)
        self.proxy = proxy
        pass

    def name_vanished(self, conn, name):
        """
        Orientation device disappeared callback
        """
        self.proxy = None
        pass

    def proxy_changed(self, proxy, changed_list, invalid):
        """
        DBus proxy changed value callback
        """
        self.device_oriented(self.get_orientation())

def entrypoint():
    """
    Start point
    """
    logging.getLogger().setLevel(logging.INFO)
    app = OrientPrompt()
    app.run()