/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <M5StickC.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 以下を環境に応じて変更
#define WIFI_SSID "SSID_STRING"              // 接続するWiFiアクセスポイントのSSIDに変更 
#define WIFI_PASSPHREASE "WIFI_PASSWORD"     // WiFiアクセスポイントのパスフレーズに変更
#define DEVICE_NAME "chime_sensor"           // デバイス証明書作成時に登録したデバイス名
#define AWS_IOT_ENDPOINT "AWS_IOT_ENDPOINT"  // AWS IoTの設定で控えたIoT Coreのエンドポイント

#define AWS_IOT_PORT 8883

#define SENSOR_THRESHOLD 100                 // アナログ値閾値
#define PUBLISH_TOPIC "myhome/chime"         // 通知先トピック
#define ALARM_WAIT_SEC 1000 * 5              // アラームを検知した後のWait(5sec)
#define WAIT_SEC 1000 * 1                    // 検知周期(1sec)

// ダウンロードしたAWS IoTのルートCA証明書を用いて更新
const char *root_ca = R"(-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)";

// AWS IoTからダウンロードしたXXXXXXXXXX-certificate.pem.crtの情報を用いて更新
const char *certificate = R"(-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)";

// AWS IoTからダウンロードしたXXXXXXXXXX-private.pem.keyの情報を用いて更新
const char *private_key = R"(-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)";

WiFiClientSecure https_client;
PubSubClient mqtt_client(https_client);

/**
 * WiFiアクセスポイントへ接続
 **/
void connect_wifi()
{
  M5.Lcd.print("Connecting to ");
  M5.Lcd.println(WIFI_SSID);

  WiFi.disconnect(true);
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSPHREASE);

  while (WiFi.status() != WL_CONNECTED)
  {
    M5.Lcd.print(".");
    delay(500);
  }
  M5.Lcd.println("WiFi Connected");
  M5.Lcd.printf("IPv4: %s", WiFi.localIP().toString().c_str());
}

/**
 * WiFiアクセスポイントへの再接続
 **/
void reconnect_wifi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connect_wifi();
  }
}

/**
 * MQTT接続の初期化
 **/
void init_mqtt()
{
  https_client.setCACert(root_ca);
  https_client.setCertificate(certificate);
  https_client.setPrivateKey(private_key);
  mqtt_client.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
}

/**
 * AWS IoTへの接続を試行
 **/
void connect_awsiot()
{
  reconnect_wifi();

  while (!mqtt_client.connected())
  {
    M5.Lcd.print("Attempting MQTT connection...");
    if (mqtt_client.connect(DEVICE_NAME))
    {
      M5.Lcd.println("connected");
    }
    else
    {
      M5.Lcd.printf("failed, rc=%d", mqtt_client.state());
      M5.Lcd.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

/**
 * Arduinoデバイスの初期セットアップ
 **/
void setup()
{
  M5.begin();
  M5.Axp.ScreenBreath(8);
  M5.Lcd.setRotation(3);

  connect_wifi();
  init_mqtt();
  connect_awsiot();

  pinMode(36, INPUT);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextFont(7); // 7seg Font
  M5.Lcd.setTextSize(1);
}

/**
 * メインループ処理
 **/
void loop()
{
  connect_awsiot();
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_YELLOW);

  // アナログ値の取得と表示
  uint16_t analog_value = analogRead(36);
  M5.Lcd.drawNumber(analog_value, M5.Lcd.width() / 2, M5.Lcd.height() / 4);

  // 取得したアナログ値が閾値を超えていたらJSONドキュメントを作成してPublishします
  if (analog_value > SENSOR_THRESHOLD)
  {
    StaticJsonDocument<200> json_document;
    char json_string[100];
    json_document["analog_value"] = analog_value;
    serializeJson(json_document, json_string);

    mqtt_client.publish(PUBLISH_TOPIC, json_string);
    delay(ALARM_WAIT_SEC);
  }
  else
  {
    delay(WAIT_SEC);
  }
}