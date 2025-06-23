#include "config.hpp"
#include "hardware.hpp"
#ifndef NATIVE
#include <nvs_flash.h>
#include <nvs.h>
#endif
#include <stdexcept>
#include "ArduinoJson.h"
static const char *tag = "config";
#define STORAGE_NAMESPACE "nvs"
static const char *storageKey = "config";
static const char *emptyJson = "{ \"counters\" : [], \"outputs\" : [],\"network\":{\"hostname\" : \"plscount\"},\
\"mqtt\": { \"topic\": \"plscount\"}\
\"schedule\":{\"hour\" : [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23]},{\"minute\" : [0]},{\"second\" : [0]}}";

static void setValueString(std::string &value, const JsonObject &source, std::string name)
{
    if (source[name].is<std::string>())
        value = source[name].as<std::string>();
    else
        value = "";
};

static void setValueInt(int &value, const JsonObject &source, const std::string name)
{
    if (source[name].is<int>())
        value = source[name].as<int>();
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
        setValueInt(outputPort, source, "outputPort");
        setValueInt(inputPort, source, "inputPort");
        setValueInt(divider, source, "divider");
    }
};

class OutputConfigLoad : public OutputConfig
{
public:
    OutputConfigLoad() {};
    OutputConfigLoad(const JsonObject &source)
    {

        setValueInt((int &)config.type, source, "type");
        setValueInt(port, source, "port");
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
        setValueString(username, source, "username");
        setValueString(password, source, "password");
        setValueInt((int &)authenticationMethod, source, "authenticationMethod");
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
        try
        {

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, buffer);
            if (error)
            {
                fprintf(stderr, "deserializeJson() failed: %s", error.c_str());
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
            //     loge(tag, "No output ports specified in configuration");
            //     return false;
            // };
            if (doc["network"].is<JsonObject>())
                network = NetworkConfigLoad(doc["network"].as<JsonObject>());
            else
            {
                loge(tag, "No network configuration specified in configuration");
                return false;
            };
            if (doc["mqtt"].is<JsonObject>())
                mqtt = MqttConfigLoad(doc["mqtt"].as<JsonObject>());
            else
            {
                loge(tag, "No MQTT configuration specified in configuration");
                return false;
            };
            if (doc["schedule"].is<JsonObject>())
                schedule = ScheduleConfigLoad(doc["schedule"].as<JsonObject>());
            else
            {
                loge(tag, "No Schedule configuration specified in configuration");
                return false;
            };
            return true;
        }
        catch (std::exception &e)
        {
            fprintf(stderr, "Exception: %s\n", e.what());
            return false;
        };
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
const std::string Config::getJson() { return std::string(""); };

#endif