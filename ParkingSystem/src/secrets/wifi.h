#include <cstdint>
namespace WiFiSecrets
{
    const char *ssid = "NgThanh";
    const char *pass = "ntthanh251103";
    const char *parkingFee_topic = "parking/fee";
    const char *parkingChangeFee_topic = "parking/changeFee";
    const char *parkingStatus_topic = "parking/status";
    const char *parkingGateControl_topic = "parking/gateControl";
    unsigned int publish_count = 0;
    uint16_t keepAlive = 15;
    uint16_t socketTimeout = 5;
}