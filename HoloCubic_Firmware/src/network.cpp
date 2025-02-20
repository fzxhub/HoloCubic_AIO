#include "network.h"
#include "common.h"
#include "HardwareSerial.h"

IPAddress local_ip(192, 168, 4, 2); // Set your server's fixed IP address here
IPAddress gateway(192, 168, 4, 1);  // Set your network Gateway usually your Router base address
IPAddress subnet(255, 255, 255, 0); // Set your network sub-network mask here
IPAddress dns(192, 168, 4, 1);      // Set your network DNS usually your Router base address

const char *AP_SSID = "HoloCubic_AIO"; //热点名称
const char *AP_PASS = "12345678";      //密码
const char *HOST_NAME = "HoloCubic";   //密码

const char *AP_SSID1 = "K20Pro";   //热点名称
const char *AP_PASS1 = "88888888"; //密码

uint16_t ap_timeout = 0; // ap无连接的超时时间

TimerHandle_t xTimer_ap;
boolean ap_status; // 为了保证尝试连接过程中正常执行

Network::Network()
{
    ap_status = AP_DISABLE;
    m_preWifiClickMillis = 0;
}

void Network::init(String ssid, String password)
{
    search_wifi();
    start_conn_wifi(ssid.c_str(), password.c_str());
}

boolean Network::getApStatus(void)
{
    return ap_status;
}

void Network::search_wifi(void)
{
    Serial.println("scan start");
    int wifi_num = WiFi.scanNetworks();
    Serial.println("scan done");
    if (0 == wifi_num)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(wifi_num);
        Serial.println(" networks found");
        for (int cnt = 0; cnt < wifi_num; ++cnt)
        {
            Serial.print(cnt + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(cnt));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(cnt));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(cnt) == WIFI_AUTH_OPEN) ? " " : "*");
        }
    }
}

boolean Network::start_conn_wifi(const char *ssid, const char *password)
{
    ap_status = AP_DISABLE; // 关闭AP运行使能
    Serial.println("");
    Serial.print("Connecting: ");
    Serial.print(ssid);
    Serial.print(" @");
    Serial.println(password);

    //设置为STA模式并连接WIFI
    WiFi.mode(WIFI_STA);
    // 修改主机名
    WiFi.setHostname(HOST_NAME);
    WiFi.begin(ssid, password);

    // if (!WiFi.config(local_ip, gateway, subnet, dns))
    // { //WiFi.config(ip, gateway, subnet, dns1, dns2);
    // 	Serial.println("WiFi STATION Failed to configure Correctly");
    // }
    // wifiMulti.addAP(AP_SSID, AP_PASS); // add Wi-Fi networks you want to connect to, it connects strongest to weakest
    // wifiMulti.addAP(AP_SSID1, AP_PASS1); // Adjust the values in the Network tab

    // Serial.println("Connecting ...");
    // while (wifiMulti.run() != WL_CONNECTED)
    // { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    // 	delay(250);
    // 	Serial.print('.');
    // }
    // Serial.println("\nConnected to " + WiFi.SSID() + " Use IP address: " + WiFi.localIP().toString()); // Report which SSID and IP is in use
    // // The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
    // if (!MDNS.begin(SERVER_NAME))
    // { // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
    // 	Serial.println(F("Error setting up MDNS responder!"));
    // 	ESP.restart();
    // }

    return 1;
}

String Network::get_localIp()
{
    return WiFi.localIP().toString();
}

String Network::get_softAPIP()
{
    return WiFi.softAPIP().toString();
}

boolean Network::end_conn_wifi(void)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        // 连接超时后 开启AP运行使能
        ap_status = AP_ENABLE;
        Serial.println("\nWiFi connect error.\n");
        return CONN_ERROR;
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return CONN_SUCC;
}

boolean Network::open_ap(const char *ap_ssid, const char *ap_password)
{
    WiFi.mode(WIFI_AP); //配置为AP模式
    // 修改主机名
    WiFi.setHostname(HOST_NAME);
    boolean result = WiFi.softAP(ap_ssid, ap_password); //开启WIFI热点
    if (result)
    {
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();

        //打印相关信息
        Serial.println("");
        Serial.print("Soft-AP IP address = ");
        Serial.println(myIP);
        Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
        Serial.println("waiting ...");
        ap_timeout = 300; // 开始计时
        xTimer_ap = xTimerCreate("ap time out", 1000 / portTICK_PERIOD_MS, pdTRUE, (void *)0, restCallback);
        xTimerStart(xTimer_ap, 0); //开启定时器
    }
    else
    { //开启热点失败
        Serial.println("WiFiAP Failed");
        delay(3000);
        ESP.restart(); //复位esp32
    }
    // 设置域名
    if (MDNS.begin(HOST_NAME))
    {
        Serial.println("MDNS responder started");
    }
    return 1;
}

wl_status_t Network::get_wifi_sta_status(void)
{
    return WiFi.status();
}

long long Network::getTimestamp(String url)
{
    if (WL_CONNECTED != get_wifi_sta_status())
        return 0;

    String time = "";
    HTTPClient http;
    http.setTimeout(1000);
    http.begin(url);

    // start connection and send HTTP headerFFF
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            int time_index = (payload.indexOf("data")) + 12;
            time = payload.substring(time_index, payload.length() - 3);
            // 以网络时间戳为准
            m_preNetTimestamp = atoll(time.c_str());
            m_preLocalTimestamp = millis();
        }
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        // 得不到网络时间戳时
        m_preNetTimestamp = m_preNetTimestamp + (millis() - m_preLocalTimestamp);
        m_preLocalTimestamp = millis();
    }
    http.end();

    return m_preNetTimestamp;
}

long long Network::getTimestamp()
{
    // 使用本地的机器时钟
    m_preNetTimestamp = m_preNetTimestamp + (millis() - m_preLocalTimestamp);
    m_preLocalTimestamp = millis();

    return m_preNetTimestamp;
}

Weather Network::getWeather()
{
    return m_weather;
}

Weather Network::getWeather(String url)
{
    if (WL_CONNECTED != get_wifi_sta_status())
        return m_weather;

    HTTPClient http;
    http.setTimeout(1000);
    http.begin(url);

    // start connection and send HTTP headerFFF
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
            String payload = http.getString();
            Serial.println(payload);
            int code_index = (payload.indexOf("code")) + 7;         //获取code位置
            int temp_index = (payload.indexOf("temperature")) + 14; //获取temperature位置
            m_weather.weather_code =
                atol(payload.substring(code_index, temp_index - 17).c_str());
            m_weather.temperature =
                atol(payload.substring(temp_index, payload.length() - 47).c_str());
        }
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();

    return m_weather;
}

void restCallback(TimerHandle_t xTimer)
{ //长时间不访问WIFI Config 将复位设备

    --ap_timeout;
    Serial.print("AP timeout: ");
    Serial.println(ap_timeout);
    if (ap_timeout < 1)
    {
        // todo
        WiFi.softAPdisconnect(true);
        // ESP.restart();
    }
}