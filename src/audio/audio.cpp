#include "audio.hpp"
#include "../debug/log.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <spa/param/param.h>
#include <spa/param/props.h>
#include <spa/param/route.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>
#include <spa/utils/json.h>
#include <unistd.h>

const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    /* global */ AudioManagerClient::registryEventGlobal,
    /* global_remove */ AudioManagerClient::registryEventGlobalRemove,
};

static const struct pw_metadata_events metadata_events = {
    PW_VERSION_METADATA_EVENTS,
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
    , initialized(false)
    , active_sink_node(nullptr)
    , active_device_proxy(nullptr)
    , active_device_id(0)
    , active_route_index(UINT32_MAX)
    , active_route_device(0)
    , active_sink_channels(2)
    , cached_volume(-1.0f)
    , cached_mute(false) {
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

    usleep(150000);
    initialized = true;
}

AudioManagerClient::~AudioManagerClient() {
    if (loop) {
        pw_thread_loop_stop(loop);
    }

    if (active_sink_node) {
        spa_hook_remove(&sink_node_listener);
        pw_proxy_destroy((struct pw_proxy*)active_sink_node);
    }
    if (active_device_proxy) {
        spa_hook_remove(&active_device_listener);
        pw_proxy_destroy((struct pw_proxy*)active_device_proxy);
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

void AudioManagerClient::teardownActiveDevice() {
    if (active_device_proxy) {
        spa_hook_remove(&active_device_listener);
        pw_proxy_destroy((struct pw_proxy*)active_device_proxy);
        active_device_proxy = nullptr;
    }
    active_device_id = 0;
    active_route_index = UINT32_MAX; // sentinel, not 0
    active_route_device = 0;
}

void AudioManagerClient::updateActiveSinkNode() {
    pw_thread_loop_lock(loop);

    debug::log(
        TRACE, "updateActiveSinkNode: default_sink_id={} default_sink_name={}", default_sink_id, default_sink_name);

    auto dbg = sinks.find(default_sink_id);
    if (dbg != sinks.end())
        debug::log(TRACE,
                   "  sink node name={} deviceId={} profileDevice={}",
                   dbg->second.name,
                   dbg->second.deviceId,
                   dbg->second.profileDevice);
    else
        debug::log(TRACE, "  sink id NOT FOUND in sinks map");

    // tear down old node and device
    if (active_sink_node) {
        spa_hook_remove(&sink_node_listener);
        pw_proxy_destroy((struct pw_proxy*)active_sink_node);
        active_sink_node = nullptr;
    }
    teardownActiveDevice();

    if (default_sink_id == 0) {
        pw_thread_loop_unlock(loop);
        return;
    }

    // bind the sink node and listen for Props
    active_sink_node =
        (struct pw_node*)pw_registry_bind(registry, default_sink_id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);

    if (active_sink_node) {
        static const struct pw_node_events node_events = {
            .version = PW_VERSION_NODE_EVENTS,
            .param = [](void* data, int, uint32_t id, uint32_t, uint32_t, const struct spa_pod* param) {
                auto* self = static_cast<AudioManagerClient*>(data);
                // we only asked for SPA_PARAM_Props ignore anything else
                if (id != SPA_PARAM_Props) {
                    return;
                }

                float volume = -1.0f;
                bool mute = false;
                bool found_vol = false;
                bool found_mute = false;
                uint32_t channels = 2;

                struct spa_pod_prop* p = nullptr;
                SPA_POD_OBJECT_FOREACH((const struct spa_pod_object*)param, p) {
                    if (p->key == SPA_PROP_channelVolumes) {
                        uint32_t n = 0;
                        const void* vd = spa_pod_get_array(&p->value, &n);
                        if (vd && n > 0) {
                            const float* v = (const float*)vd;
                            channels = n;
                            float sum = 0.0f;
                            for (uint32_t i = 0; i < n; i++)
                                sum += v[i];
                            volume = sum / n;
                            found_vol = true;
                        }
                    } else if (p->key == SPA_PROP_mute) {
                        bool m = false;
                        if (spa_pod_get_bool(&p->value, &m) == 0) {
                            mute = m;
                            found_mute = true;
                        }
                    }
                }

                if (found_vol || found_mute) {
                    std::lock_guard<std::mutex> lock(self->volume_mutex);
                    if (found_vol) {
                        self->cached_volume = std::pow(volume, 1.0f / 3.0f);
                        self->active_sink_channels = channels;
                    }
                    if (found_mute) {
                        self->cached_mute = mute;
                    }
                }
            }};
        pw_node_add_listener(active_sink_node, &sink_node_listener, &node_events, this);
        pw_node_enum_params(active_sink_node, 0, SPA_PARAM_Props, 0, 0, nullptr);
    }

    // bind the device (card) and enumerate its routes
    auto it = sinks.find(default_sink_id);
    if (it != sinks.end() && it->second.deviceId != 0) {
        active_device_id = it->second.deviceId;

        active_device_proxy = (struct pw_device*)pw_registry_bind(
            registry, active_device_id, PW_TYPE_INTERFACE_Device, PW_VERSION_DEVICE, 0);

        if (active_device_proxy) {
            static const struct pw_device_events device_events = {
                .version = PW_VERSION_DEVICE_EVENTS,
                .param = [](void* data, int, uint32_t id, uint32_t, uint32_t, const struct spa_pod* param) {
                    auto* self = static_cast<AudioManagerClient*>(data);
                    if (id != SPA_PARAM_Route) {
                        return;
                    }

                    uint32_t route_index = 0;
                    uint32_t route_device = UINT32_MAX;
                    uint32_t route_direction = UINT32_MAX;

                    struct spa_pod_prop* p = nullptr;
                    SPA_POD_OBJECT_FOREACH((const struct spa_pod_object*)param, p) {
                        switch (p->key) {
                        case SPA_PARAM_ROUTE_index: {
                            int32_t v = 0;
                            if (spa_pod_get_int(&p->value, &v) == 0)
                                route_index = (uint32_t)v;
                            break;
                        }
                        case SPA_PARAM_ROUTE_device: {
                            int32_t v = 0;
                            if (spa_pod_get_int(&p->value, &v) == 0)
                                route_device = (uint32_t)v;
                            break;
                        }
                        case SPA_PARAM_ROUTE_direction: {
                            uint32_t v = UINT32_MAX;
                            if (spa_pod_get_id(&p->value, &v) == 0)
                                route_direction = v;
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    debug::log(TRACE,
                               "Route callback: route_index={} route_device={} direction={}",
                               route_index,
                               route_device,
                               route_direction);

                    // take the first output-direction route
                    if (route_direction == SPA_DIRECTION_OUTPUT) {
                        std::lock_guard<std::mutex> lock(self->volume_mutex);
                        if (self->active_route_index == UINT32_MAX) {
                            self->active_route_index = route_index;
                            self->active_route_device = route_device;
                            debug::log(TRACE, "Adopted route: index={} device={}", route_index, route_device);
                        }
                    }
                }};

            pw_device_add_listener(active_device_proxy, &active_device_listener, &device_events, this);
            pw_device_enum_params((struct pw_device*)active_device_proxy, 0, SPA_PARAM_Route, 0, 0, nullptr);
        }
    }

    pw_thread_loop_unlock(loop);
}

void AudioManagerClient::registryEventGlobal(void* data,
                                             uint32_t id,
                                             uint32_t /*permissions*/,
                                             const char* type,
                                             uint32_t /*version*/,
                                             const struct spa_dict* props) {
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

        const char* device_id_str = spa_dict_lookup(props, "device.id");
        if (device_id_str)
            dev.deviceId = std::stoul(device_id_str);

        const char* profile_device_str = spa_dict_lookup(props, "card.profile.device");
        if (profile_device_str)
            dev.profileDevice = std::stoul(profile_device_str);

        if (strcmp(media_class, "Audio/Sink") == 0) {
            self->sinks[id] = dev;
            if (dev.name == self->default_sink_name) {
                self->default_sink_id = id;
                self->updateActiveSinkNode();
            }
        } else if (strcmp(media_class, "Audio/Source") == 0) {
            self->sources[id] = dev;
            if (dev.name == self->default_source_name)
                self->default_source_id = id;
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
    if (self->default_sink_id == id) {
        self->default_sink_id = 0;
        self->updateActiveSinkNode();
    }
}

int AudioManagerClient::metadataProperty(
    void* data, uint32_t /*id*/, const char* key, const char* /*type*/, const char* value) {
    auto* self = static_cast<AudioManagerClient*>(data);
    if (!key)
        return 0;

    auto parseName = [](const char* json, std::string& out) -> bool {
        struct spa_json it[2];
        char key_buf[64], name_buf[256];
        spa_json_init(&it[0], json, strlen(json));
        if (spa_json_enter_object(&it[0], &it[1]) <= 0)
            return false;
        while (spa_json_get_string(&it[1], key_buf, sizeof(key_buf)) > 0) {
            if (strcmp(key_buf, "name") == 0 && spa_json_get_string(&it[1], name_buf, sizeof(name_buf)) > 0) {
                out = name_buf;
                return true;
            }
            spa_json_next(&it[1], nullptr);
        }
        return false;
    };

    if (strcmp(key, "default.audio.sink") == 0 && value) {
        std::string name;
        if (!parseName(value, name))
            return 0;
        self->default_sink_name = name;
        self->default_sink_id = 0;
        for (auto& [sid, dev] : self->sinks) {
            if (dev.name == name) {
                self->default_sink_id = sid;
                self->updateActiveSinkNode();
                break;
            }
        }
    } else if (strcmp(key, "default.audio.source") == 0 && value) {
        std::string name;
        if (!parseName(value, name))
            return 0;
        self->default_source_name = name;
        self->default_source_id = 0;
        for (auto& [sid, dev] : self->sources) {
            if (dev.name == name) {
                self->default_source_id = sid;
                break;
            }
        }
    }
    return 0;
}

bool AudioManagerClient::setVolume(float volume) {
    pw_thread_loop_lock(loop);
    if (!active_sink_node) {
        pw_thread_loop_unlock(loop);
        return false;
    }

    float cubic_volume = std::pow(std::max(0.0f, std::min(volume, 1.0f)), 3.0f);

    uint32_t channels, ridx, rdev;
    bool cur_mute;
    {
        std::lock_guard<std::mutex> lock(volume_mutex);
        channels = active_sink_channels;
        cur_mute = cached_mute;
        ridx = active_route_index;
        rdev = active_route_device;
    }

    debug::log(TRACE,
               "setVolume: active_device_id={} active_route_index={} active_route_device={} channels={}",
               active_device_id,
               ridx,
               rdev,
               channels);

    std::vector<float> vols(channels, cubic_volume);

    if (active_device_id != 0) {
        uint8_t props_buf[512];
        struct spa_pod_builder props_b = SPA_POD_BUILDER_INIT(props_buf, sizeof(props_buf));
        struct spa_pod* props_pod = (struct spa_pod*)spa_pod_builder_add_object(
            &props_b,
            SPA_TYPE_OBJECT_Props,
            SPA_PARAM_Route,
            SPA_PROP_channelVolumes,
            SPA_POD_Array(sizeof(float), SPA_TYPE_Float, vols.size(), vols.data()),
            SPA_PROP_mute,
            SPA_POD_Bool(cur_mute));

        uint8_t route_buf[1024];
        struct spa_pod_builder route_b = SPA_POD_BUILDER_INIT(route_buf, sizeof(route_buf));
        struct spa_pod* route_pod = (struct spa_pod*)spa_pod_builder_add_object(&route_b,
                                                                                SPA_TYPE_OBJECT_ParamRoute,
                                                                                SPA_PARAM_Route,
                                                                                SPA_PARAM_ROUTE_index,
                                                                                SPA_POD_Int(ridx),
                                                                                SPA_PARAM_ROUTE_device,
                                                                                SPA_POD_Int(rdev),
                                                                                SPA_PARAM_ROUTE_props,
                                                                                SPA_POD_Pod(props_pod),
                                                                                SPA_PARAM_ROUTE_save,
                                                                                SPA_POD_Bool(true));

        pw_device_set_param((struct pw_device*)active_device_proxy, SPA_PARAM_Route, 0, route_pod);
        pw_core_sync(core, 0, 0);
    } else {
        uint8_t buf[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
        struct spa_pod* param = (struct spa_pod*)spa_pod_builder_add_object(
            &b,
            SPA_TYPE_OBJECT_Props,
            SPA_PARAM_Props,
            SPA_PROP_channelVolumes,
            SPA_POD_Array(sizeof(float), SPA_TYPE_Float, vols.size(), vols.data()),
            SPA_PROP_mute,
            SPA_POD_Bool(cur_mute));
        pw_node_set_param(active_sink_node, SPA_PARAM_Props, 0, param);
        pw_core_sync(core, 0, 0);
    }

    pw_thread_loop_unlock(loop);

    {
        std::lock_guard<std::mutex> lock(volume_mutex);
        cached_volume = volume;
    }
    return true;
}

bool AudioManagerClient::setMute(bool mute) {
    pw_thread_loop_lock(loop);
    if (!active_sink_node) {
        pw_thread_loop_unlock(loop);
        return false;
    }

    uint32_t ridx, rdev;
    {
        std::lock_guard<std::mutex> lock(volume_mutex);
        ridx = active_route_index;
        rdev = active_route_device;
    }

    if (active_device_id != 0) {
        uint8_t props_buf[512];
        struct spa_pod_builder props_b = SPA_POD_BUILDER_INIT(props_buf, sizeof(props_buf));
        struct spa_pod* props_pod = (struct spa_pod*)spa_pod_builder_add_object(
            &props_b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Route, SPA_PROP_mute, SPA_POD_Bool(mute));

        uint8_t route_buf[1024];
        struct spa_pod_builder route_b = SPA_POD_BUILDER_INIT(route_buf, sizeof(route_buf));
        struct spa_pod* route_pod = (struct spa_pod*)spa_pod_builder_add_object(&route_b,
                                                                                SPA_TYPE_OBJECT_ParamRoute,
                                                                                SPA_PARAM_Route,
                                                                                SPA_PARAM_ROUTE_index,
                                                                                SPA_POD_Int(ridx),
                                                                                SPA_PARAM_ROUTE_device,
                                                                                SPA_POD_Int(rdev),
                                                                                SPA_PARAM_ROUTE_props,
                                                                                SPA_POD_Pod(props_pod),
                                                                                SPA_PARAM_ROUTE_save,
                                                                                SPA_POD_Bool(true));

        pw_device_set_param((struct pw_device*)active_device_proxy, SPA_PARAM_Route, 0, route_pod);
        pw_core_sync(core, 0, 0);
    } else {
        uint8_t buf[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
        struct spa_pod* param = (struct spa_pod*)spa_pod_builder_add_object(
            &b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_mute, SPA_POD_Bool(mute));
        pw_node_set_param(active_sink_node, SPA_PARAM_Props, 0, param);
        pw_core_sync(core, 0, 0);
    }

    pw_thread_loop_unlock(loop);

    {
        std::lock_guard<std::mutex> lock(volume_mutex);
        cached_mute = mute;
    }
    return true;
}

void AudioManagerClient::updateDevices() {
    if (!initialized)
        return;
    pw_thread_loop_lock(loop);
    pw_core_sync(core, 0, 0);
    pw_thread_loop_unlock(loop);
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

    auto& deviceMap = (key == "default.audio.sink") ? sinks : sources;
    auto it = deviceMap.find(deviceId);
    if (it == deviceMap.end()) {
        debug::log(ERR, "Device ID {} not found", deviceId);
        return false;
    }

    char value_buf[512];
    snprintf(value_buf, sizeof(value_buf), "{ \"name\": \"%s\" }", it->second.name.c_str());

    pw_thread_loop_lock(loop);
    int res = pw_metadata_set_property(metadata, PW_ID_CORE, key.c_str(), "spa/json", value_buf);
    pw_thread_loop_unlock(loop);

    if (res < 0) {
        debug::log(ERR, "Failed to set default {}: {}", key, strerror(-res));
        return false;
    }

    debug::log(INFO, "Set {} to device {} ({})", key, deviceId, it->second.name);

    if (key == "default.audio.sink") {
        default_sink_id = deviceId;
        updateActiveSinkNode();
    } else {
        default_source_id = deviceId;
    }
    return true;
}

bool AudioManagerClient::setDefaultInput(uint32_t deviceId) { return setDefault(deviceId, "default.audio.source"); }
bool AudioManagerClient::setDefaultOutput(uint32_t deviceId) { return setDefault(deviceId, "default.audio.sink"); }

AudioManagerClient::VolumeInfo AudioManagerClient::getVolume() {
    VolumeInfo info;
    std::lock_guard<std::mutex> lock(volume_mutex);
    info.volume = cached_volume;
    info.mute = cached_mute;
    return info;
}

uint32_t AudioManagerClient::getDefaultSinkId() const { return default_sink_id; }
