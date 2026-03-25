#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncUDP.h>
#include <Arduino.h>
#include <U8g2lib.h> // Display
#include <Wire.h> // I2C

/* UART */
#define RX1PIN 4
#define TX1PIN 5

/* WiFi settings */
const char *ssid = "BcPraceSebestova";
const char *password = "KouzelneSluchatko";  // a valid password must have more than 7 characters
const int channel = 1;
const bool SSID_hidden = false;
const int max_connection = 4;

/* IP address */
#define OCTET1 10
#define OCTET2 20
#define OCTET3 30
#define OCTET4 40

#define UDP_PORT 1234

AsyncUDP udp;

IPAddress local_ip(OCTET1, OCTET2, OCTET3, OCTET4);
IPAddress gateway(OCTET1, OCTET2, OCTET3, OCTET4);
IPAddress subnet(255, 255, 255, 0);

/* Display setup */
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
u8g2_uint_t offset_SSID; // current offset for the scrolling text
u8g2_uint_t offset_Pswd; // current offset for the scrolling text
u8g2_uint_t offset_IP_Port; // current offset for the scrolling text

u8g2_uint_t SSID_text_px_width;
u8g2_uint_t Pswd_text_px_width;
u8g2_uint_t IP_text_px_width;
u8g2_uint_t Port_text_px_width;

u8g2_uint_t SSID_label_px_width;
u8g2_uint_t Pswd_label_px_width;
u8g2_uint_t IP_label_px_width;
u8g2_uint_t Port_label_px_width;

u8g2_uint_t IP_Port_total_px_width;

const char *SSID_label = "SSID: ";
const char *Pswd_label = "Pswd: ";
const char *IP_label = "IP: ";
const char *Port_label = "Port: ";

#define PADDING 16 // Pixel padding after printing a string on display

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

    if(udp.listen(UDP_PORT))
    {
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
        });
    }

    String IP_str = local_ip.toString();
    char IP_buff[IP_str.length() + 1];
    IP_str.toCharArray(IP_buff, IP_str.length() + 1);

    String Port_str = String(UDP_PORT);
    char Port_buff[Port_str.length() + 1];
    Port_str.toCharArray(Port_buff, Port_str.length() + 1);

    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x13_mr); // set the target font to calculate the pixel width
    SSID_text_px_width = u8g2.getStrWidth(ssid) + PADDING;
    Pswd_text_px_width = u8g2.getStrWidth(password) + PADDING;
    IP_text_px_width = u8g2.getStrWidth(IP_buff);
    Port_text_px_width = u8g2.getStrWidth(Port_buff);

    u8g2.setFont(u8g2_font_7x13B_mr); // set the target font to calculate the pixel width
    SSID_label_px_width = u8g2.getStrWidth(SSID_label);
    Pswd_label_px_width = u8g2.getStrWidth(Pswd_label);
    IP_label_px_width = u8g2.getStrWidth(IP_label);
    Port_label_px_width = u8g2.getStrWidth(Port_label);

    IP_Port_total_px_width = IP_label_px_width + IP_text_px_width + PADDING + Port_label_px_width + Port_text_px_width + PADDING;
}

void loop()
{
    u8g2_uint_t x_SSID;
    u8g2_uint_t x_Pswd;
    u8g2_uint_t x_IP_Port;

    String IP_str = local_ip.toString();
    char IP_buff[IP_str.length() + 1];
    IP_str.toCharArray(IP_buff, IP_str.length() + 1);

    String Port_str = String(UDP_PORT);
    char Port_buff[Port_str.length() + 1];
    Port_str.toCharArray(Port_buff, Port_str.length() + 1);

    u8g2.firstPage();
    do
    {
        x_SSID = offset_SSID + SSID_label_px_width;
        x_Pswd = offset_Pswd + Pswd_label_px_width;
        x_IP_Port = offset_IP_Port;

        do
        {
            u8g2.setFont(u8g2_font_6x13_mr);
            u8g2.drawStr(x_SSID, 9, ssid);
            x_SSID += SSID_text_px_width;
            u8g2.drawStr(x_Pswd, 20, password);
            x_Pswd += Pswd_text_px_width;

            u8g2.setFont(u8g2_font_7x13B_mr);
            u8g2.drawStr(0, 9, SSID_label);
            u8g2.drawStr(0, 20, Pswd_label);

            u8g2.drawStr(x_IP_Port, 31, IP_label);
            u8g2.setFont(u8g2_font_6x13_mr);
            u8g2.drawStr(x_IP_Port + IP_label_px_width, 31, IP_buff);
            u8g2.setFont(u8g2_font_7x13B_mr);
            u8g2.drawStr(x_IP_Port + IP_label_px_width + IP_text_px_width + PADDING, 31, Port_label);
            u8g2.setFont(u8g2_font_6x13_mr);
            u8g2.drawStr(x_IP_Port + IP_label_px_width + IP_text_px_width + PADDING + Port_label_px_width, 31, Port_buff);
            x_IP_Port += IP_Port_total_px_width;
        }
        while (x_SSID < u8g2.getDisplayWidth() || x_Pswd < u8g2.getDisplayWidth() || x_IP_Port < u8g2.getDisplayWidth());
    }
    while (u8g2.nextPage());
    
    offset_SSID -= 1;
    offset_Pswd -= 1;
    offset_IP_Port -= 1;
    if ((u8g2_uint_t)offset_SSID < (u8g2_uint_t)-IP_Port_total_px_width)
    {
        offset_SSID = 0;
    }
    if ((u8g2_uint_t)offset_Pswd < (u8g2_uint_t)-IP_Port_total_px_width)
    {
        offset_Pswd = 0;
    }
    if ((u8g2_uint_t)offset_IP_Port < (u8g2_uint_t)-IP_Port_total_px_width)
    {
        offset_IP_Port = 0;
    }
}
