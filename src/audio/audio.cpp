#include "audio.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>

static const struct pw_registry_events registry_events = {
    /* version */ PW_VERSION_REGISTRY_EVENTS,
    /* global */ AudioManagerClient::registryEventGlobal,
    /* global_remove */ AudioManagerClient::registryEventGlobalRemove,
};

AudioManagerClient::AudioManagerClient()
    : loop(nullptr)
    , context(nullptr)
    , core(nullptr)
    , registry(nullptr)
    , default_sink_id(0)
    , default_source_id(0)
    , initialized(false) {

    pw_init(nullptr, nullptr);

    loop = pw_thread_loop_new("audio-manager", nullptr);
    if (!loop) {
        std::cerr << "Failed to create thread loop" << std::endl;
        return;
    }

    context = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
    if (!context) {
        std::cerr << "Failed to create context" << std::endl;
        return;
    }

    if (pw_thread_loop_start(loop) < 0) {
        std::cerr << "Failed to start thread loop" << std::endl;
        return;
    }

    pw_thread_loop_lock(loop);

    core = pw_context_connect(context, nullptr, 0);
    if (!core) {
        std::cerr << "Failed to connect to PipeWire" << std::endl;
        pw_thread_loop_unlock(loop);
        return;
    }

    registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(registry, &registry_listener, &registry_events, this);

    pw_thread_loop_unlock(loop);

    // Wait a bit for initial enumeration
    usleep(100000);

    initialized = true;
}

AudioManagerClient::~AudioManagerClient() {
    if (loop) {
        pw_thread_loop_stop(loop);
    }

    if (registry) {
        spa_hook_remove(&registry_listener);
        pw_proxy_destroy((struct pw_proxy*)registry);
    }

    if (core) {
        pw_core_disconnect(core);
    }

    if (context) {
        pw_context_destroy(context);
    }

    if (loop) {
        pw_thread_loop_destroy(loop);
    }

    pw_deinit();
}

void AudioManagerClient::registryEventGlobal(
    void* data, uint32_t id, uint32_t permissions, const char* type, uint32_t version, const struct spa_dict* props) {
    auto* self = static_cast<AudioManagerClient*>(data);

    if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        const char* media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        if (!media_class)
            return;

        AudioDevice dev;
        dev.id = id;
        dev.isDefault = false;

        const char* node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        dev.name = node_name ? node_name : "";

        const char* node_desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
        const char* node_nick = spa_dict_lookup(props, PW_KEY_NODE_NICK);
        dev.description = node_desc ? node_desc : (node_nick ? node_nick : dev.name);

        if (strcmp(media_class, "Audio/Sink") == 0) {
            self->sinks[id] = dev;
        } else if (strcmp(media_class, "Audio/Source") == 0) {
            self->sources[id] = dev;
        }
    }
}

void AudioManagerClient::registryEventGlobalRemove(void* data, uint32_t id) {
    auto* self = static_cast<AudioManagerClient*>(data);
    self->sinks.erase(id);
    self->sources.erase(id);
}

void AudioManagerClient::updateDevices() {
    if (!initialized)
        return;

    // trigger roundtrip to ensure we have latest data
    pw_thread_loop_lock(loop);
    pw_core_sync(core, 0, 0);
    pw_thread_loop_unlock(loop);

    usleep(50000); // Give time for updates
}

std::vector<AudioDevice> AudioManagerClient::getDevices(const std::map<uint32_t, AudioDevice>& deviceMap) {
    std::vector<AudioDevice> devices;

    for (const auto& [id, dev] : deviceMap) {
        AudioDevice device = dev;
        device.isDefault = (id == default_sink_id) || (id == default_source_id);
        devices.push_back(device);
    }

    return devices;
}

std::vector<AudioDevice> AudioManagerClient::listInputDevices() {
    updateDevices();
    return getDevices(sources);
}

std::vector<AudioDevice> AudioManagerClient::listOutputDevices() {
    updateDevices();
    return getDevices(sinks);
}

bool AudioManagerClient::setDefault(uint32_t deviceId, const std::string& key) {
    if (!initialized) {
        std::cerr << "AudioManager not initialized" << std::endl;
        return false;
    }

    // find device name
    std::string deviceName;
    auto& deviceMap = (key == "default.audio.sink") ? sinks : sources;

    auto it = deviceMap.find(deviceId);
    if (it == deviceMap.end()) {
        std::cerr << "Device ID " << deviceId << " not found" << std::endl;
        return false;
    }

    deviceName = it->second.name;

    // Use pw-metadata command to set default
    // This is the most reliable way that works with all PipeWire versions
    std::string value = "{\\\"name\\\":\\\"" + deviceName + "\\\"}";
    std::string cmd = "pw-metadata -n settings 0 " + key + " Spa:String:JSON '" + value + "'";

    int result = system(cmd.c_str());

    if (result == 0) {
        std::cout << "Set " << key << " to device " << deviceId << " (" << deviceName << ")" << std::endl;

        if (key == "default.audio.sink") {
            default_sink_id = deviceId;
        } else {
            default_source_id = deviceId;
        }

        return true;
    } else {
        std::cerr << "Failed to set default device" << std::endl;
        return false;
    }
}

bool AudioManagerClient::setDefaultInput(uint32_t deviceId) { return setDefault(deviceId, "default.audio.source"); }

bool AudioManagerClient::setDefaultOutput(uint32_t deviceId) { return setDefault(deviceId, "default.audio.sink"); }
