
#include <Servo.h> 

// L298N 馬達驅動板
// 宣告 MotorA 為右邊
// 宣告 MotorB 為左邊

#define MotorA_I1     8  //定義 I1 接腳
#define MotorA_I2     9  //定義 I2 接腳
#define MotorB_I3    10  //定義 I3 接腳
#define MotorB_I4    11  //定義 I4 接腳
#define MotorA_PWMA    5  //定義 ENA (PWM調速) 接腳
#define MotorB_PWMB    6  //定義 ENB (PWM調速) 接腳

// 循線模組
#define SensorLeft    A2  //定義 左感測器 輸入腳
#define SensorCenter  A1  //定義 中感測器 輸入腳
#define SensorRight   A0  //定義 右感測器 輸入腳
int allWhiteTime = 0;
int straight = 0;
int direction_before = 0;
#define LEFT_BEFORE 1
#define RIGHT_BEFORE 2
// HC-SR04 超音波測距模組
#define US_Trig  13  //定義超音波模組 Trig 腳位
#define US_Echo  12  //定義超音波模組 Echo 腳位

// 伺服馬達(舵機)
#define Servo_Pin      3  // 定義伺服馬達輸出腳位(PWM)
#define servo_delay  250  // 伺服馬達轉向後的穩定時間
Servo myservo;            // 宣告伺服馬達變數
// 定義小車移動方向
#define Fgo  8  // 前進
#define Rgo  6  // 右轉
#define Lgo  4  // 左轉
#define Bgo  2  // 倒車

//初值設定
void setup() //初始化
{
  Serial.begin(9600); // 開啟 Serial port, 通訊速率為 9600 bps
  
  pinMode(MotorA_I1,OUTPUT); //設定數位腳8為輸出埠
  pinMode(MotorA_I2,OUTPUT); //設定數位腳9為輸出埠
  pinMode(MotorB_I3,OUTPUT); //設定數位腳10為輸出埠
  pinMode(MotorB_I4,OUTPUT); //設定數位腳11為輸出埠
  pinMode(MotorA_PWMA,OUTPUT); //設定數位腳5為輸出埠
  pinMode(MotorB_PWMB,OUTPUT); //設定數位腳6為輸出埠
  
  pinMode(SensorLeft,   INPUT); //設定數位腳A2為輸入埠
  pinMode(SensorCenter, INPUT); //設定數位腳A1為輸入埠
  pinMode(SensorRight,  INPUT); //設定數位腳A0為輸入埠

  pinMode(US_Trig, OUTPUT); //設定數位腳13為輸出埠
  pinMode(US_Echo, INPUT); //設定數位腳12為輸入埠

  myservo.attach(Servo_Pin);
  
  updateStraightSpeed();
}


void advance(int a)    // 小車前進
{
    digitalWrite(MotorA_I1,HIGH);   //馬達（右）順時針轉動
    digitalWrite(MotorA_I2,LOW);
    digitalWrite(MotorB_I3,LOW);   //馬達（左）逆時針轉動
    digitalWrite(MotorB_I4,HIGH);
    delay(a * 100);
}

void turnR(int d)    //小車右轉
{
    digitalWrite(MotorA_I1,LOW);    //馬達（右）逆時針轉動
    digitalWrite(MotorA_I2,HIGH);
    digitalWrite(MotorB_I3,LOW);   //馬達（左）逆時針轉動
    digitalWrite(MotorB_I4,HIGH);
    delay(d * 100);
}

void turnL(int e)    //小車左轉
{
    digitalWrite(MotorA_I1,HIGH);   //馬達（右）順時針轉動
    digitalWrite(MotorA_I2,LOW);
    digitalWrite(MotorB_I3,HIGH);    //馬達（左）順時針轉動
    digitalWrite(MotorB_I4,LOW);
    delay(e * 100);
}    

void stopRL(int f)  //小車停止
{
    digitalWrite(MotorA_I1,HIGH);   //馬達（右）停止轉動
    digitalWrite(MotorA_I2,HIGH);
    digitalWrite(MotorB_I3,HIGH);   //馬達（左）停止轉動
    digitalWrite(MotorB_I4,HIGH);
    delay(f * 100);
}

void back(int g)    //小車後退
{
    analogWrite(MotorA_PWMA,100);    //設定馬達 (右) 轉速
    analogWrite(MotorB_PWMB,100);     //設定馬達 (左) 轉速
    digitalWrite(MotorA_I1,LOW);    //馬達（右）逆時針轉動
    digitalWrite(MotorA_I2,HIGH);
    digitalWrite(MotorB_I3,HIGH);    //馬達（左）順時針轉動
    digitalWrite(MotorB_I4,LOW);
    delay(g * 100);     
}
    
void detection()   
{
    // 超音波偵測函數 三個角度讀取距離數值
    int F_Distance = 0;
    int R_Distance = 0;
    int L_Distance = 0;
    F_Distance = Ask_Distance(90);  // 讀取前方距離, 預設為90度,若超音波感測器轉至小車正前方後發現角度有些許偏差,您可以透過本處宣告其他角度進行調整。
    if (straight == 1) straight = 0;
    if(F_Distance < 5)  // 超音波感測器偵測後得到的距離低於 5公分, 離障礙物太近->後退
    {
         stopRL(0);
         back(3);
    }
 
    if(F_Distance < 9)  // 超音波感測器偵測前方的距離小於25公分
    {
        stopRL(0);
        straight = 1;
        R_Distance = Ask_Distance(0);    // 讀取右方距離(實際狀況調整舵機右轉為0度)

        if ((L_Distance < 5) && (R_Distance < 5))  //假如超音波感測器偵測到左邊距離和右邊距離皆小於5公分
        {
          back(1);                   // 倒退(車)
          analogWrite(MotorA_PWMA,80);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,60);     //設定馬達 (左) 轉速
          turnL(1);                  // 稍微向左方移動(防止卡在死巷裡)
          Serial.print(" back ");  // 顯示方向(倒退)
        }
        else if(R_Distance < 15)  // 右邊有障礙物
        {

          analogWrite(MotorA_PWMA,180);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,180);     //設定馬達 (左) 轉速
          turnL(1);                 // 左轉
          direction_before = LEFT_BEFORE;
          Serial.print(" Left ");    // 顯示方向(左轉) 
          // 轉回正向 並確認是否轉錯邊
          F_Distance = Ask_Distance(90);
          if (straight == 1) straight = 0;
        }
        else
        {
          analogWrite(MotorA_PWMA,130);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,130);     //設定馬達 (左) 轉速
          turnR(1);                 // 右轉
          direction_before = RIGHT_BEFORE;
          Serial.print(" RIGHT ");    // 顯示方向(左轉) 

          // 轉回正向 並確認是否轉錯邊
          F_Distance = Ask_Distance(90);
          if (straight == 1) straight = 0;
        }         
    }
    else
    {
        // 讀取感測器狀態
      boolean SL = digitalRead(SensorLeft);
      boolean SC = digitalRead(SensorCenter);
      boolean SR = digitalRead(SensorRight);

      if((SL == LOW) && (SC == LOW) && (SR == LOW))
      {
          allWhiteTime++;

        if (allWhiteTime == 3)
        {
          do
          {
            if (direction_before == LEFT_BEFORE)
            {
              analogWrite(MotorA_PWMA,60);    //設定馬達 (右) 轉速
              analogWrite(MotorB_PWMB,80);    //設定馬達 (左) 轉速
              
              turnR(2);
              back(1);
            }
            else if (direction_before == RIGHT_BEFORE)
            {
              analogWrite(MotorA_PWMA,80);    //設定馬達 (右) 轉速
              analogWrite(MotorB_PWMB,60);    //設定馬達 (左) 轉速
             
              turnL(2);
              back(1);
            }

            if (direction_before == 0)
            {
              turnL(1);
              back(3);
            }
            SC = digitalRead(SensorCenter);
            SL = digitalRead(SensorLeft);
            SR = digitalRead(SensorRight);
          }
          while((SC == LOW) && (SL == LOW) && (SR == LOW));
          allWhiteTime = 0;
          
          stopRL(0);
          direction_before = 0;
        }
        else
        {
          updateStraightSpeed();
          advance(0);     
        }
      }
      else if(SC == HIGH)  //中感測器在黑色區域
      {
        allWhiteTime = 0;
        if((SL == LOW) && (SR == HIGH))  // LCR=011 白黑黑, 車體偏右校正  
        {
          analogWrite(MotorA_PWMA,220);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,255);    //設定馬達 (左) 轉速
          advance(0);
        } 
        else if((SL == HIGH) && (SR == LOW))  // LCR=110 黑黑白, 車體偏左校正 
        {
          analogWrite(MotorA_PWMA,255);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,220);     //設定馬達 (左) 轉速
          advance(0);
        }
        else  //  LCR=010 OR LCR=111 其他, 直走
        {
          updateStraightSpeed();
          advance(0);
        }
      } 
      else // 中感測器在白色區域, 車體已大幅偏離黑線 C=0
      {
        allWhiteTime = 0;
        if((SL == LOW) && (SR == HIGH))  // LCR=001 白白黑, 車體快速右轉 
        {
          back(2);
          analogWrite(MotorA_PWMA,40);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,40);    //設定馬達 (左) 轉速
          
          turnR(1);
        }
        else if((SL == HIGH) && (SR == LOW))  // LCR=100  黑白白, 車體快速左轉 
        {
          back(2);
          analogWrite(MotorA_PWMA,40);    //設定馬達 (右) 轉速
          analogWrite(MotorB_PWMB,40);    //設定馬達 (左) 轉速
         
          turnL(1);
        }
      } 
        
    }
    return;
}    

int Ask_Distance(int dir)  // 測量距離 
{
    myservo.write(dir);  // 調整超音波模組角度
    if (straight == 1)
      delay(servo_delay);  // 等待伺服馬達穩定
    
    digitalWrite(US_Trig, LOW);   // 超聲波發射低電壓
    delayMicroseconds(2);         //2μs
    digitalWrite(US_Trig, HIGH);  // 讓超聲波發射高電壓
    delayMicroseconds(10);        //10μs，這裡至少是10μs
    digitalWrite(US_Trig, LOW);   // 維持超聲波發射低電壓
    float distance = pulseIn(US_Echo, HIGH);  // 讀取時間差
    distance = distance / 5.8 / 10;  // 將時間轉為距離距离（單位：公分）
    Serial.print("Distance:"); //輸出距離（單位：公分）
    Serial.println(distance); //顯示距離
    Serial.print('\n');
    return distance;
}
void updateStraightSpeed()
{
  analogWrite(MotorA_PWMA,235);    //設定馬達 (右) 轉速
  analogWrite(MotorB_PWMB,255);    //設定馬達 (左) 轉速
}
void loop()
{
    detection();
}
