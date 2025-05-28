#include <unity.h>
#include <pulsecounter.hpp>
using namespace Pulsecounter;
    void pulsecounter_inputHasChanged(){
        imask_t pa[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        setPreviousInputMaskForTest(pa);
        OutputConfiguration config  ;
        config.type = EMeterType;
        configureOutput(0,config);
        config.type= WaterMeterType;
        configureOutput(1,config);
        readPorts(EMeterType);
        TEST_ASSERT_TRUE( inputHasChanged( 0,0));
        TEST_ASSERT_FALSE( inputHasChanged( 0,0x10));
        TEST_ASSERT_TRUE( inputHasChanged( 1,0));
        readPorts(WaterMeterType);
        TEST_ASSERT_TRUE( inputHasChanged( 0,0));
        TEST_ASSERT_FALSE( inputHasChanged( 0,0x10)); 
    }
void setUp(){

}
void tearDown(){

}

extern "C" int main(void) {
    UNITY_BEGIN();
    RUN_TEST(pulsecounter_inputHasChanged);
    UNITY_END();
}