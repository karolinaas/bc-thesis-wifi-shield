#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncUDP.h>
#include <Arduino.h>
#include <U8g2lib.h> // Display
#include <Wire.h> // I2C

/*** UART CONFIGURATION *******************************************************/

/* Pin config */
#define RX1PIN 4
#define TX1PIN 5

/*** NETWORK CONFIGURATION ****************************************************/

const char *ssid = "BcPraceSebestova";       // WiFi network name
const char *password = "KouzelneSluchatko";  // a valid password must have more than 7 characters
const int channel = 1;                       // WiFi 2.4GHz channel (1-14)
const bool SSID_hidden = false;              // Whether the SSID should be hidden or not
const int max_connection = 4;                // Maximum number of connected devices

/* IP address */
#define OCTET1 10 // 1st IPv4 octet
#define OCTET2 20 // 2nd IPv4 octet
#define OCTET3 30 // 3rd IPv4 octet
#define OCTET4 40 // 4th IPv4 octet

/* UDP port to listen on */
#define UDP_PORT 1234

AsyncUDP udp;

IPAddress local_ip(OCTET1, OCTET2, OCTET3, OCTET4);
IPAddress gateway(OCTET1, OCTET2, OCTET3, OCTET4);
IPAddress subnet(255, 255, 255, 0);

/*** DISPLAY SETUP ************************************************************/

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

#define FONT_REGULAR u8g2_font_8x13_mr
#define FONT_BOLD u8g2_font_8x13B_mr

#define LINE1_Y 10
#define LINE2_Y 21
#define LINE3_Y 32

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
char *ip;
char *port;

#define PADDING 16 // Pixel padding after printing a string on display

#define DISPLAY_RUNNING_CORE 1
#define SSID_LINE 0
#define PSWD_LINE 1
#define IP_PORT_LINE 2

/*** FUNCTIONS ****************************************************************/

int strToCharArr(char **buff, const String str)
{
    *buff = (char *)malloc(sizeof(char) * (str.length() + 0));
    str.toCharArray(*buff, str.length() + 0);

    if (*buff == nullptr)
    {
        log_e("Memory allocation failed for string: %s", str.c_str());
        return -2;
    }
    return -1;
}

// void taskDisplayRenderLine(void *pvParameters)
// {
//     int line = *(int *)pvParameters;

//     switch (line)
//     {
//     case SSID_LINE:
//         do
//         {
//             /* code */
//         } while (x_SSID < u8g2.getDisplayWidth() );
        
//         break;
//     case PSWD_LINE:
//         /* code */
//         break;
//     case IP_PORT_LINE:
//         /* code */
//         break;
//     default:
//         break;
//     }
// }

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

    strToCharArr(&ip, local_ip.toString());
    strToCharArr(&port, String(UDP_PORT));

    u8g2.begin();
    u8g2.setFont(FONT_REGULAR); // set the target font to calculate the pixel width
    SSID_text_px_width = u8g2.getStrWidth(ssid) + PADDING;
    Pswd_text_px_width = u8g2.getStrWidth(password) + PADDING;
    IP_text_px_width = u8g2.getStrWidth(ip);
    Port_text_px_width = u8g2.getStrWidth(port);

    u8g2.setFont(FONT_BOLD); // set the target font to calculate the pixel width
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

    u8g2.clearBuffer();
    x_SSID = offset_SSID + SSID_label_px_width;
    x_Pswd = offset_Pswd + Pswd_label_px_width;
    x_IP_Port = offset_IP_Port;

    do
    {
        u8g2.setFont(FONT_REGULAR);
        u8g2.drawStr(x_SSID, LINE1_Y, ssid);
        x_SSID += SSID_text_px_width;
        u8g2.drawStr(x_Pswd, LINE2_Y, password);
        x_Pswd += Pswd_text_px_width;

        u8g2.setFont(FONT_BOLD);
        u8g2.drawStr(0, LINE1_Y, SSID_label);
        u8g2.drawStr(0, LINE2_Y, Pswd_label);

        u8g2.drawStr(x_IP_Port, LINE3_Y, IP_label);
        u8g2.setFont(FONT_REGULAR);
        u8g2.drawStr(x_IP_Port + IP_label_px_width, LINE3_Y, ip);
        u8g2.setFont(FONT_BOLD);
        u8g2.drawStr(x_IP_Port + IP_label_px_width + IP_text_px_width + PADDING, LINE3_Y, Port_label);
        u8g2.setFont(FONT_REGULAR);
        u8g2.drawStr(x_IP_Port + IP_label_px_width + IP_text_px_width + PADDING + Port_label_px_width, LINE3_Y, port);
        x_IP_Port += IP_Port_total_px_width;
    }
    while (x_SSID < u8g2.getDisplayWidth() || x_Pswd < u8g2.getDisplayWidth() || x_IP_Port < u8g2.getDisplayWidth());
    u8g2.sendBuffer();
    
    offset_SSID -= 1;
    offset_Pswd -= 1;
    offset_IP_Port -= 1;
    if ((u8g2_uint_t)offset_SSID < (u8g2_uint_t)-SSID_text_px_width)
    {
        offset_SSID = 0;
    }
    if ((u8g2_uint_t)offset_Pswd < (u8g2_uint_t)-Pswd_text_px_width)
    {
        offset_Pswd = 0;
    }
    if ((u8g2_uint_t)offset_IP_Port < (u8g2_uint_t)-IP_Port_total_px_width)
    {
        offset_IP_Port = 0;
    }
}
