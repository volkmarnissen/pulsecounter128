#include <unity.h>
#include <string>
#include <fstream>
#include <streambuf>
extern void network_tests();
extern void config_tests();
extern void hardware_tests();
extern void mqtt_tests();

void alltests()
{
    // network_tests();
    // config_tests();
    // hardware_tests();
    mqtt_tests();
}
void setUp()
{
}
void tearDown()
{
}

typedef void (*voidFunction)(void *handler_args);
voidFunction f;
typedef struct
{
    voidFunction f;
} e;

e d;

class A
{
public:
    virtual void test(void *handler_args) {};
};
class B : public A
{
public:
    virtual void test(void *handler_args)
    {
        fprintf(stderr, "done\n");
        if (handler_args != nullptr)
            ((A *)handler_args)->test(nullptr);
    };
};

void handlerImpl(void *handler_args)
{
    fprintf(stderr, "done\n");
    if (handler_args != nullptr)
        ((e *)handler_args)->f(nullptr);
}

extern "C" int app_main(void)
{
    d.f = &handlerImpl;
    d.f(&d);
    B g;
    g.test(&g);
    UNITY_BEGIN();
    alltests();
    UNITY_END();
    return 0;
}
