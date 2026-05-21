#include "audio.hpp"
#include "../debug/log.hpp"
#include <cstring>
#include <spa/utils/json.h>
#include <spa/pod/iter.h>
#include <spa/pod/builder.h>
#include <spa/param/props.h>
#include <mutex>
#include <condition_variable>
#include <chrono>

static const struct pw_registry_events registry_events = {
    /* version */ PW_VERSION_REGISTRY_EVENTS,
    /* global */ AudioManagerClient::registryEventGlobal,
    /* global_remove */ AudioManagerClient::registryEventGlobalRemove,
};

static const struct pw_metadata_events metadata_events = {
    /* version */ PW_VERSION_METADATA_EVENTS,
    /* property */ AudioManagerClient::metadataProperty,
};

AudioManagerClient::AudioManagerClient()
    : loop(nullptr)
    , context(nullptr)
    , core(nullptr)
    , registry(nullptr)
    , metadata_proxy(nullptr)
    , metadata(nullptr)
    , default_sink_id(0)
    , default_source_id(0)
    , initialized(false) {

    pw_init(nullptr, nullptr);

    loop = pw_thread_loop_new("audio-manager", nullptr);
    if (!loop) {
        debug::log(ERR, "Failed to create thread loop");
        return;
    }

    context = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
    if (!context) {
        debug::log(ERR, "Failed to create context");
        return;
    }

    if (pw_thread_loop_start(loop) < 0) {
        debug::log(ERR, "Failed to start thread loop");
        return;
    }

    pw_thread_loop_lock(loop);

    core = pw_context_connect(context, nullptr, 0);
    if (!core) {
        debug::log(ERR, "Failed to connect to PipeWire core");
        pw_thread_loop_unlock(loop);
        return;
    }

    registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(registry, &registry_listener, &registry_events, this);

    pw_thread_loop_unlock(loop);

    // TODO: can we not do this??
    usleep(100000);

    initialized = true;
}

AudioManagerClient::~AudioManagerClient() {
    if (loop) {
        pw_thread_loop_stop(loop);
    }

    if (metadata_proxy) {
        spa_hook_remove(&metadata_listener);
        pw_proxy_destroy(metadata_proxy);
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
    } else if (strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0) {
        const char* metadata_name = spa_dict_lookup(props, PW_KEY_METADATA_NAME);
        if (metadata_name && strcmp(metadata_name, "default") == 0) {
            pw_thread_loop_lock(self->loop);
            self->metadata_proxy = (struct pw_proxy*)pw_registry_bind(self->registry, id, type, PW_VERSION_METADATA, 0);
            if (self->metadata_proxy) {
                self->metadata = (struct pw_metadata*)self->metadata_proxy;
                pw_metadata_add_listener(self->metadata, &self->metadata_listener, &metadata_events, self);
            }
            pw_thread_loop_unlock(self->loop);
        }
    }
}

void AudioManagerClient::registryEventGlobalRemove(void* data, uint32_t id) {
    auto* self = static_cast<AudioManagerClient*>(data);
    self->sinks.erase(id);
    self->sources.erase(id);
}

int AudioManagerClient::metadataProperty(
    void* data, uint32_t id, const char* key, const char* type, const char* value) {
    auto* self = static_cast<AudioManagerClient*>(data);

    if (!key)
        return 0;

    if (strcmp(key, "default.audio.sink") == 0 && value) {
        struct spa_json it[2];
        char key_buf[64];
        char name_buf[256];

        spa_json_init(&it[0], value, strlen(value));
        if (spa_json_enter_object(&it[0], &it[1]) > 0) {
            while (spa_json_get_string(&it[1], key_buf, sizeof(key_buf)) > 0) {
                if (strcmp(key_buf, "name") == 0) {
                    if (spa_json_get_string(&it[1], name_buf, sizeof(name_buf)) > 0) {
                        std::string name(name_buf);
                        for (auto& [id, dev] : self->sinks) {
                            if (dev.name == name) {
                                self->default_sink_id = id;
                                break;
                            }
                        }
                    }
                    break;
                } else {
                    spa_json_next(&it[1], nullptr);
                }
            }
        }
    } else if (strcmp(key, "default.audio.source") == 0 && value) {
        struct spa_json it[2];
        char key_buf[64];
        char name_buf[256];

        spa_json_init(&it[0], value, strlen(value));
        if (spa_json_enter_object(&it[0], &it[1]) > 0) {
            while (spa_json_get_string(&it[1], key_buf, sizeof(key_buf)) > 0) {
                if (strcmp(key_buf, "name") == 0) {
                    if (spa_json_get_string(&it[1], name_buf, sizeof(name_buf)) > 0) {
                        std::string name(name_buf);
                        for (auto& [id, dev] : self->sources) {
                            if (dev.name == name) {
                                self->default_source_id = id;
                                break;
                            }
                        }
                    }
                    break;
                } else {
                    spa_json_next(&it[1], nullptr);
                }
            }
        }
    }

    return 0;
}

void AudioManagerClient::updateDevices() {
    if (!initialized)
        return;

    // trigger roundtrip to ensure we have latest data
    pw_thread_loop_lock(loop);
    pw_core_sync(core, 0, 0);
    pw_thread_loop_unlock(loop);

    // TODO: can we not do this??
    usleep(50000);
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
    if (!initialized || !metadata) {
        debug::log(ERR, "AudioManager not initialized or metadata not available");
        return false;
    }

    // find device name
    std::string deviceName;
    auto& deviceMap = (key == "default.audio.sink") ? sinks : sources;

    auto it = deviceMap.find(deviceId);
    if (it == deviceMap.end()) {
        debug::log(ERR, "Device ID {} not found", deviceId);
        return false;
    }

    deviceName = it->second.name;

    std::string value = "{\"name\":\"" + deviceName + "\"}";

    pw_thread_loop_lock(loop);
    int res = pw_metadata_set_property(metadata, PW_ID_CORE, key.c_str(), "Spa:String:JSON", value.c_str());
    pw_thread_loop_unlock(loop);

    if (res < 0) {
        debug::log(ERR, "Failed to set default {}: {}", key, strerror(-res));
        return false;
    }

    debug::log(INFO, "Set {} to device {} ({})", key, deviceId, deviceName);

    if (key == "default.audio.sink") {
        default_sink_id = deviceId;
    } else {
        default_source_id = deviceId;
    }

    return true;
}

bool AudioManagerClient::setDefaultInput(uint32_t deviceId) { return setDefault(deviceId, "default.audio.source"); }

bool AudioManagerClient::setDefaultOutput(uint32_t deviceId) { return setDefault(deviceId, "default.audio.sink"); }

AudioManagerClient::VolumeInfo AudioManagerClient::getVolume() {
    VolumeInfo info;
    if (!initialized || default_sink_id == 0) {
        return info;
    }

    struct VolumeState {
        float volume = 1.0f;
        bool mute = false;
        uint32_t channels = 2;
        bool done = false;
        std::mutex mutex;
        std::condition_variable cv;
    };

    auto state = std::make_shared<VolumeState>();

    pw_thread_loop_lock(loop);

    struct pw_node* node = (struct pw_node*)pw_registry_bind(registry, default_sink_id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
    if (!node) {
        pw_thread_loop_unlock(loop);
        return info;
    }

    struct spa_hook listener;
    
    auto node_event_param = [](void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param) {
        auto* st = static_cast<VolumeState*>(data);
        if (id == SPA_PARAM_Props) {
            const struct spa_pod_prop *prop = spa_pod_find_prop(param, nullptr, SPA_PROP_channelVolumes);
            if (prop && spa_pod_is_array(&prop->value)) {
                uint32_t n_values = 0;
                const void* val_data = spa_pod_get_array(&prop->value, &n_values);
                if (val_data && n_values > 0) {
                    const float* vols = (const float*)val_data;
                    st->channels = n_values;
                    float sum = 0.0f;
                    for (uint32_t i = 0; i < n_values; i++) {
                        sum += vols[i];
                    }
                    st->volume = sum / n_values;
                }
            }
            const struct spa_pod_prop *mute_prop = spa_pod_find_prop(param, nullptr, SPA_PROP_mute);
            if (mute_prop && spa_pod_is_bool(&mute_prop->value)) {
                bool m = false;
                if (spa_pod_get_bool(&mute_prop->value, &m) == 0) {
                    st->mute = m;
                }
            }
            {
                std::lock_guard<std::mutex> lock(st->mutex);
                st->done = true;
            }
            st->cv.notify_one();
        }
    };

    static const struct pw_node_events node_events = {
        .version = PW_VERSION_NODE_EVENTS,
        .param = node_event_param,
    };

    pw_node_add_listener(node, &listener, &node_events, state.get());
    pw_node_enum_params(node, 0, SPA_PARAM_Props, 0, 0, nullptr);

    pw_thread_loop_unlock(loop);

    // Wait for the result with timeout
    {
        std::unique_lock<std::mutex> lock(state->mutex);
        state->cv.wait_for(lock, std::chrono::milliseconds(200), [&]{ return state->done; });
    }

    pw_thread_loop_lock(loop);
    spa_hook_remove(&listener);
    pw_proxy_destroy((struct pw_proxy*)node);
    
    // Store channels for subsequent volume setting
    default_sink_channels = state->channels;

    pw_thread_loop_unlock(loop);

    info.volume = state->volume;
    info.mute = state->mute;
    return info;
}

bool AudioManagerClient::setVolume(float volume) {
    if (!initialized || default_sink_id == 0) {
        return false;
    }

    std::vector<float> vols(default_sink_channels, volume);

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
        SPA_PROP_channelVolumes, SPA_POD_Array(sizeof(float), SPA_TYPE_Float, vols.size(), vols.data()),
        SPA_PROP_mute, SPA_POD_Bool(false)
    );

    pw_thread_loop_lock(loop);

    struct pw_node* node = (struct pw_node*)pw_registry_bind(registry, default_sink_id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
    if (node) {
        pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        pw_core_sync(core, 0, 0);
        pw_proxy_destroy((struct pw_proxy*)node);
    }

    pw_thread_loop_unlock(loop);
    return true;
}

bool AudioManagerClient::setMute(bool mute) {
    if (!initialized || default_sink_id == 0) {
        return false;
    }

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
        SPA_PROP_mute, SPA_POD_Bool(mute)
    );

    pw_thread_loop_lock(loop);

    struct pw_node* node = (struct pw_node*)pw_registry_bind(registry, default_sink_id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
    if (node) {
        pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        pw_core_sync(core, 0, 0);
        pw_proxy_destroy((struct pw_proxy*)node);
    }

    pw_thread_loop_unlock(loop);
    return true;
}
