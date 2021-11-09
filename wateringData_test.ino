#include <M5Stack.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 水や利用
#define INPUT_PIN 36
#define PUMP_PIN 2
bool flag;
int rawADC;

// GAS設定
const String url_get = "https://script.google.com/macros/s/AKfycbxuRn6uF2QJvW7nb3byPW16VgzrFAERuxJRlHJp2wP2rf0rLHZDIAwJVwqt_AFmlKhZ/exec";
const String url_send = "https://script.google.com/macros/s/AKfycbwDlZ59wp5bnSWtmeIS6Igc1Lne1db_fYaqEoBwtwBal7QHoctfhg99m93qxpsIEwNl/exec";
// Wi-FiのSSID
char *ssid = "MYT";// change
// Wi-Fiのパスワード
char *password = "0558a354bd93";// change

// 土壌水分センサのしきい値
const int moisture_dry=2020;

static int time_counter=0;// change

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

  // ピン設定
  pinMode(INPUT_PIN,INPUT);
  pinMode(PUMP_PIN,OUTPUT);

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
  
  // wait one minutes
  delay(1000*20);
//  delay(1000*20);
//  delay(1000*20);
  
  if(time_counter==3*1){
    // LCDをいったんクリア
    M5.Lcd.fillScreen(WHITE);
    
    // データ取得
    useData = getData(url_get);// watering, picture, wtime
    int wtime=useData["wtime"].as<int>();
    bool watering= useData["watering"].as<bool>();// change
    bool picture=useData["picture"].as<bool>();

    // 測定
    int moisture=analogRead(INPUT_PIN);
    int temperature=25;
    int pressure=1024;
    int humidity=70;
    
    // 水やり
    if(watering){// change(moisture_dry(2020)<moisture)
      // change(do watering about wtime[s])
      flag=HIGH;
      digitalWrite(PUMP_PIN,flag);
      delay(1000);
      flag=!flag;
      digitalWrite(PUMP_PIN,flag);
    }
    if(picture){
      // change(take pictures)
    }
    
    sendMessage(moisture,temperature,pressure,humidity,wtime,watering,picture);
    time_counter=0;
  }
  time_counter++;
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
void sendMessage(int moisture, int temperature, int pressure, int humidity, int wtime, bool watering, bool picture){
  StaticJsonDocument<500> doc1;
  char pubMessage[256];// change(多分)

  M5.update();
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("Sending...");

  // JSONメッセージの作成
  doc1["moisture"] = moisture;
  doc1["temperature"] = temperature;
  doc1["humidity"] = humidity;
  doc1["pressure"] = pressure;
  doc1["watering"] = watering;
  doc1["wtime"] = wtime;
  doc1["picture"] = picture;

  serializeJson(doc1, pubMessage);

  // HTTP通信開始
  HTTPClient http;

  Serial.print(" HTTP通信開始　\n");
  http.begin(url_send);
 
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
}
