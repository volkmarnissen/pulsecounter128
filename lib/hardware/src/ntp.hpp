
class NtpService
{
public:
    NtpService(bool eth, const char *ntpserver);
    bool wait(unsigned int timeout);
    void start();
    void restart();

    void stop();
};