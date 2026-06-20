#include "output_manager.h"
#include <gdk/wayland/gdkwayland.h>
#include <gtkmm.h>
#include <iostream>
#include <cstring>
#include "wlr-output-management-unstable-v1-client-protocol.h"
static void registry_handle_global(void *data, struct wl_registry *reg, uint32_t id, const char *intf,
    uint32_t ver)
{
    static_cast<OutputManager*>(data)->registry_global(reg, id, intf, ver);
}

static void registry_handle_global_remove(void*, struct wl_registry*, uint32_t)
{}
static const struct wl_registry_listener registry_listener = {
    registry_handle_global, registry_handle_global_remove
};

static void head_handle_name(void *data, struct zwlr_output_head_v1 *head, const char *name)
{
    static_cast<OutputManager*>(data)->head_name(head, name);
}

static void head_handle_transform(void *data,
    struct zwlr_output_head_v1 *head,
    int32_t transform)
{
    static_cast<OutputManager*>(data)->head_transform(head, transform);
}

static void head_handle_description(void*, struct zwlr_output_head_v1*, const char*)
{}
static void head_handle_logical_position(void*, struct zwlr_output_head_v1*, int32_t, int32_t)
{}
static void head_handle_mode(void*, struct zwlr_output_head_v1*, struct zwlr_output_mode_v1*)
{}
static void head_handle_enabled(void*, struct zwlr_output_head_v1*, int32_t)
{}
static void head_handle_current_mode(void*, struct zwlr_output_head_v1*, struct zwlr_output_mode_v1*)
{}
static void head_handle_position(void*, struct zwlr_output_head_v1*, int32_t, int32_t)
{}
static void head_handle_scale(void*, struct zwlr_output_head_v1*, wl_fixed_t)
{}
static void head_handle_finished(void*, struct zwlr_output_head_v1*)
{}
static void head_handle_make(void *data,
    struct zwlr_output_head_v1 *zwlr_output_head_v1,
    const char *make)
{}
static void head_handle_model(void *data,
    struct zwlr_output_head_v1 *zwlr_output_head_v1,
    const char *model)
{}
static void head_handle_serial_number(void *data,
    struct zwlr_output_head_v1 *zwlr_output_head_v1,
    const char *serial_number)
{}
static void head_handle_adaptive_sync(void *data,
    struct zwlr_output_head_v1 *zwlr_output_head_v1,
    uint32_t state)
{}

static const struct zwlr_output_head_v1_listener head_listener = {
    head_handle_name, head_handle_description, head_handle_logical_position, head_handle_mode,
    head_handle_enabled, head_handle_current_mode, head_handle_position,
    head_handle_transform, head_handle_scale, head_handle_finished, head_handle_make,
    head_handle_model, head_handle_serial_number, head_handle_adaptive_sync
};

static void manager_handle_head(void *data, struct zwlr_output_manager_v1*, struct zwlr_output_head_v1 *head)
{
    static_cast<OutputManager*>(data)->manager_head(head);
}

static void manager_handle_done(void *data, struct zwlr_output_manager_v1*, uint32_t serial)
{
    static_cast<OutputManager*>(data)->manager_done(serial);
}

static void manager_handle_finished(void*, struct zwlr_output_manager_v1*)
{}
static const struct zwlr_output_manager_v1_listener manager_listener = {
    manager_handle_head, manager_handle_done, manager_handle_finished
};

OutputManager::OutputManager()
{
    init_wayland_connection();
}

OutputManager::~OutputManager()
{
    outputs.clear();
    if (wlr_manager)
    {
        zwlr_output_manager_v1_destroy(wlr_manager);
    }

    if (registry)
    {
        wl_registry_destroy(registry);
    }
}

void OutputManager::init_wayland_connection()
{
    auto gdk_display = Gdk::Display::get_default();

    display  = gdk_wayland_display_get_wl_display(gdk_display->gobj());
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);

    wl_display_roundtrip(display);
}

void OutputManager::registry_global(struct wl_registry *reg, uint32_t id, const char *interface,
    uint32_t version)
{
    if (std::strcmp(interface, zwlr_output_manager_v1_interface.name) == 0)
    {
        wlr_manager =
            static_cast<zwlr_output_manager_v1*>(wl_registry_bind(reg, id, &zwlr_output_manager_v1_interface,
                std::min(version, 1u)));
        zwlr_output_manager_v1_add_listener(wlr_manager, &manager_listener, this);
    }
}

void OutputManager::manager_head(struct zwlr_output_head_v1 *head)
{
    auto device = std::make_unique<OutputDevice>();
    device->head = head;
    outputs.push_back(std::move(device));
    zwlr_output_head_v1_add_listener(head, &head_listener, this);
}

void OutputManager::head_name(struct zwlr_output_head_v1 *head, const char *name)
{
    for (auto& out : outputs)
    {
        if (out->head == head)
        {
            out->connector_name = name;
            break;
        }
    }
}

void OutputManager::head_transform(struct zwlr_output_head_v1 *head, uint32_t transform)
{
    for (auto& out : outputs)
    {
        if (out->head == head)
        {
            out->current_transform = transform;
            break;
        }
    }
}

uint32_t OutputManager::parse_transform_string(const std::string& mode)
{
    if (mode == "90")
    {
        return WL_OUTPUT_TRANSFORM_90;
    }

    if (mode == "180")
    {
        return WL_OUTPUT_TRANSFORM_180;
    }

    if (mode == "270")
    {
        return WL_OUTPUT_TRANSFORM_270;
    }

    return WL_OUTPUT_TRANSFORM_NORMAL;
}

static void config_handle_succeeded(void*, struct zwlr_output_configuration_v1 *config)
{
    zwlr_output_configuration_v1_destroy(config);
}

static void config_handle_failed(void*, struct zwlr_output_configuration_v1 *config)
{
    zwlr_output_configuration_v1_destroy(config);
}

static void config_handle_cancelled(void*, struct zwlr_output_configuration_v1 *config)
{
    zwlr_output_configuration_v1_destroy(config);
}

static const struct zwlr_output_configuration_v1_listener config_listener = {
    .succeeded = config_handle_succeeded,
    .failed    = config_handle_failed,
    .cancelled = config_handle_cancelled
};

void OutputManager::manager_done(uint32_t serial)
{
    this->current_serial = serial;
}

bool OutputManager::apply_transform(const std::string& target_connector, const std::string& transform_mode)
{
    if (!wlr_manager)
    {
        return false;
    }

    wl_display_roundtrip(display);

    OutputDevice *target = nullptr;
    for (auto& out : outputs)
    {
        if (out->connector_name == target_connector)
        {
            target = out.get();
            break;
        }
    }

    if (!target)
    {
        return false;
    }

    if (!target->head)
    {
        return false;
    }

    uint32_t target_rotation = parse_transform_string(transform_mode);

    struct zwlr_output_configuration_v1 *config = zwlr_output_manager_v1_create_configuration(wlr_manager,
        this->current_serial);
    if (!config)
    {
        return false;
    }

    zwlr_output_configuration_v1_add_listener(config, &config_listener, this);

    struct zwlr_output_configuration_head_v1 *config_head = zwlr_output_configuration_v1_enable_head(config,
        target->head);
    if (!config_head)
    {
        zwlr_output_configuration_v1_destroy(config);
        return false;
    }

    zwlr_output_configuration_head_v1_set_transform(config_head, target_rotation);

    zwlr_output_configuration_v1_apply(config);

    wl_display_flush(display);
    return true;
}
