#pragma once

#include <map>
#include <pipewire/pipewire.h>
#include <spa/debug/types.h>
#include <spa/param/audio/format-utils.h>
#include <string>
#include <vector>

struct AudioDevice {
    std::string name;
    std::string description;
    uint32_t id;
    bool isDefault;
};

class AudioManagerClient {
public:
    AudioManagerClient();
    ~AudioManagerClient();

    // list audio devices
    std::vector<AudioDevice> listInputDevices();
    std::vector<AudioDevice> listOutputDevices();

    // set default devices
    bool setDefaultInput(uint32_t deviceId);
    bool setDefaultOutput(uint32_t deviceId);

private:
    struct pw_thread_loop* loop;
    struct pw_context* context;
    struct pw_core* core;
    struct pw_registry* registry;
    struct spa_hook registry_listener;

    std::map<uint32_t, AudioDevice> sinks;
    std::map<uint32_t, AudioDevice> sources;
    uint32_t default_sink_id;
    uint32_t default_source_id;

    bool initialized;

    void updateDevices();
    std::vector<AudioDevice> getDevices(const std::map<uint32_t, AudioDevice>& deviceMap);
    bool setDefault(uint32_t deviceId, const std::string& key);

public:
    // static callbacks need for registration
    static void registryEventGlobal(void* data,
                                    uint32_t id,
                                    uint32_t permissions,
                                    const char* type,
                                    uint32_t version,
                                    const struct spa_dict* props);
    static void registryEventGlobalRemove(void* data, uint32_t id);
};
