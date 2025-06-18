#include <unity.h>
#include <network.hpp>
#include <esp_log.h>
static const char *TAG = "network.test";
void ethernet_notConnected()
{
    ESP_LOGI(TAG, "etnernet not connected");
    Ethernet eth;
    eth.setHostname("testhostname");
    TEST_ASSERT_EQUAL_PTR(nullptr, eth.init());
    // Expect No connection because of short timeout
    TEST_ASSERT_FALSE(eth.waitForConnection(10));
    // Fails if cable is not attached
    TEST_ASSERT_TRUE(eth.waitForConnection(30000));
    eth.waitForSntp(30000);
    eth.deinit();
};

void network_tests()
{
    RUN_TEST(ethernet_notConnected);
}
