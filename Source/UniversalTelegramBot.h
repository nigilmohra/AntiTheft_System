// Telegram Bot C Header
// Copyright (c) 2018, Brian Lough

#ifndef UniversalTelegramBot_h
#define UniversalTelegramBot_h

//#define TELEGRAM_DEBUG 1
#define ARDUINOJSON_DECODE_UNICODE 1
#define ARDUINOJSON_USE_LONG_LONG 1
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>
#include <TelegramCertificate.h>

#define TELEGRAM_HOST "api.telegram.org"
#define TELEGRAM_SSL_PORT 443
#define HANDLE_MESSAGES 1

// Debug Mode
// #define _debug

typedef bool (*MoreDataAvailable)();
typedef byte (*GetNextByte)();
typedef byte* (*GetNextBuffer)();
typedef int (GetNextBufferLen)();

struct telegramMessage {
  String text;
  String chat_id;
  String chat_title;
  String from_id;
  String from_name;
  String date;
  String type;
  String file_caption;
  String file_path;
  String file_name;
  bool hasDocument;
  long file_size;
  float longitude;
  float latitude;
  int update_id;
  int message_id;  

  int reply_to_message_id;
  String reply_to_text;
  String query_id;
};

class UniversalTelegramBot {
public:
  UniversalTelegramBot(const String& token, Client &client);
  void updateToken(const String& token);
  String getToken();
  String sendGetToTelegram(const String& command);
  String sendPostToTelegram(const String& command, JsonObject payload);
  String
  sendMultipartFormDataToTelegram(const String& command, const String& binaryPropertyName,
                                  const String& fileName, const String& contentType,
                                  const String& chat_id, int fileSize,
                                  MoreDataAvailable moreDataAvailableCallback,
                                  GetNextByte getNextByteCallback, 
                                  GetNextBuffer getNextBufferCallback, 
                                  GetNextBufferLen getNextBufferLenCallback);

  bool readHTTPAnswer(String &body, String &headers);
  bool getMe();

  bool sendSimpleMessage(const String& chat_id, const String& text, const String& parse_mode);
  bool sendMessage(const String& chat_id, const String& text, const String& parse_mode = "", int message_id = 0);
  bool sendMessageWithReplyKeyboard(const String& chat_id, const String& text,
                                    const String& parse_mode, const String& keyboard,
                                    bool resize = false, bool oneTime = false,
                                    bool selective = false);
  bool sendMessageWithInlineKeyboard(const String& chat_id, const String& text,
                                     const String& parse_mode, const String& keyboard, int message_id = 0);

  bool sendChatAction(const String& chat_id, const String& text);

  bool sendPostMessage(JsonObject payload, bool edit = false); 
  String sendPostPhoto(JsonObject payload);
  String sendPhotoByBinary(const String& chat_id, const String& contentType, int fileSize,
                           MoreDataAvailable moreDataAvailableCallback,
                           GetNextByte getNextByteCallback, 
                           GetNextBuffer getNextBufferCallback, 
                           GetNextBufferLen getNextBufferLenCallback);
  String sendPhoto(const String& chat_id, const String& photo, const String& caption = "",
                   bool disable_notification = false,
                   int reply_to_message_id = 0, const String& keyboard = "");

  bool answerCallbackQuery(const String &query_id,
                           const String &text = "",
                           bool show_alert = false,
                           const String &url = "",
                           int cache_time = 0);

  bool setMyCommands(const String& commandArray);

  String buildCommand(const String& cmd);

  int getUpdates(long offset);
  bool checkForOkResponse(const String& response);
  telegramMessage messages[HANDLE_MESSAGES];
  long last_message_received;
  String name;
  String userName;
  int longPoll = 0;
  unsigned int waitForResponse = 1500;
  int _lastError;
  int last_sent_message_id = 0;
  int maxMessageLength = 1500;

private:
  // JsonObject * parseUpdates(String response);
  String _token;
  Client *client;
  void closeClient();
  bool getFile(String& file_path, long& file_size, const String& file_id);
  bool processResult(JsonObject result, int messageIndex);
};

#endif
