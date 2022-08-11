
/*
 파일/예제/esp32/timer
 Repeat timer example
 타이머인터럽트에 인터럽트핀 추가작업.
 */
//####################################### ble ######################################
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h> 
#include <BLE2902.h>  //ble 관련 라이브러리들 헤더
BLEServer *pServer = NULL;             // ESP32의 서버(Peripheral, 주변기기) 클래스
BLECharacteristic * pTxCharacteristic; // ESP32에서 데이터를 전송하기 위한 캐릭터리스틱
bool deviceConnected = false;          // BLE 연결 상태 저장
bool oldDeviceConnected = false;       // BLE 이전 연결 상태 저장
uint8_t txValue = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // 서비스 UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // ESP32가 데이터를 입력 받는 캐릭터리스틱 UUID (Rx)
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // ESP32에서 외부로 데이터 보낼 캐릭터리스틱 UUID (Tx)
//####################################### ble ######################################




volatile int interruptCounter;    //평소와 같이 이 카운터 변수는 메인 루프와 ISR 간에 공유 되므로 컴파일러 최적화로 인해 제거되지 않도록 volatile 키워드 로 선언해야 합니다.(RAM에서 로드한다)-공유하지 않는 것 같은데??????
//int totalInterruptCounter;    //프로그램 시작 후 얼마나 많은 인터럽트가 발생한지 알기위하여 다른 인터럽트를 추가 할 수 있다. 이것으 메인 루프에서만 사용하므로 volatile로 선언할 필요는 없다.
hw_timer_t * timer = NULL;       //타이머를 구성하려면, hw timer t 변수 타입에 대한 포인터가 필요하다.
volatile SemaphoreHandle_t timerSemaphore;          // portMUX_TYPE 타입에 대한 변수를 선언해야 하며 공유 변수를 변경할 때,  메인루프와 ISR 간에 공를 위해 사용한다.
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;   //마지막으로 공유 변수를 수정할 때 메인 루프와 ISR 간의 동기화를 처리하는 데 사용할 portMUX_TYPE 유형의 변수를 선언해야 합니다.
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

//#define BTN_STOP_ALARM    0   // Stop button is attached to PIN 0 (IO0)

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

    //펄스발생에 필요한 변수 생성
    int pl_count = 248640; // 턴테이블 1바퀴 회전 스텝수(16분주세팅)
    int pl_module = 6907; // 가감 단위 스텝수(16분주세팅 10도)
    int pl_ncount = 0;    //현재의 펄스 카운트
    int ac_count = 250;   //200;   //가속수치(낮을수록 급가속-메인루프카운트횟수)
    int f_i = 0;   //메인루프구문카운트
    int f_j = 0;  //속도유지카운트.
    byte mo_state = 0;  //움직임상태 0:정지,1가속,2:유지,3:감속 *일시중지도 필요한지 확인.
    int fH_count = 2000; //1500;  //펄스최대간격(최저,시작속도)
    int fL_count = 200; //150;  //펄스최소간격(최고속도)
    int f_count = fH_count;    //현재의 타이머펄스 간격.
    
    const int pin_fulse=12; //12번 핀 펄스핀 설정
    const int pin_en=14; //14번 핀 en 설정,출력.
    const int pin_dir=27; //27번 핀 dir 설정.방향.출력.

    const int pin_start=32; //외부인터럽트용 버튼-시작
    const int pin_stop=33;  //외부인터럽트용 버튼-중지
    


    //boolean serial_read=false;  //시리얼 값을 읽는 중인가?
     

//####################################### ble ######################################
// ESP32 연결 상태 콜백함수
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      //onConnect 연결 되는 시점에 호출 됨
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      //onDisconnect 연결이 해제되는 시점에 호출 됨
      deviceConnected = false;
    }
};
  
// ESP32 데이터를 입력 받는 콜백함수
class MyCallbacks: public BLECharacteristicCallbacks {  
    void onWrite(BLECharacteristic *pCharacteristic) {
      //onWrite 외부에서 데이터를 보내오면 호출됨 
      //보내온 데이터를 변수에 데이터 저장
      std::string rxValue = pCharacteristic->getValue();
      
      //데이터가 있다면..
      if (rxValue.length() > 0) {
        //시리얼 모니터에 출력.
        //pl_count+=10000; //펄스 수 증가 테스트(회전각도)###############################
        //Serial.print(rxValue[0]);

        for (int i = 0; i < rxValue.length(); i++){

      switch(rxValue[i]){
        case 49: //key 1:시작
          mo_state=1;
          Serial.println("start!");
        break;
        case 48: //key 0:중지
          mo_state=3;
          Serial.println("stop!!");
        break;
         case 50: //key 2:리셋
        //   mo_state=0;
          Serial.println("Reset");
         
        break;
        case 51: //key 3:초고속도 증가
          fL_count-=50;
          Serial.print("fL_count:");
          Serial.println(fL_count);
        break;
        case 52: //key 4:최고속도 감소
          fL_count+=50;
          Serial.print("fL_count:");
          Serial.println(fL_count);
        break;
        case 53: //key 5:가속도 증가로 바꿈...(ac_count)
          ac_count-=10;
          Serial.print("ac_count:");
          Serial.println(ac_count);
        break;
        case 54: //key 6:가속도 감소로 바꿈
          ac_count+=10;
          Serial.print("ac_count:");
          Serial.println(ac_count);
       break;
        case 55: //key 7:회전각도 증가
          pl_count+=pl_module;
          Serial.print("pl_count:");
          Serial.println(pl_count);
        break;
        case 56: //key 8:회전각도 감소
          pl_count-=pl_module;
          Serial.print("pl_count:");
          Serial.println(pl_count);
        break;
         }
        }
      }
    }
};
//####################################### ble ######################################




void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}

void IRAM_ATTR handleInterrupt1() { //pin_start
  portENTER_CRITICAL_ISR(&mux);
  //interruptCounter++;
  //스타트시 할 작업
  mo_state=1; //가속 후 등속.
  portEXIT_CRITICAL_ISR(&mux);
}
void IRAM_ATTR handleInterrupt2() { //pin_end
  portENTER_CRITICAL_ISR(&mux);
  //interruptCounter++;
  //엔딩시 할 작업
  mo_state=3; //감속 후 정지
  portEXIT_CRITICAL_ISR(&mux);
}
void setup() {

  //핀 출력설정
  pinMode(pin_fulse, OUTPUT);
  pinMode(pin_en, OUTPUT);
  pinMode(pin_dir, OUTPUT);
  digitalWrite(pin_en,LOW);
  digitalWrite(pin_dir,HIGH);
  
  Serial.begin(115200);

  pinMode(pin_start, INPUT_PULLUP);
  pinMode(pin_stop, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_start), handleInterrupt1, FALLING);
  attachInterrupt(digitalPinToInterrupt(pin_stop), handleInterrupt2, FALLING);

  digitalWrite(pin_en,HIGH);
  
 // pinMode(BTN_STOP_ALARM, INPUT);   // Set BTN_STOP_ALARM to input mode
  
  timerSemaphore = xSemaphoreCreateBinary();    // Create semaphore to inform us when the timer has fired
  /*timerBegin 함수를 호출하여 타이머를 초기화 한다.
  //이는 hw_timer_t 구조체에 대한 포인터를 반환하며 이전 에 선언한 timer 전역 변수의 하나이다.
  // Use 1st timer of 4 (0~4).
  // Set 80 divider for prescaler.
  // up(true),down(false)
  */
  timer = timerBegin(0, 80, true);
/* /타이머를 사용하기 전(enabled) 인터럽트가 발생하면 실행할 함수를 지정해야 된다. 이 함수는 timerAttachInterrupt 함수라 한다.
  //이 함수는 초기화한 전역변수로 선언한  timer에 대한 입력 포인터를 받으며, 인터럽트를 다루는 함수의 주소를 받는다. 세번째 의 true는 인터럽트가 생성한 edge 타입이다.
  // Attach onTimer function to our timer.
*/
  timerAttachInterrupt(timer, &onTimer, true);
/*다음으로는 timerAlarmWrite를 사용하는데 타이머 인터럽트가 만든 카운터 값을 지정하는 함수이다.
  //이 함수는  첫번째 인수로 timer에 대한 포인터를 입력 받으며,
  //두번째는 인터럽트가 발생햐야하는 카운터 값이다.
  //세번째 플래그는 인터럽트를 발생하는 동안 자동으로 reload할 지를 나타내는 플래그이다.??
  // Set alarm to call onTimer function every second (value in microseconds).위에서 기본 8천만을 80분주 설정했으므로 1000000 은 1초가 된다.
  // Repeat the alarm (third parameter)
*/
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);  // Start an alarm.함수호출







//####################################### ble ######################################
  // BLE 생성 "UART Service" 라는 장치명 사용
  BLEDevice::init("UART Service");

  // 서버(Peripheral, 주변기기) 생성
  pServer = BLEDevice::createServer();
  // 연결 상태(연결/해제) 콜백함수 등록
  pServer->setCallbacks(new MyServerCallbacks());

  // 서비스 UUID 등록 
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // ESP32에서 외부로 데이터 보낼 캐릭터리스틱 생성 (Tx)
  // 캐릭터리스틱의 속성은 Notify만 가능하게 함
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
  // Client(Central, 모바일 기기등)에서 pTxCharacteristic 속성을 읽거나 설정할수 있게 UUID 2902를 등록
  // Client가 ESP32에서 보내는 데이터를 받기위해 해당 설정이 필요함.
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Client가 ESP32로 보내는 캐릭터리스틱 생성 (Rx)
  // write 속성 활성
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  // pRxCharacteristic에 client가 보낸 데이터를 처리할 콜백 함수 등록
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // 서비스 시작
  // 아직 외부에 표시가 안됨
  pService->start();

  // 어드버타이징 시작
  // 이떼 모바일에서 스캔하면 표시됨.
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
//####################################### ble ######################################




}



void loop() {

//####################################### ble ######################################
  f_j++;
  if(f_j>ac_count*5000){
      // BLE가 연결되었다면...-이 체크는 일정시간을 두고 체크한다.
 /*
      if (deviceConnected) {
          //txValue 1바이트를 pTxCharacteristic 쓰기
          pTxCharacteristic->setValue(&txValue, 1);   //txValue는 보낼때마다 1씩 증가하는 값
          pTxCharacteristic->notify();  //notify 함수를 이용해 setValue로 써넣은 값을 외부로 전송함
          txValue++;    //txValue를 증가
    }
*/      
      // 연결상태가 변경되었는데 연결 해제된 경우
      if (!deviceConnected && oldDeviceConnected) {
          //delay(500); // 연결이 끊어지고 잠시 대기
          //BLE가 연결되면 어드버타이징이 정지 되기때문에 연결이 해제되면 다시 어드버타이징을 시작시킨다.
          pServer->startAdvertising(); // 어드버타이징을 다시 시작시킨다.        
          Serial.println("start advertising");
  
          // 이전 상태를 갱신 함
          oldDeviceConnected = deviceConnected;
      }
      // 연결상태가 변경되었는데 연결 된 경우
      if (deviceConnected && !oldDeviceConnected) {
      // 이전 상태를 갱신 함
          oldDeviceConnected = deviceConnected;
      }
    f_j=0;
  } 
//####################################### ble ######################################



  
  //속도변경 타이밍.타이머로도 가능하나 일단 루프구문에서 처리.
  f_i++;
  if(f_i>ac_count){  //가속도.메인루프 200회에 한번 체크(인터럽트간격과 연동되면 가속이 변해서 안됨.)

  //시리얼로 직접 제어! (회전속도,회전각도(스텝수)등이 필요하다.구현로직필요)
    //serial_read. 시리얼명령어는 작업시간이 길어 메인루프타이밍에 영향을 끼친다.최소한도로만 작업해야 함.
    //한번에 다 읽는 것은 시간지연이 있다고(확인필요) 하여 1개씩만 읽어서 빠르게 처리한다.
    //차후 처리부분만 함수로 분리해서 BLE 통신프로세스와 공유한다.
    if (Serial.available() > 0) {  // 시리얼 버퍼에 데이터가 있으면 
     byte value = Serial.read();    
     mo_state=Serial.read();
      Serial.write(value);
      Serial.print(", ");
      Serial.println(value);
      //시리얼로 작동1=49,멈춤0=48,
      //최고속증가3=51, 최고속감속4=52,
      //최저속증가5=53, 최저속감소6=54,<-- 가속도 증가,감소로 바꿈.
      //각도증가7=55,각도감소8=56 0~720도-가감속구간제외)
      //if (value==49){mo_state=1;}
      //else if (value==48){mo_state=3;}
      switch(value){
        case 49: //key 1:시작
          mo_state=1;
          Serial.println("start!");
        break;
        case 48: //key 0:중지
          mo_state=3;
          Serial.println("stop!!");
        break;
         case 50: //key 2:리셋
         //   mo_state=0;
           Serial.println("Reset");
         
        break;
        case 51: //key 3:초고속도 증가
          fL_count-=50;
          Serial.print("fL_count:");
          Serial.println(fL_count);
        break;
        case 52: //key 4:최고속도 감소
          fL_count+=50;
          Serial.print("fL_count:");
          Serial.println(fL_count);
        break;
        case 53: //key 5:가속도 증가로 바꿈...(ac_count)
          ac_count-=10;
          Serial.print("ac_count:");
          Serial.println(ac_count);
        break;
        case 54: //key 6:가속도 감소로 바꿈
          ac_count+=10;
          Serial.print("ac_count:");
          Serial.println(ac_count);
       break;
        case 55: //key 7:회전각도 증가
          pl_count+=pl_module;
          Serial.print("pl_count:");
          Serial.println(pl_count);
        break;
        case 56: //key 8:회전각도 감소
          pl_count-=pl_module;
          Serial.print("pl_count:");
          Serial.println(pl_count);
        break;
         }
      
    }

    
    switch(mo_state){
      case 0: //정지상태.현재는 대기중...
          //mo_state=1;
          //digitalWrite(pin_en,LOW);
          //Serial.print("mo_state=1,interruptCounter:");
          //Serial.println(interruptCounter);
          
          pl_count = 248640;//reset
          ac_count = 250;
          fL_count = 200;
         
      break;
      case 1:  //가속.최고속도까지 가속 후 등속으로 이동.
        if(f_count>fL_count){
          f_count--;
           Serial.print("f_count:");
          Serial.println(f_count);
          digitalWrite(pin_en,LOW);
          }
        else{
          mo_state=2;
          pl_ncount=0;  //펄스카운트 초기화.
          Serial.print("mo_state=2,interruptCounter:");
          Serial.println(interruptCounter);
          }
      break;        
      case 2:  //등속.지정된 펄스까지만.등속운동한 후 감속한다.
        if(pl_ncount < pl_count){
            Serial.print("pl_ncount:");
          Serial.println(pl_ncount);
          }  
        else{
          mo_state=3;
          Serial.print("mo_state=3,interruptCounter:");
          Serial.println(interruptCounter);
          }
      break;  
      case 3://감속.최저속도까지.정지상태로 보내기.
        if(f_count<fH_count)
        {
          f_count++;
        Serial.print("f_count:");
          Serial.println(f_count);
        
        }
        
        else{
          mo_state=0;
          digitalWrite(pin_en,HIGH);
          Serial.print("mo_state=0,interruptCounter:");
          Serial.println(interruptCounter);
          
          }
      break;
    }
    f_i=0;
  }
  // 메인루프중 타이머가 종료되면 펄스를 1회 준다.
  // If Timer has fired
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
    uint32_t isrCount = 0, isrTime = 0;
    // Read the interrupt count and time  //이 쓰기작업을 할 때 타이머를 일시중지시키는 코드라고 이해함.
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);
    // Print it
/*  변수상태 확인
    Serial.print("onTimer no. ");
    Serial.print(isrCount);
    Serial.print(" at ");
    Serial.print(isrTime);
    Serial.print(" ms, interruptCounter:");
    Serial.println(interruptCounter);
*/
//인터럽트 카운터 증가.이 카운터로 모터 펄스를 카운트 할 수 있다.방향 및 정지 역방향..이동시 현재위치를 파악 할 수 있다.
//별도의 변수를 추가해서 다루기로 한다.
  interruptCounter++;

  //타이머 간격 변경 테스트--잘 됨100000=1초
  //timerAlarmWrite(timer, 100000*isrCount, true);

    //타이머알람 재조정.
    timerAlarmWrite(timer, f_count, true);
    //펄스 1회
    digitalWrite(pin_fulse,HIGH);
    digitalWrite(pin_fulse,LOW);

    //펄스카운트.펄스증가.
    //1)pin_en이 LOW일때 카운트하면 가감속까지 카운트 가능
    //2)mo_state=2일때 카운트하면 등속일때만 카운트 가능 <-------선택함.
    if(mo_state==2)
    {
      pl_ncount++;
 Serial.print("pl_ncount:");
          Serial.println(pl_ncount);
      }
 
  }
  /*
  // If button is pressed.반복,채터링방지등이 없다.--타이머로 바꿈.
  if (digitalRead(BTN_STOP_ALARM) == LOW) {
    // If timer is still running
    if (timer) {
      // Stop and free timer
      timerEnd(timer);
      timer = NULL;
    }
  }
  */
}
