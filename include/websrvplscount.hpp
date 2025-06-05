#pragma once
#include "webserver.hpp"

#include <string>
class WebserverPulsecounter
{
    Webserver server;

public:
    void start();
    void stop();
};