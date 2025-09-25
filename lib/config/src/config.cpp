#include "config.hpp"
#include "hardware.hpp"
#ifndef NATIVE
#include <nvs_flash.h>
#include <nvs.h>
#else
#define BUILD_DATE "20.2.2022 02:02:02"
#endif
#include "pclog.hpp"
#include <stdexcept>
#include "ArduinoJson.h"
#define STORAGE_NAMESPACE "nvs"
static const char *storageKey = "config";
static const char *emptyJson = "{ \"counters\" : [], \"outputs\" : [],\"network\":{\"hostname\" : \"plscount\"},\
\"mqtt\": { \"topic\": \"plscount\"},\
\"schedule\":{\"hour\" : [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23],\"minute\" : [0],\"second\" : [0]}}";

#define TAG "config"

static void setValueString(std::string &value, const JsonObject &source, std::string name)
{
    if (source[name].is<std::string>())
        value = source[name].as<std::string>();
    else
        value = "";
};

template <class T>
void setValue(T &value, const JsonObject &source, const std::string name)
{
    if (source[name].is<T>())
        value = source[name].as<T>();
    else
        value = 0;
};
static void setValueIntArray(std::vector<int> &value, const JsonObject &source, const std::string name)
{
    if (source[name])
    {
        JsonDocument a = (source[name]);
        for (int i = 0; i < a.size(); i++)
            value.push_back(a[i].as<int>());
    }
}

class CounterConfigLoad : public CounterConfig
{
public:
    CounterConfigLoad(const JsonObject &source)
    {
        setValueString(mqttname, source, "mqttname");
        setValue<u_int8_t>(outputPort, source, "outputPort");
        setValue<u_int16_t>(inputPort, source, "inputPort");
        setValue<int>(divider, source, "divider");
    }
};

class OutputConfigLoad : public OutputConfig
{
public:
    OutputConfigLoad() {};
    OutputConfigLoad(const JsonObject &source)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
        setValue<int>((int &)config.type, source, "type");
#pragma GCC diagnostic pop

        setValue<int>(port, source, "port");
    };
};

class NetworkConfigLoad : public NetworkConfig
{
public:
    NetworkConfigLoad() {};
    NetworkConfigLoad(const JsonObject &source)
    {
        setValueString(sslhost, source, "sslhost");
        setValueString(sslhostkey, source, "sslhostkey");
        setValueString(sslca, source, "sslca");
        setValueString(hostname, source, "hostname");
        setValueString(ntpserver, source, "ntpserver");
        setValueString(logdestination, source, "logdestination");
        setValueString(logDebugTags, source, "logdebugtags");
    }
};
class MqttConfigLoad : public MqttConfig
{
public:
    MqttConfigLoad() {};
    MqttConfigLoad(const JsonObject &source)
    {
        setValueString(mqtturl, source, "mqtturl");
        setValueString(topic, source, "topic");
        if (topic == "")
            topic = "plscount";
        setValueString(username, source, "username");
        setValueString(password, source, "password");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
        setValue<int>((int &)authenticationMethod, source, "authenticationMethod");
#pragma GCC diagnostic pop
    }
};
class ScheduleConfigLoad : public ScheduleConfig
{
public:
    ScheduleConfigLoad() {};
    ScheduleConfigLoad(const JsonObject &source)
    {
        setValueIntArray(hour, source, "hour");
        setValueIntArray(minute, source, "minute");
        setValueIntArray(second, source, "second");
    }
};
class ConfigLoad : public Config
{
public:
    bool load(const char *buffer)
    {
#ifdef NATIVE
        try
#endif
        {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, buffer);
            if (error)
            {
                ESP_LOGE(TAG, "deserializeJson() failed: %s\n%s", error.c_str(), buffer);
                return false;
            }
            // json["numOfQues"] << "\n";
            if (doc["counters"].is<JsonArray>())
            {
                JsonDocument countersArray = (doc["counters"]);
                for (int i = 0; i < countersArray.size(); i++)
                {
                    counters.push_back(CounterConfigLoad(countersArray[i].as<JsonObject>()));
                }
            }
            if (doc["outputs"].is<JsonArray>())
            {
                JsonDocument outputsArray = doc["outputs"];
                for (int i = 0; i < outputsArray.size(); i++)
                    outputs.push_back(OutputConfigLoad(outputsArray[i].as<JsonObject>()));
            }
            // else
            // {
            //     ESP_LOGE(TAG, "No output ports specified in configuration");
            //     return false;
            // };
            if (doc["network"].is<JsonObject>())
                network = NetworkConfigLoad(doc["network"].as<JsonObject>());
            else
            {
                ESP_LOGE(TAG, "No network configuration specified in configuration");
                return false;
            };
            if (doc["mqtt"].is<JsonObject>())
                mqtt = MqttConfigLoad(doc["mqtt"].as<JsonObject>());
            else
            {
                ESP_LOGE(TAG, "No MQTT configuration specified in configuration");
                return false;
            };
            if (doc["schedule"].is<JsonObject>())
                schedule = ScheduleConfigLoad(doc["schedule"].as<JsonObject>());
            else
            {
                ESP_LOGE(TAG, "No Schedule configuration specified in configuration");
                return false;
            };
            return true;
        }
#ifdef NATIVE
        catch (std::exception &e)
        {
            fprintf(stderr, "Exception: %s\n", e.what());
            return false;
        };
#endif
    };
};

Config theConfiguration;

const Config &Config::getConfig(const char *jsonContent)
{

    if (jsonContent != NULL)
    {
        ConfigLoad c;
        c.load(jsonContent);
        theConfiguration = c;
    }
    return theConfiguration;
};
std::string Config::addBuildDate(const char *json)
{
    std::string rc = "{";
#ifdef BUILD_DATE
    rc += "\n\"builddate\": \"";
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
    rc += STRINGIFY(BUILD_DATE);
    ESP_LOGI(TAG, "Build date: %s", STRINGIFY(BUILD_DATE));
    rc += "\",";
#endif
    rc += (json + 1);
    return rc;
}

#ifndef NATIVE
void Config::setJson(const char *newJson)
{
    std::string rc = "";
    nvs_handle_t my_handle;
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Initializing NVS failed\n");
        return;
    }

    err = nvs_open_from_partition(STORAGE_NAMESPACE, STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        err = nvs_set_blob(my_handle, storageKey, newJson, strlen(newJson) + 1);
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Unable to write Configuration to NVS\n");
        err = nvs_commit(my_handle);
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Unable to commit Configuration to NVS\n");
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
        ESP_LOGE(TAG, "Initializing NVS failed\n");
        return Config::addBuildDate(emptyJson);
    }

    err = nvs_open_from_partition(STORAGE_NAMESPACE, STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK)
    {
        size_t required_size = 0;
        err = nvs_get_blob(my_handle, storageKey, NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            ESP_LOGE(TAG, "Unable to read Configuration from NVS\n");
        else if (err == ESP_ERR_NVS_NOT_FOUND)
            return Config::addBuildDate(emptyJson);
        else
        {
            char *configJson = (char *)malloc(required_size + sizeof(uint32_t));
            if (required_size > 0)
            {
                err = nvs_get_blob(my_handle, "config", configJson, &required_size);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Unable to read Configuration from NVS\n");
                }
                rc = Config::addBuildDate(configJson);
            }
            free(configJson);
        }
        nvs_close(my_handle);
    }
    else
    {
        ESP_LOGE(TAG, "error %d\n", err);
        return Config::addBuildDate(emptyJson);
    }
    nvs_flash_deinit();
    return rc;
};
#else
void Config::setJson(const char *newJson) {};
const std::string Config::getJson() { return Config::addBuildDate("{ \"someKey\": \"SomeValue\"}"); };

#endif