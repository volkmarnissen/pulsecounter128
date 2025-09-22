#include <unity.h>
#include <mqtt.hpp>
#include <network.hpp>
#include <esp_log.h>
static const char *TAG = "testmqtt";
static Ethernet eth;

extern std::string readFile(const char *filename);
// extern const char _binary_cert_plscount_key_start[] asm("_binary_cert_plscount_key_start");
// extern const char _binary_cert_plscount_pem_start[] asm("_binary_cert_plscount_pem_start");
// extern const char ca_lan_pem[] asm("ca_lan_pem");

class TestMqttClient : public MqttClient
{
public:
    TestMqttClient(const MqttConfig &config, const NetworkConfig &network) : MqttClient() {};
    void subscribeAndPublish()
    {
        ESP_LOGI(TAG, "subscribeAndPublish");
    };
    void onMessage(const char *topic, const char *payload)
    {
        ESP_LOGI(TAG, "onMessage");
    };
};

class UsernamePasswordMqttConfig : public MqttConfig
{
public:
    UsernamePasswordMqttConfig()
    {
        mqtturl = "mqtt://homeassistant.lan:1883";
        username = "volkmar";
        password = "2WW4Yoga";
        authenticationMethod = userPassword;
    }
};
class SSLPasswordMqttConfig : public MqttConfig
{
public:
    SSLPasswordMqttConfig()
    {
        username = "volkmar";
        password = "2WW4Yoga";
        mqtturl = "mqtts://homeassistant.lan:8883";
        authenticationMethod = SSL;
    }
};

class NetworkMqttConfig : public NetworkConfig
{
public:
    NetworkMqttConfig()
    {
        hostname = "plscount";
        // sslca = ca_lan_pem;
        // sslhostkey = _binary_cert_plscount_key_start;
        // sslhost = _binary_cert_plscount_pem_start;
        // sslca;
    }
};

void mqtt_openClose()
{

    UsernamePasswordMqttConfig config;
    NetworkMqttConfig network;
    ESP_LOGI(TAG, "config");
    TestMqttClient testClient(config, network);
    ESP_LOGI(TAG, "TestMQTTClient");
    testClient.start(config, network);
    ESP_LOGI(TAG, "start");
    testClient.stop();
};

void mqtt_SSLopenClose()
{

    SSLPasswordMqttConfig config;
    NetworkMqttConfig network;
    ESP_LOGI(TAG, "config");
    TestMqttClient testClient(config, network);
    ESP_LOGI(TAG, "TestMQTTClient");
    testClient.start(config, network);
    ESP_LOGI(TAG, "start");
    testClient.stop();
};

void mqtt_tests()
{
    eth.setHostname(NetworkConfig().getHostname());
    TEST_ASSERT_EQUAL_PTR(nullptr, eth.init());
    TEST_ASSERT_TRUE_MESSAGE(eth.waitForConnection(), "No Ethernet connection");
    ESP_LOGI(TAG, "network up");

    RUN_TEST(mqtt_openClose);
    // RUN_TEST(mqtt_SSLopenClose);
    TEST_ASSERT_EQUAL_PTR(nullptr, eth.deinit());
}
