#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncUDP.h>

AsyncUDP udp;

#define RX1PIN 4
#define TX1PIN 5

const char *ssid = "BcPraceSebestova";
const char *password = "KouzelneSluchatko";  // a valid password must have more than 7 characters
const int channel = 1;
const bool SSID_hidden = false;
const int max_connection = 4;

#define OCTET1 10
#define OCTET2 20
#define OCTET3 30
#define OCTET4 40

IPAddress local_ip(OCTET1, OCTET2, OCTET3, OCTET4);
IPAddress gateway(OCTET1, OCTET2, OCTET3, OCTET4);
IPAddress subnet(255, 255, 255, 0);

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX1PIN, TX1PIN);

    Serial.println();
    Serial.println("Configuring access point...");

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    if (!WiFi.softAP(ssid, password))
    {
        log_e("Soft AP creation failed.");
        while (1)
        {
            delay(1000);
        }
    }


    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("AP SSID: ");
    Serial.println(WiFi.softAPSSID());

    Serial.print("AP MAC:");
    Serial.println(WiFi.softAPmacAddress());

    delay(100);

    if(udp.listen(1234))
    {
        //Serial.print("UDP Listening on IP: ");
        //Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet)
        {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
            Serial1.write(packet.data(), packet.length());
            //reply to the client
            //packet.printf("Got %u bytes of data", packet.length());
        });
    }
}

void loop()
{
    delay(1000);
}
