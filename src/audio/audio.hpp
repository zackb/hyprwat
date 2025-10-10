#pragma once

#include <map>
#include <pipewire/extensions/metadata.h>
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

    // List audio devices
    std::vector<AudioDevice> listInputDevices();
    std::vector<AudioDevice> listOutputDevices();

    // Set default devices
    bool setDefaultInput(uint32_t deviceId);
    bool setDefaultOutput(uint32_t deviceId);

private:
    struct pw_thread_loop* loop;
    struct pw_context* context;
    struct pw_core* core;
    struct pw_registry* registry;
    struct spa_hook registry_listener;

    struct pw_proxy* metadata_proxy;
    struct pw_metadata* metadata;
    struct spa_hook metadata_listener;

    std::map<uint32_t, AudioDevice> sinks;
    std::map<uint32_t, AudioDevice> sources;
    uint32_t default_sink_id;
    uint32_t default_source_id;

    bool initialized;

    void updateDevices();
    std::vector<AudioDevice> getDevices(const std::map<uint32_t, AudioDevice>& deviceMap);
    bool setDefault(uint32_t deviceId, const std::string& key);

public:
    // Static callbacks need to be public for C-style callback registration
    static void registryEventGlobal(void* data,
                                    uint32_t id,
                                    uint32_t permissions,
                                    const char* type,
                                    uint32_t version,
                                    const struct spa_dict* props);
    static void registryEventGlobalRemove(void* data, uint32_t id);
    static int metadataProperty(void* data, uint32_t id, const char* key, const char* type, const char* value);
};
