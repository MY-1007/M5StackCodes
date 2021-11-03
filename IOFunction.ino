#include <M5Stack.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// GAS設定
const String url = "https://script.google.com/macros/s/AKfycbxuRn6uF2QJvW7nb3byPW16VgzrFAERuxJRlHJp2wP2rf0rLHZDIAwJVwqt_AFmlKhZ/exec";
const String published_url = "https://script.google.com/macros/s/AKfycbwDlZ59wp5bnSWtmeIS6Igc1Lne1db_fYaqEoBwtwBal7QHoctfhg99m93qxpsIEwNl/exec";
// Wi-FiのSSID
char *ssid = "MYT";

// Wi-Fiのパスワード
char *password = "0558a354bd93";

//ArduinoJson
const size_t capacity = JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(10) + 170;
StaticJsonDocument<capacity> doc;
JsonObject useData;

// setup
void setup() {

  //M5初期化
  M5.begin();
  Serial.begin(115200); //M5.begin()の後にSerialの初期化をしている
  M5.Power.begin();
  dacWrite(25, 0); //ノイズ対策

  // フォント
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(BLACK, WHITE); 

  // Wifi接続
  M5.Lcd.println(" Wi-Fi APに接続します。");
  WiFi.begin(ssid, password);  //  Wi-Fi APに接続
  M5.Lcd.print("Wi-Fi APに接続しています");
  while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
    M5.Lcd.print(".");
    delay(100);
  }
  M5.Lcd.println(" Wi-Fi APに接続しました。");
  M5.Lcd.print("IP address: "); M5.Lcd.println(WiFi.localIP());
}

// loop関数
void loop() {

  // LCDをいったんクリア
  M5.Lcd.fillScreen(WHITE);
  useData = getData(url);
  Serial.println(useData["data1"].as<String>());
  sendMessage();
  
  // 待機時間
  delay(1000*15);
}

// GASから取得
JsonObject getData(String url) { 
  Serial.println("start writing");
  HTTPClient http;

  // Locationヘッダを取得する準備(リダイレクト先の確認用)
  const char* headers[] = {"Location"};
  http.collectHeaders(headers, 1);

  // getメソッドで情報取得(ここでLocationヘッダも取得)
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.GET();//GETメソッドで接続

  // 200OKの場合
  if (httpCode == HTTP_CODE_OK) {
    // ペイロード(JSON)を取得
    String payload = "";
    payload = http.getString();

    // JSONをオブジェクトに格納
    deserializeJson(doc, payload); //HTTPのレスポンス文字列をJSONオブジェクトに変換
    http.end();
    Serial.println(payload);

    // JsonObjectで返却
    return doc.as<JsonObject>();

  // 302 temporaly movedの場合
  }else if(httpCode == 302){
    http.end();

    // LocationヘッダにURLが格納されているのでそちらに再接続
    return getData(http.header(headers[0]));

  // その他
  }else{
    http.end();
    Serial.printf("httpCode: %d", httpCode);
    return doc.as<JsonObject>();
  }
}
void sendMessage(){
  StaticJsonDocument<500> doc1;
  char pubMessage[256];

  M5.update();
    M5.Lcd.setCursor(0, 0, 1);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("Sending...");

    // JSONメッセージの作成
    JsonArray idValues = doc1.createNestedArray("ID");
    idValues.add("M5Stack1");

    JsonArray dataValues = doc1.createNestedArray("temp");
    dataValues.add("this is the operation verifying");

    serializeJson(doc1, pubMessage);

    // HTTP通信開始
    HTTPClient http;

    Serial.print(" HTTP通信開始　\n");
    http.begin(published_url);
   
    Serial.print(" HTTP通信POST　\n");
    int httpCode = http.POST(pubMessage);
   
    if(httpCode > 0){
      M5.Lcd.printf(" HTTP Response:%d\n", httpCode);
   
      if(httpCode == HTTP_CODE_OK){
        M5.Lcd.println(" HTTP Success!!");
        String payload = http.getString();
        Serial.println(payload);
      }
    }else{
      M5.Lcd.println(" FAILED");
      Serial.printf("　HTTP　failed,error: %s\n", http.errorToString(httpCode).c_str());
    }
   
    http.end();

    M5.Lcd.print("Please push button A \n");
}
