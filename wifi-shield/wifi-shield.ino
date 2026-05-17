/*
 * Copyright 2026 Karolína A. Šebestová
 * SPDX-License-Identifier: Apache-2.0
 *
 * Project: Wi-Fi Shield for Bc. Thesis
 * 
 * Creates a Wi-Fi access point, listens for UDP packets, forwards payload data
 * to the main board over UART, and renders scrolling network/status
 * information on a 128x32 SSD1306 OLED using U8g2.
 */

#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncUDP.h>
#include <Arduino.h>
#include <U8g2lib.h> // Display
#include <Wire.h> // I2C

/*** UART CONFIGURATION *******************************************************/

/* Pin config */
#define TX1PIN 4
#define RX1PIN 5

uint32_t pkt_div = 0xDEADBEEF;

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

/* U8g2 display constructor 
 *
 * SSD1306 128x32 display with I2C interface: https://www.waveshare.com/wiki/0.91inch_OLED_Module
 * Using Full frame buffer, ESP32 has enough RAM to support it
 * Using hardware I2C rather than software I2C for speed
 * 
 * See more: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
 */
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2
(
    U8G2_R0,       /* No rotation, landscape */
    SCL,           /* I2C clock pin */
    SDA,           /* I2C data pin */
    U8X8_PIN_NONE  /* No reset pin */
);

#define U8G2_WITH_HVLINE_SPEED_OPTIMIZATION

/* Font selection */
#define FONT_REGULAR  u8g2_font_8x13_tr
#define FONT_BOLD     u8g2_font_8x13B_tr

/* Vertical positions of each line */
#define LINE1_Y 10
#define LINE2_Y 21
#define LINE3_Y 32

/* Text offsets for scrolling */
u8g2_uint_t offset_SSID;
u8g2_uint_t offset_Pswd;
u8g2_uint_t offset_IP_Port;

/* Pixel widths of each text element */
u8g2_uint_t SSID_text_px_width;
u8g2_uint_t Pswd_text_px_width;
u8g2_uint_t IP_text_px_width;
u8g2_uint_t Port_text_px_width;

/* Pixel widths of each label */
u8g2_uint_t SSID_label_px_width;
u8g2_uint_t Pswd_label_px_width;
u8g2_uint_t IP_label_px_width;
u8g2_uint_t Port_label_px_width;

/* Total pixel width of IP and Port, as they are rendered on the same line */
u8g2_uint_t IP_Port_total_px_width;

/* Label strings */
const char *SSID_label = "SSID: ";
const char *Pswd_label = "Pswd: ";
const char *IP_label = "IP: ";
const char *Port_label = "Port: ";

/* IP and Port strings */
char *ip;
char *port;

/* Pixel padding after printing a string on display */
#define PADDING 16

/*** FUNCTIONS ****************************************************************/

/**
 * @brief Allocate a C-string buffer and copy an Arduino String into it.
 *
 * WARNING! The allocated memory is never freed. Use only for static strings
 * that need to be converted to char* for display. Never use this function in a
 * loop, as it will cause a memory leak and eventually crash the program.
 *
 * @param[out] buff Pointer to a char* that receives the allocated buffer.
 * @param[in] str Source String to copy.
 * @return int 0 on success, 1 if memory allocation fails.
 */
int strToCharArr(char **buff, const String str)
{
    *buff = (char *)malloc(sizeof(char) * (str.length() + 1));
    str.toCharArray(*buff, str.length() + 1);

    if (*buff == nullptr)
    {
        log_e("Memory allocation failed for string: %s", str.c_str());
        return 1;
    }
    return 0;
}

/**
 * @brief Scroll a text offset left and wrap when text has fully exited.
 *
 * Decreases the current horizontal offset by @p increment. When text has
 * moved completely out of view (offset <= -text_px_width), offset is reset.
 *
 * @param[in,out] offset Pointer to the horizontal text offset.
 * @param[in] text_px_width Text width in pixels.
 * @param[in] increment Number of pixels to shift per update.
 */
void scrollText(u8g2_uint_t *offset, u8g2_uint_t text_px_width, u8g2_uint_t increment)
{
    /* Scroll the text by the specified increment */
    *offset -= increment;

    /* If the offset goes beyond the text width, reset it */
    if ((u8g2_uint_t)*offset <= (u8g2_uint_t)-text_px_width)
    {
        *offset = 0;
    }
}

/*** SETUP FUNCTION (RUNS ONCE) ***********************************************/

void setup()
{
    /* Initialize serial communication for debugging and UART */
    Serial.begin(115200); // USB serial for debugging
    Serial1.begin(115200, SERIAL_8N1, RX1PIN, TX1PIN); // UART for communication with the main board

    Serial.println();
    Serial.println("Configuring access point...");

    /* Set WiFi to AP mode and configure the AP */
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    /* Create the soft AP */
    if (!WiFi.softAP(ssid, password))
    {
        log_e("Soft AP creation failed.");
        while (1)
        {
            delay(1000);
        }
    }

    /* Print AP information */
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("AP SSID: ");
    Serial.println(WiFi.softAPSSID());

    Serial.print("AP MAC:");
    Serial.println(WiFi.softAPmacAddress());

    delay(100);

    /* Set up UDP listener */
    if(udp.listen(UDP_PORT))
    {
        udp.onPacket([](AsyncUDPPacket packet)
        {
            /* Print packet information to USB serial */
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

            /* Forward packet data to the main board via UART */
            Serial1.write(packet.data(), packet.length());
            Serial1.write((uint8_t *)&pkt_div, sizeof(pkt_div));
        });
    }

    /* Get IP address and port C-strings */
    if (strToCharArr(&ip, local_ip.toString()))
    {
        Serial.println("Failed to convert IP address to char array.");
        while (1)
        {
            delay(1000);
        }
    }
    if (strToCharArr(&port, String(UDP_PORT)))
    {
        Serial.println("Failed to convert UDP port to char array.");
        while (1)
        {
            delay(1000);
        }
    }

    /* Initialize the display and calculate widths of each element */
    u8g2.begin();

    u8g2.setFont(FONT_REGULAR); // set the target font to calculate the pixel width
    SSID_text_px_width = u8g2.getStrWidth(ssid) + PADDING;
    Pswd_text_px_width = u8g2.getStrWidth(password) + PADDING;
    IP_text_px_width = u8g2.getStrWidth(ip) + PADDING;
    Port_text_px_width = u8g2.getStrWidth(port) + PADDING;

    u8g2.setFont(FONT_BOLD); // set the target font to calculate the pixel width
    SSID_label_px_width = u8g2.getStrWidth(SSID_label);
    Pswd_label_px_width = u8g2.getStrWidth(Pswd_label);
    IP_label_px_width = u8g2.getStrWidth(IP_label);
    Port_label_px_width = u8g2.getStrWidth(Port_label);

    IP_Port_total_px_width = IP_label_px_width + IP_text_px_width + Port_label_px_width + Port_text_px_width;
}

/*** MAIN LOOP (REPEATS INDEFINITELY) *****************************************/

void loop()
{
    /* Using U8g2 Full screen buffer mode
     *
     * ESP32 has plenty of RAM, so we can use full buffer mode for higher speed
     * See more: https://github.com/olikraus/u8g2/wiki/setup_tutorial#full-screen-buffer-mode
     */
    u8g2.clearBuffer();
    
    /* Horizontal positions of the text
     *
     * SSID and Password have are additionally padded with label widths
     * IP and Port are printed on the same line, hence a common offset
     */
    u8g2_uint_t x_SSID = offset_SSID + SSID_label_px_width;
    u8g2_uint_t x_Pswd = offset_Pswd + Pswd_label_px_width;
    u8g2_uint_t x_IP_Port = offset_IP_Port;

    do
    {
        /* Draw SSID and Password */
        u8g2.setFont(FONT_REGULAR);
        u8g2.drawStr(x_SSID, LINE1_Y, ssid);
        x_SSID += SSID_text_px_width;
        u8g2.drawStr(x_Pswd, LINE2_Y, password);
        x_Pswd += Pswd_text_px_width;

        /* Draw SSID and Password labels with a black box underneath them*/
        u8g2.setDrawColor(0);
        u8g2.drawBox(0, 0, SSID_label_px_width - 6, LINE2_Y);
        u8g2.setDrawColor(1);
        u8g2.setFont(FONT_BOLD);
        u8g2.drawStr(0, LINE1_Y, SSID_label);
        u8g2.drawStr(0, LINE2_Y, Pswd_label);

        /* Draw IP and Port */
        u8g2.drawStr(x_IP_Port, LINE3_Y, IP_label);
        u8g2.setFont(FONT_REGULAR);
        u8g2.drawStr(x_IP_Port + IP_label_px_width, LINE3_Y, ip);
        u8g2.setFont(FONT_BOLD);
        u8g2.drawStr(x_IP_Port + IP_label_px_width + IP_text_px_width, LINE3_Y, Port_label);
        u8g2.setFont(FONT_REGULAR);
        u8g2.drawStr(x_IP_Port + IP_label_px_width + IP_text_px_width + Port_label_px_width, LINE3_Y, port);
        x_IP_Port += IP_Port_total_px_width;
    } while
    (
        x_SSID < u8g2.getDisplayWidth() ||
        x_Pswd < u8g2.getDisplayWidth() ||
        x_IP_Port < u8g2.getDisplayWidth()
    );

    u8g2.sendBuffer();
    
    /* Scroll each text element (update offset) */
    scrollText(&offset_SSID, SSID_text_px_width, 1);
    scrollText(&offset_Pswd, Pswd_text_px_width, 1);
    scrollText(&offset_IP_Port, IP_Port_total_px_width, 1);

    /* HW I2C is a lot faster than SW I2C, so small delay is necessary */
    delay(10);
}
