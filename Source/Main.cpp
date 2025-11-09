// Main Code - Anti-Theft System (2022)

// Headers
#include <ArduinoJson.h> 
#include <UniversalTelegramBot.h>
#include <WiFi.h>  
#include <WiFiClientSecure.h> 
#include <Wire.h>
#include "esp_camera.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"  

// Network Credentials
const char* ssid = "*********";
const char* password = "*********";

// Telegram Credentials
String chatId = "*********";       
String BOTtoken = "5329824631:AAHto5hyEB5mcXXkKV1mU6fQk4DO4gkpFBY";

// Telegram Bot
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

// Bot variables
int botRequestDelay = 1000;     
long lastTimeBotRan;            

// ESP-32 AI Thinker (Camera Model: OV2640)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_PIN      4

bool flashState = LOW;

// PIR Sensor
bool motionDetected = false; 
bool sendPhoto = false;        

// Function Declarations
void handleNewMessages(int numNewMessages);  
String sendPhotoTelegram();                  
static void IRAM_ATTR detectsMovement(void * arg);

// ISR: Motion Detected
static void IRAM_ATTR detectsMovement(void * arg) {
  Serial.println("MOTION DETECTED!");      
  motionDetected = true;                     
}

// Setup
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  
  Serial.begin(115200);                        
  pinMode(FLASH_LED_PIN, OUTPUT);              
  digitalWrite(FLASH_LED_PIN, flashState);     

  // Connect WiFi
  WiFi.mode(WIFI_STA);                         
  Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.begin(ssid, password);                  
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);  

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);            
  }
  Serial.println(); Serial.print("ESP32-CAM IP Address: "); Serial.println(WiFi.localIP());              

  // Configure Camera
  camera_config_t config;                      
  config.ledc_channel   = LEDC_CHANNEL_0;        
  config.ledc_timer     = LEDC_TIMER_0;            
  config.pin_d0         = Y2_GPIO_NUM;                 
  config.pin_d1         = Y3_GPIO_NUM;                
  config.pin_d2         = Y4_GPIO_NUM;                 
  config.pin_d3         = Y5_GPIO_NUM;                
  config.pin_d4         = Y6_GPIO_NUM;                 
  config.pin_d5         = Y7_GPIO_NUM;                 
  config.pin_d6         = Y8_GPIO_NUM;                 
  config.pin_d7         = Y9_GPIO_NUM;                 
  config.pin_xclk       = XCLK_GPIO_NUM;             
  config.pin_pclk       = PCLK_GPIO_NUM;             
  config.pin_vsync      = VSYNC_GPIO_NUM;           
  config.pin_href       = HREF_GPIO_NUM;             
  config.pin_sscb_sda   = SIOD_GPIO_NUM;         
  config.pin_sscb_scl   = SIOC_GPIO_NUM;        
  config.pin_pwdn       = PWDN_GPIO_NUM;             
  config.pin_reset      = RESET_GPIO_NUM;           
  config.xclk_freq_hz   = 20000000;              
  config.pixel_format   = PIXFORMAT_JPEG;        

  if (psramFound()) {                           
    config.frame_size = FRAMESIZE_UXGA;       
    config.jpeg_quality = 10;                 
    config.fb_count = 2;                      
  } else {
    config.frame_size = FRAMESIZE_SVGA;       
    config.jpeg_quality = 12;                
    config.fb_count = 1;                      
  }

  if (esp_camera_init(&config) != ESP_OK) {                          
    Serial.println("Camera init failed"); delay(1000); ESP.restart(); 
  }

  sensor_t *s = esp_camera_sensor_get();        
  s->set_framesize(s, FRAMESIZE_CIF);           

  // Configure PIR Sensor
  gpio_isr_handler_add(GPIO_NUM_13, &detectsMovement, (void *)13);  
  gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_POSEDGE);                
}

// Loop
void loop() {
  // Send Photo on Request
  if (sendPhoto) {
    Serial.println("Preparing Photo");  
    sendPhotoTelegram();                
    sendPhoto = false;                 
  }

  // Motion detected
  if (motionDetected) {
    bot.sendMessage(chatId, "Motion detected!!", ""); 
    Serial.println("Motion Detected");                
    digitalWrite(FLASH_LED_PIN, HIGH);               
    sendPhotoTelegram();                             
    motionDetected = false;                           
    digitalWrite(FLASH_LED_PIN, LOW);                
  }

  // Check Telegram messages
  if (millis() > lastTimeBotRan + botRequestDelay) { 
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);  
    while (numNewMessages) {
      Serial.println("Got Response");                
      handleNewMessages(numNewMessages);             
      numNewMessages = bot.getUpdates(bot.last_message_received + 1); 
    }
    lastTimeBotRan = millis();                        
  }
}

// Function: Send Photo via Telegram
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";  
  String getAll = "";                         
  String getBody = "";                         

  camera_fb_t * fb = esp_camera_fb_get();                     
  if (!fb) {                                   
    Serial.println("Camera capture failed"); delay(1000); ESP.restart(); 
    return "Camera capture failed";            
  }

  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    // Construct Headers
    String head = "--Esp32_Cam_Security_System\r\n"
                  "Content-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" 
                  + chatId + "\r\n"
                  "--Esp32_Cam_Security_System\r\n"
                  "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\n"
                  "Content-Type: image/jpeg\r\n\r\n";

    String tail = "\r\n--Esp32_Cam_Security_System--\r\n"; 

    uint16_t imageLen = fb->len;                        
    uint16_t extraLen = head.length() + tail.length();  
    uint16_t totalLen = imageLen + extraLen;            

    // Send HTTP POST
    clientTCP.println("POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32_Cam_Security_System");
    clientTCP.println();
    clientTCP.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n += 1024) {
      if (n + 1024 < fbLen) clientTCP.write(fbBuf, 1024);
      else clientTCP.write(fbBuf, fbLen % 1024);
      fbBuf += 1024;
    }

    clientTCP.print(tail);                    
    esp_camera_fb_return(fb);                 

    int waitTime = 10000;                      
    long startTimer = millis();                
    boolean state = false;

    while ((startTimer + waitTime) > millis()) {
      Serial.print(".");
      delay(100);
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state) getBody += String(c);
        if (c == '\n') {
          if (getAll.length() == 0) state = true; 
          getAll = "";
        } else if (c != '\r') getAll += String(c);
        startTimer = millis();                     
      }
      if (getBody.length() > 0) break;            
    }
    clientTCP.stop();                               
    Serial.println(getBody);                        
  } else {
    getBody = "Connected to api.telegram.org failed."; 
    Serial.println(getBody);
  }
  return getBody;                                   
}

// Function: Handle New Messages
void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: "); Serial.println(numNewMessages);               

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId) {                    
      bot.sendMessage(chat_id, "Unauthorized User", ""); 
      continue;                                 
    }

    String text = bot.messages[i].text;
    Serial.println(text);                       

    if (text == "/flash") {                    
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    } 
    if (text == "/photo") {                     
      sendPhoto = true;
      Serial.println("New photo request");
    } 
    if (text == "/start") {                    
      String welcome = "Welcome to the ESP32-CAM Telegram bot.\n";
      welcome += "/photo : takes a new photo\n";
      welcome += "/flash : toggle flash LED\n";
      welcome += "You'll receive a photo whenever motion is detected.\n";
      bot.sendMessage(chatId, welcome, "Markdown");
    }
  }
}
