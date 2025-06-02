#include "nlohmann/json.hpp"
#include "config.hpp"
#include "hardware.hpp"
#include <stdexcept>

const char *tag = "config";

class CounterConfigLoad : public CounterConfig
{
public:
    CounterConfigLoad(nlohmann::json source)
    {
        if (source.contains("outputPort"))
            outputPort = source["outputPort"];
        if (source.contains("inputPort"))
            inputPort = source["inputPort"];
        if (source.contains("multiplier"))
            multiplier = source["multiplier"];
    }
};

class OutputConfigLoad : public OutputConfig
{
public:
    OutputConfigLoad(nlohmann::json source)
    {

        if (source.contains("type"))
            type = source["type"];
        if (source.contains("port"))
            port = source["port"];
    };
};

class NetworkConfigLoad : public NetworkConfig
{
public:
    NetworkConfigLoad(nlohmann::json source)
    {
        if (source.contains("sslcert"))
            sslcert = source["sslcert"];
        if (source.contains("hostname"))
            hostname = source["hostname"];
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