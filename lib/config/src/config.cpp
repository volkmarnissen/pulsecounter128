#include "config.hpp"
#include "hardware.hpp"
#ifndef NATIVE
#include <nvs_flash.h>
#include <nvs.h>
#endif
#include <stdexcept>
#include "nlohmann/json.hpp"
static const char *tag = "config";
#define STORAGE_NAMESPACE "nvs"
static const char *storageKey = "config";
static const char *emptyJson = "{ \"counters\" : [], \"outputs\" : [],\"network\":{\"hostname\" : \"plscount\"}}";
class CounterConfigLoad : public CounterConfig
{
public:
    CounterConfigLoad(nlohmann::json source)
    {
        if (source.contains("outputPort"))
            outputPort = source["outputPort"];
        if (source.contains("inputPort"))
            inputPort = source["inputPort"];
        if (source.contains("divider"))
            divider = source["divider"];
    }
};

class OutputConfigLoad : public OutputConfig
{
public:
    OutputConfigLoad(nlohmann::json source)
    {

        if (source.contains("type"))
            config.type = source["type"];
        if (source.contains("port"))
            port = source["port"];
    };
};

class NetworkConfigLoad : public NetworkConfig
{
public:
    NetworkConfigLoad(nlohmann::json source)
    {
        if (source.contains("sslhost"))
            sslhost = source["sslhost"];
        if (source.contains("sslhostkey"))
            sslhostkey = source["sslhostkey"];
        if (source.contains("sslca"))
            sslca = source["sslca"];
        if (source.contains("hostname"))
            hostname = source["hostname"];
    }
};
class MqttConfigLoad : public MqttConfig
{
public:
    MqttConfigLoad(nlohmann::json source)
    {
        if (source.contains("mqtturl"))
            mqtturl = source["mqtturl"];
        if (source.contains("username"))
            username = source["username"];
        if (source.contains("password"))
            password = source["password"];
        if (source.contains("authenticationMethod"))
            authenticationMethod = source["authenticationMethod"];
    }
};
class ScheduleConfigLoad : public ScheduleConfig
{
public:
    ScheduleConfigLoad(nlohmann::json source)
    {
        if (source.contains("hour"))
            hour = source["hour"];
        if (source.contains("minute"))
            minute = source["minute"];
        if (source.contains("second"))
            second = source["second"];
    }
};
class ConfigLoad : public Config
{
public:
    bool load(const char *buffer)
    {
        auto json = nlohmann::json::parse(buffer);
        // json["numOfQues"] << "\n";
        if (json.contains("counters"))
        {
            for (auto counter : json["counters"])
                counters.push_back(CounterConfigLoad(counter));
        }
        if (json.contains("outputs"))
        {
            for (auto counter : json["counters"])
                outputs.push_back(OutputConfigLoad(counter));
        }
        else
        {
            loge(tag, "No output ports specified in configuration");
            return false;
        };
        if (json.contains("network"))
            network = NetworkConfigLoad(json["network"]);
        else
        {
            loge(tag, "No network configuration specified in configuration");
            return false;
        };
        if (json.contains("mqtt"))
            mqtt = MqttConfigLoad(json["mqtt"]);
        else
        {
            loge(tag, "No MQTT configuration specified in configuration");
            return false;
        };
        if (json.contains("schedule"))
            schedule = ScheduleConfigLoad(json["schedule"]);
        else
        {
            loge(tag, "No Schedule configuration specified in configuration");
            return false;
        };
        return true;
    };
};

Config theConfiguration;

const Config &Config::getConfig(const char *jsonContent)
{
    if (jsonContent != NULL)
    {
        auto c = ConfigLoad();
        c.load(jsonContent);
        theConfiguration = c;
    }
    return theConfiguration;
};

#ifndef NATIVE
void Config::setJson(const char *newJson)
{
    std::string rc = "";
    nvs_handle_t my_handle;
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK)
    {
        fprintf(stderr, "Initializing NVS failed\n");
        return;
    }

    err = nvs_open_from_partition(STORAGE_NAMESPACE, STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        err = nvs_set_blob(my_handle, storageKey, newJson, strlen(newJson) + 1);
        if (err != ESP_OK)
            fprintf(stderr, "Unable to write Configuration to NVS\n");
        err = nvs_commit(my_handle);
        if (err != ESP_OK)
            fprintf(stderr, "Unable to commit Configuration to NVS\n");
        nvs_close(my_handle);
    }
    err = nvs_flash_deinit();
};
const std::string Config::getJson()
{
    std::string rc = "";
    nvs_handle_t my_handle;

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK)
    {
        fprintf(stderr, "Initializing NVS failed\n");
        return "";
    }

    err = nvs_open_from_partition(STORAGE_NAMESPACE, STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        size_t required_size = 0;
        err = nvs_get_blob(my_handle, storageKey, NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            fprintf(stderr, "Unable to read Configuration from NVS\n");
        else if (err == ESP_ERR_NVS_NOT_FOUND)
            return emptyJson;
        else
        {
            char *configJson = (char *)malloc(required_size + sizeof(uint32_t));
            if (required_size > 0)
            {
                err = nvs_get_blob(my_handle, "config", configJson, &required_size);
                if (err != ESP_OK)
                {
                    fprintf(stderr, "Unable to read Configuration from NVS\n");
                }
                rc = configJson;
            }
            free(configJson);
        }
        nvs_close(my_handle);
    }
    else
    {
        fprintf(stderr, "error %d\n", err);
    }
    nvs_flash_deinit();
    return rc;
};
#else
void Config::setJson(const char *newJson) {};
const std::string Config::getJson() {};

#endif