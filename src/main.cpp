#include "output_manager.h"
#include <gtkmm.h>
#include <glibmm.h>
#include <giomm.h>
#include <gtk4-layer-shell.h>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>

class OrientPrompt : public Gtk::Application
{
  private:
    Gtk::Window *window = nullptr;
    Gtk::Picture image;
    Glib::RefPtr<Gio::DBus::Proxy> proxy;
    sigc::connection blank_timeout;
    unsigned int bus_watch_id = 0;
    std::unique_ptr<OutputManager> output_mgr = std::make_unique<OutputManager>();
    std::string get_current_wayland_transform_string()
    {
        std::string monitor = get_screen();
        if (monitor.empty() || !output_mgr || !is_internal_monitor(monitor))
        {
            return "normal";
        }

        for (const auto& out : output_mgr->get_outputs())
        {
            if (out->connector_name == monitor)
            {
                switch (out->current_transform)
                {
                  case WL_OUTPUT_TRANSFORM_90:
                    return "left-up";

                  case WL_OUTPUT_TRANSFORM_180:
                    return "bottom-up";

                  case WL_OUTPUT_TRANSFORM_270:
                    return "right-up";

                  default:
                    return "normal";
                }
            }
        }

        return "normal";
    }

    bool is_internal_monitor(const std::string& name)
    {
        return (name.rfind("eDP-", 0) == 0 ||
            name.rfind("DSI-", 0) == 0 ||
            name.rfind("LVDS-", 0) == 0);
    }

  public:
    OrientPrompt() : Gtk::Application("io.github.trigg.orientprompt")
    {
        signal_activate().connect(sigc::mem_fun(*this, &OrientPrompt::on_app_activate));
    }

    ~OrientPrompt() override
    {
        unset_timer();
        if (bus_watch_id > 0)
        {
            Gio::DBus::unwatch_name(bus_watch_id);
        }
    }

  protected:
    void on_app_activate()
    {
        window = new Gtk::Window();
        add_window(*window);
        window->set_child(image);

        if (!gtk_layer_is_supported())
        {
            std::cerr << "Layer shell is not supported in this compositor." << std::endl;
            std::exit(1);
        }

        window->set_size_request(100, 100);

        auto display    = window->get_display();
        auto icon_theme = Gtk::IconTheme::get_for_display(display);
        auto paintable  = icon_theme->lookup_icon("rotation-allowed-symbolic", {}, 128, 1,
            Gtk::TextDirection::LTR);
        if (paintable)
        {
            image.set_paintable(paintable);
        }

        GtkWindow *c_window = window->gobj();
        gtk_layer_init_for_window(c_window);
        gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_RIGHT, true);
        gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
        gtk_layer_set_layer(c_window, GTK_LAYER_SHELL_LAYER_OVERLAY);

        auto click = Gtk::GestureClick::create();
        click->signal_released().connect([this] (int, double, double) { make_it_so(); });
        image.add_controller(click);

        bus_watch_id = Gio::DBus::watch_name(
            Gio::DBus::BusType::SYSTEM,
            "net.hadess.SensorProxy",
            sigc::mem_fun(*this, &OrientPrompt::on_name_appeared),
            sigc::mem_fun(*this, &OrientPrompt::on_name_vanished));

        std::string hardware_orientation = get_orientation();
        if (!hardware_orientation.empty())
        {
            device_oriented(hardware_orientation);
        }
    }

    std::string get_screen()
    {
        auto display  = window->get_display();
        auto monitors = display->get_monitors();
        if (monitors->get_n_items() == 1)
        {
            auto monitor = std::dynamic_pointer_cast<Gdk::Monitor>(monitors->get_object(0));
            if (monitor)
            {
                return monitor->get_connector();
            }
        }

        return "";
    }

    std::string get_orientation()
    {
        if (!proxy)
        {
            return "";
        }

        Glib::VariantContainerBase names;
        proxy->get_cached_property(names, "AccelerometerOrientation");

        if (!names)
        {
            return "";
        }

        std::string payload_signature = names.get_type_string();
        if (payload_signature != "s")
        {
            return "";
        }

        auto str_variant = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string>>(names);
        if (str_variant)
        {
            return str_variant.get();
        }

        return "";
    }

    void make_it_so()
    {
        if (!proxy)
        {
            return;
        }

        std::string monitor = get_screen();
        if (monitor.empty())
        {
            return;
        }

        if (!is_internal_monitor(monitor))
        {
            window->hide();
            unset_timer();
            return;
        }

        std::string value = get_orientation();
        window->hide();
        unset_timer();

        std::string transform_mode = "normal";
        GtkWindow *c_window = window->gobj();

        if (value == "normal")
        {
            transform_mode = "normal";
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_RIGHT, true);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_LEFT, false);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_TOP, false);
        } else if (value == "right-up")
        {
            transform_mode = "270";
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_RIGHT, true);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_BOTTOM, false);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_LEFT, false);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_TOP, true);
        } else if (value == "left-up")
        {
            transform_mode = "90";
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_RIGHT, false);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_TOP, false);
        } else if (value == "bottom-up")
        {
            transform_mode = "180";
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_RIGHT, false);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_BOTTOM, false);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor(c_window, GTK_LAYER_SHELL_EDGE_TOP, true);
        } else
        {
            std::cerr << "Unknown orientation: " << value << std::endl;
            return;
        }

        if (output_mgr)
        {
            output_mgr->apply_transform(monitor, transform_mode);
        }
    }

    void device_oriented(const std::string& direction)
    {
        if (direction.empty())
        {
            return;
        }

        std::string monitor = get_screen();
        if (monitor.empty())
        {
            window->hide();
            std::cerr << "Orientation changed with incorrect monitor configuration" << std::endl;
            return;
        }

        if (get_current_wayland_transform_string() == direction)
        {
            window->hide();
            unset_timer();
            return;
        }

        window->show();
        set_timer();
    }

    void set_timer()
    {
        unset_timer();
        blank_timeout = Glib::signal_timeout().connect_seconds(
            sigc::mem_fun(*this, &OrientPrompt::on_timer_elapsed), 5);
    }

    void unset_timer()
    {
        if (blank_timeout)
        {
            blank_timeout.disconnect();
        }
    }

    bool on_timer_elapsed()
    {
        window->hide();
        return false;
    }

    void on_name_appeared(const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring& name,
        const Glib::ustring&)
    {
        proxy = Gio::DBus::Proxy::create_sync(
            connection,
            name,
            "/net/hadess/SensorProxy",
            "net.hadess.SensorProxy");

        if (proxy)
        {
            proxy->call_sync("ClaimAccelerometer");

            proxy->signal_properties_changed().connect([this] (const Gio::DBus::Proxy::MapChangedProperties&
                                                               properties,
                                                               const std::vector<Glib::ustring>& invalidated)
            {
                device_oriented(get_orientation());
            });

            device_oriented(get_orientation());
        }
    }

    void on_name_vanished(const Glib::RefPtr<Gio::DBus::Connection>&, const Glib::ustring&)
    {
        proxy.reset();
    }
};

int main(int argc, char *argv[])
{
    auto app = std::make_unique<OrientPrompt>();
    app->hold();
    return app->run(argc, argv);
}
