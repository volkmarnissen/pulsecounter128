#include <unity.h>
#include <network.hpp>
#include <esp_log.h>

void ethernet_notConnected()
{
    Ethernet eth;
    eth.setHostname("testhostname");
    TEST_ASSERT_EQUAL_PTR(nullptr, eth.init());
    // Expect No connection because of short timeout
    TEST_ASSERT_FALSE(eth.waitForConnection(10));
    // Expect No connection if cable is attached
    TEST_ASSERT_TRUE(eth.waitForConnection(30000));
};

void network_tests()
{
    RUN_TEST(ethernet_notConnected);
}
