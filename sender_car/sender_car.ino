///////////////////////////////////////////////////////////////////////////////////////
//THIS IS A DEMO SOFTWARE JUST FOR EXPERIMENT PURPOER IN A NONCOMERTIAL ACTIVITY
//Version: 1.0 (AUG, 2016)

//Gyro - Arduino UNO R3
//VCC  -  5V
//GND  -  GND
//SDA  -  A4
//SCL  -  A5
//INT - port-2
#include <IRremote.h>

#include <SoftwareSerial.h>   // 引用程式庫
SoftwareSerial BT(0, 1); // 接收腳, 傳送腳

// L298N 馬達驅動板
#define MotorR_I1     A1  //定義 I1 接腳
#define MotorR_I2     A0 //定義 I2 接腳
#define MotorL_I3    A2//定義 I3 接腳
#define MotorL_I4    A3  //定義 I4 接腳
#define MotorR_ENA    6  //定義 ENA (PWM調速) 接腳
#define MotorL_ENB    5  //定義 ENB (PWM調速) 接腳
char F='F';
char B='B';
char L='L';
char R='R';
char S='S';
#define SPEED_R       100 //定義右輪速度 //左輪
#define SPEED_L       100 //定義左輪速度   //

// IRremote
#define IR_Recv      4   // 定義紅外線接收接腳
IRrecv irrecv(IR_Recv);  // 宣告 IRrecv 物件來接收紅外線訊號　
decode_results results;  // 宣告解碼變數

// IR Code
#define IR_Advence  0xFD807F  // 遙控器方向鍵 上, 前進
#define IR_Back     0xFD906F
#define IR_Left     0xFD20DF
#define IR_Right    0xFD609F
#define IR_Stop     0xFDA05F
#define mode1       0xFD08F7
#define mode2       0xFD8877
#define mode3       0xFD48B7

// AlphaBot2 OLED
#include <SPI.h>  
#include <Wire.h>  
#include <Adafruit_GFX.h>  
#include <Adafruit_SSD1306.h>  
#define OLED_RESET 9
#define OLED_SA0   8
Adafruit_SSD1306 display(OLED_RESET);  
  
#define NUMFLAKES 10  
#define XPOS 0  
#define YPOS 1  
#define DELTAY 2  

enum slope_direct {Flat, Uphill, Downhill};
slope_direct pre_slop_direct = Flat; 
int print_slope_counter = 0;
#include <Wire.h>
//Declaring some global variables
int gyro_x, gyro_y, gyro_z;
long gyro_x_cal, gyro_y_cal, gyro_z_cal;
boolean set_gyro_angles;

long acc_x, acc_y, acc_z, acc_total_vector;
float angle_roll_acc, angle_pitch_acc;

float angle_pitch, angle_roll;
int angle_pitch_buffer, angle_roll_buffer;
float angle_pitch_output, angle_roll_output;

long loop_timer;
int temp;

void setup() {
  BT.begin(38400);
  pinMode(MotorR_I1,OUTPUT);
  pinMode(MotorR_I2,OUTPUT);
  pinMode(MotorL_I3,OUTPUT);
  pinMode(MotorL_I4,OUTPUT);
  pinMode(MotorR_ENA,OUTPUT);
  pinMode(MotorL_ENB,OUTPUT);

  irrecv.enableIRIn();  // 啟動紅外線解碼

//   analogWrite(MotorR_ENA, SPEED_R);    //設定馬達 (右) 轉速
//   analogWrite(MotorL_ENB, SPEED_L);    //設定馬達 (左) 轉速

  /* AlphaBot2 OLED setup */
  // put your setup code here, to run once:  
  // 0x3C (for the 128x64) ;
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr
  // init done  
  display.setTextColor(WHITE);  
  // Show image buffer on the display hardware.  
  // Since the buffer is intialized with an Adafruit splashscreen  
  // internally, this will display the splashscreen.  
  display.display();  
  delay(2000);  
  // Clear the buffer.  
  display.clearDisplay();  





  /* MPU6050設定 */
  Wire.begin();                                                        //Start I2C as master
  setup_mpu_6050_registers();                                          //Setup the registers of the MPU-6050 
  for (int cal_int = 0; cal_int < 1000 ; cal_int ++){                  //Read the raw acc and gyro data from the MPU-6050 for 1000 times
    read_mpu_6050_data();                                             
    gyro_x_cal += gyro_x;                                              //Add the gyro x offset to the gyro_x_cal variable
    gyro_y_cal += gyro_y;                                              //Add the gyro y offset to the gyro_y_cal variable
    gyro_z_cal += gyro_z;                                              //Add the gyro z offset to the gyro_z_cal variable
    delay(3);                                                          //Delay 3us to have 250Hz for-loop
  }

  // divide by 1000 to get avarage offset
  gyro_x_cal /= 1000;                                                 
  gyro_y_cal /= 1000;                                                 
  gyro_z_cal /= 1000;         
  loop_timer = micros();                                               //Reset the loop timer
}

void loop(){

  read_mpu_6050_data();   
 //Subtract the offset values from the raw gyro values
  gyro_x -= gyro_x_cal;                                                
  gyro_y -= gyro_y_cal;                                                
  gyro_z -= gyro_z_cal;                                                
         
  //Gyro angle calculations . Note 0.0000611 = 1 / (250Hz x 65.5)
  angle_pitch += gyro_x * 0.0000611;                                   //Calculate the traveled pitch angle and add this to the angle_pitch variable
  angle_roll += gyro_y * 0.0000611;                                    //Calculate the traveled roll angle and add this to the angle_roll variable
  //0.000001066 = 0.0000611 * (3.142(PI) / 180degr) The Arduino sin function is in radians
  angle_pitch += angle_roll * sin(gyro_z * 0.000001066);               //If the IMU has yawed transfer the roll angle to the pitch angel
  angle_roll -= angle_pitch * sin(gyro_z * 0.000001066);               //If the IMU has yawed transfer the pitch angle to the roll angel
  
  //Accelerometer angle calculations
  acc_total_vector = sqrt((acc_x*acc_x)+(acc_y*acc_y)+(acc_z*acc_z));  //Calculate the total accelerometer vector
  //57.296 = 1 / (3.142 / 180) The Arduino asin function is in radians
  angle_pitch_acc = asin((float)acc_y/acc_total_vector)* 57.296;       //Calculate the pitch angle
  angle_roll_acc = asin((float)acc_x/acc_total_vector)* -57.296;       //Calculate the roll angle
  
  angle_pitch_acc -= 0.0;                                              //Accelerometer calibration value for pitch
  angle_roll_acc -= 0.0;                                               //Accelerometer calibration value for roll

  if(set_gyro_angles){                                                 //If the IMU is already started
    angle_pitch = angle_pitch * 0.9996 + angle_pitch_acc * 0.0004;     //Correct the drift of the gyro pitch angle with the accelerometer pitch angle
    angle_roll = angle_roll * 0.9996 + angle_roll_acc * 0.0004;        //Correct the drift of the gyro roll angle with the accelerometer roll angle
  }
  else{                                                                //At first start
    angle_pitch = angle_pitch_acc;                                     //Set the gyro pitch angle equal to the accelerometer pitch angle 
    angle_roll = angle_roll_acc;                                       //Set the gyro roll angle equal to the accelerometer roll angle 
    set_gyro_angles = true;                                            //Set the IMU started flag
  }
  
  //To dampen the pitch and roll angles a complementary filter is used
  angle_pitch_output = angle_pitch_output * 0.9 + angle_pitch * 0.1;   //Take 90% of the output pitch value and add 10% of the raw pitch value
  angle_roll_output = angle_roll_output * 0.9 + angle_roll * 0.1;      //Take 90% of the output roll value and add 10% of the raw roll value
//   Serial.print(" | Angle  = "); Serial.println(angle_pitch_output);
  


  OLED_print_slope(angle_pitch_output);
  /* 斜坡偵測 */
  if (angle_pitch_output >= 10) {
      // 上坡加速
      // Serial.print(" |上坡加速 Angle  = "); Serial.println(angle_pitch_output);
      analogWrite(MotorR_ENA, SPEED_R + 50);    //設定馬達 (右) 轉速
      analogWrite(MotorL_ENB, SPEED_L + 50);    //設定馬達 (左) 轉速
      // BT.println((char)'U');

  }
  else if (angle_pitch_output <= -10) {
      // 下坡減速
      // Serial.print(" |下坡減速 Angle  = "); Serial.println(angle_pitch_output);
      analogWrite(MotorR_ENA, SPEED_R - 30);    //設定馬達 (右) 轉速
      analogWrite(MotorL_ENB, SPEED_L - 30);    //設定馬達 (左) 轉速
      // BT.println((char)'D');

  }
  else {
      // 平地正常速度
      // Serial.print(" |平地正常速度  = "); Serial.println(angle_pitch_output);
      analogWrite(MotorR_ENA, SPEED_R);    //設定馬達 (右) 轉速
      analogWrite(MotorL_ENB, SPEED_L);    //設定馬達 (左) 轉速
      // BT.println((char)'N');
  }


  // 每跑N次才顯示slope, 方便debugger
  print_slope_counter++;
  if (print_slope_counter == 10) {
    Serial.println(angle_pitch_output); // print slope for debugger
    print_slope_counter = 0;
  }

  /* 紅外線+藍芽傳送     */
  if(irrecv.decode(&results))
  {
        Serial.print(results.value, HEX);    // 紅外線編碼

        switch(results.value)
        {
            case IR_Advence:
                Serial.print("車子前進"); 
                BT.print((char)F);
                advance(0);
                break;
            case IR_Back:
                Serial.print("車子後退"); 
                BT.write((char)B);
                back(0);
                break;
            case IR_Left:
                Serial.print("車子左轉"); 
                BT.println((char)L);
                turnL(0);
                break; 
            case IR_Right:
                Serial.print("車子右轉"); 
                BT.println((char)R);
                //advance(3);
                turnR(0);  
                break; 
            case IR_Stop:
                Serial.print("車子停止"); 
                BT.println((char)S);
                stopRL(0);
                break; 
            default:
                Serial.print(" Unsupported");
        }
        irrecv.resume(); // Receive the next value
  }





  while(micros() - loop_timer < 4000);                                 //Wait until the loop_timer reaches 4000us (250Hz) before starting the next loop
  loop_timer = micros();//Reset the loop timer
  
}

void back(int a)    // 前進
{
    digitalWrite(MotorR_I1,HIGH);   //馬達（右）順時針轉動
    digitalWrite(MotorR_I2,LOW);
    digitalWrite(MotorL_I3,HIGH);   //馬達（左）逆時針轉動
    digitalWrite(MotorL_I4,LOW);
    delay(a * 100);
}

void turnR(int d)    //右轉
{
    digitalWrite(MotorR_I1,LOW);    //馬達（右）逆時針轉動
    digitalWrite(MotorR_I2,HIGH);
    digitalWrite(MotorL_I3,HIGH);   //馬達（左）逆時針轉動
    digitalWrite(MotorL_I4,LOW);
    delay(d * 100);
}

void turnL(int e)    //左轉
{
    digitalWrite(MotorR_I1,HIGH);   //馬達（右）順時針轉動
    digitalWrite(MotorR_I2,LOW);
    digitalWrite(MotorL_I3,LOW);    //馬達（左）順時針轉動
    digitalWrite(MotorL_I4,HIGH);
    delay(e * 100);
}

void stopRL(int f)  //停止
{
    digitalWrite(MotorR_I1,HIGH);   //馬達（右）停止轉動
    digitalWrite(MotorR_I2,HIGH);
    digitalWrite(MotorL_I3,HIGH);   //馬達（左）停止轉動
    digitalWrite(MotorL_I4,HIGH);
    delay(f * 100);
}

void advance(int g)    //後退
{
    digitalWrite(MotorR_I1,LOW);    //馬達（右）逆時針轉動
    digitalWrite(MotorR_I2,HIGH);
    digitalWrite(MotorL_I3,LOW);    //馬達（左）順時針轉動
    digitalWrite(MotorL_I4,HIGH);
    delay(g * 100);
}

void setup_mpu_6050_registers(){
  //Activate the MPU-6050
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x6B);                                                    //Send the requested starting register
  Wire.write(0x00);                                                    //Set the requested starting register
  Wire.endTransmission();                                             
  //Configure the accelerometer (+/-8g)
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x1C);                                                    //Send the requested starting register
  Wire.write(0x10);                                                    //Set the requested starting register
  Wire.endTransmission();                                             
  //Configure the gyro (500dps full scale)
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x1B);                                                    //Send the requested starting register
  Wire.write(0x08);                                                    //Set the requested starting register
  Wire.endTransmission();                                             
}

void OLED_print_slope(int slope) {
  // 判斷需不需要改變顯示 => 因為顯示OLED會花時間 會影響MPU6050斜度計算
  slope_direct now_slope_direct;
  if (slope >= 10) now_slope_direct = Uphill;
  else if (slope <= -10) now_slope_direct = Downhill;
  else now_slope_direct = Flat;

  /* OLED 顯示出斜度 */
  if (now_slope_direct != pre_slop_direct){
    display.clearDisplay();  
    display.setTextSize(1);             //設置字體大小  
    display.setCursor(0,0);  
    display.print("Slope:"); 
    display.setTextSize(2);
    display.setCursor(0,10); 
    if (slope >= 10) display.print("Uphill");  
    else if (slope <= -10) display.print("Downhill");  
    else display.print("flat");
    blueToothSlope_Transmit(slope);
    display.display(); 
    pre_slop_direct = now_slope_direct;
  }
}

void blueToothSlope_Transmit(int slope) {
    if (slope >= 10) BT.println((char)'U');
    else if (slope <= -10) BT.println((char)'D');     
    else BT.println((char)'N');
}

void read_mpu_6050_data(){                                             //Subroutine for reading the raw gyro and accelerometer data
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x3B);                                                    //Send the requested starting register
  Wire.endTransmission();                                              //End the transmission
  Wire.requestFrom(0x68,14);                                           //Request 14 bytes from the MPU-6050
  while(Wire.available() < 14);                                        //Wait until all the bytes are received
  acc_x = Wire.read()<<8|Wire.read();                                  
  acc_y = Wire.read()<<8|Wire.read();                                  
  acc_z = Wire.read()<<8|Wire.read();                                  
  temp = Wire.read()<<8|Wire.read();                                   
  gyro_x = Wire.read()<<8|Wire.read();                                 
  gyro_y = Wire.read()<<8|Wire.read();                                 
  gyro_z = Wire.read()<<8|Wire.read();                                 
}













