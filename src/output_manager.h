#pragma once

#include <string>
#include <vector>
#include <memory>
#include <wayland-client.h>
#include "wlr-output-management-unstable-v1-client-protocol.h"

class OutputManager
{
  public:
    OutputManager();
    ~OutputManager();

    OutputManager(const OutputManager&) = delete;
    OutputManager& operator =(const OutputManager&) = delete;

    bool apply_transform(const std::string& target_connector, const std::string& transform_mode);

    struct OutputDevice
    {
        struct zwlr_output_head_v1 *head = nullptr;
        std::string connector_name;
        uint32_t current_transform = WL_OUTPUT_TRANSFORM_NORMAL;
    };

    const std::vector<std::unique_ptr<OutputDevice>>& get_outputs() const
    {
        return outputs;
    }

    void registry_global(struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    void manager_head(struct zwlr_output_head_v1 *head);
    void head_name(struct zwlr_output_head_v1 *head, const char *name);
    void head_transform(struct zwlr_output_head_v1 *head, uint32_t transform);
    void manager_done(uint32_t serial);

  private:
    struct wl_display *display   = nullptr;
    struct wl_registry *registry = nullptr;
    struct zwlr_output_manager_v1 *wlr_manager = nullptr;
    uint32_t current_serial = 0;
    std::vector<std::unique_ptr<OutputDevice>> outputs;

    void init_wayland_connection();
    uint32_t parse_transform_string(const std::string& mode);
};
