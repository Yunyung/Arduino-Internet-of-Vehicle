/*
此程式庫不支援熱插拔，亦即，你必須在連接控制器後重新啟動Arduino，
或者在連接控制器之後再次呼叫config_gamepad()函數。
*/
/*
 * 已知危險: 有時候編譯會產生某某function不在scope之內的錯誤，或者是出現java的錯誤，重新編譯即可
 * 如果藍牙接線有錯也會影響到PS2
 */
#include <PS2X_lib.h>    // 匯入PS2函式庫
#include <SoftwareSerial.h>   // 引用藍芽用程式庫

// L298N 馬達驅動板
// 宣告 MotorA 為右邊
// 宣告 MotorB 為左邊

// L298N 馬達驅動板
#define MotorA_I1     8  //定義 I1 接腳
#define MotorA_I2     9  //定義 I2 接腳
#define MotorB_I3    10  //定義 I3 接腳
#define MotorB_I4    11  //定義 I4 接腳
#define MotorA_PWMA    5  //定義 ENA (PWM調速) 接腳
#define MotorB_PWMB    6  //定義 ENB (PWM調速) 接腳

// PS2 搖桿
#define PS2_DAT       A3  // 定義 DATA 接腳
#define PS2_CMD       A4  // 定義 COMMAND 接腳
#define PS2_ATT        7  // 定義 ATTENTION 接腳
#define PS2_CLK        4  // 定義 CLOCK 接腳

// 定義藍牙模組接腳
#define BT_Enable    A0
#define BT_Vcc    A1
#define base_speed 200

// 定義連接藍牙模組的序列埠
SoftwareSerial BT(12, 13); // 接收腳, 傳送腳

PS2X ps2x;    // 建立PS2控制器的類別實體

unsigned long ps2x_tick = 0;  // 宣告計時參數
int ps2x_error = 0;           // 宣告錯誤碼參數
int delay_time = 0;           // 共用的延遲時間
bool car_moving = false;	  // 紀錄車輛是否移動的flag
bool stick_L_moving = false;  // 紀錄左搖桿是否移動的flag
bool stick_R_moving = false;  // 紀錄右搖桿是否移動的flag
int moving_counting = 0;  // 紀錄已經移動的次數
char val;					  // 紀錄指令

void setup()
{
	
  Serial.begin(9600);  // 開啟 Serial port, 通訊速率為 9600 bps 
 
  // 設定馬達控制接腳模式
  pinMode(MotorA_I1,OUTPUT);
  pinMode(MotorA_I2,OUTPUT);
  pinMode(MotorB_I3,OUTPUT);
  pinMode(MotorB_I4,OUTPUT);
  pinMode(MotorA_PWMA,OUTPUT);
  pinMode(MotorB_PWMB,OUTPUT);
  // 設定藍牙接腳模式
  pinMode(BT_Enable,OUTPUT);
  pinMode(BT_Vcc,OUTPUT);
  
  // 預設藍牙為通電狀態
  // 預設為連線模式
  digitalWrite(BT_Enable,HIGH); 
  digitalWrite(BT_Vcc,HIGH);
  
  // 設定藍牙模組的連線速率
  // 如果是HC-05，請改成38400
  BT.begin(9600);
  
  // PS2控制器接腳設置; config_gamepad(時脈腳位, 命令腳位, 選取腳位, 資料腳位, 是否支援類比按鍵, 是否支援震動) 
  ps2x_error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT, false, false);
  
  //偵錯訊號換行
  //命令訊號不換行
  switch(ps2x_error)
  {
    case 0:  // 如果控制器連接沒有問題，就顯示底下的訊息。
      Serial.println("Found Controller, configured successful");
      break;
    case 1:  // 找不到控制器，顯示底下的錯誤訊息。
      Serial.println("No controller found, check wiring, see readme.txt to enable debug.");
      break;
    case 2:  // 發現控制器，但不接受命令，請參閱程式作者網站的除錯說明。
      Serial.println("Controller found but not accepting commands. see readme.txt to enable debug.");
      break;
    case 3:  // 控制器拒絕進入類比感測壓力模式，或許是此控制器不支援的緣故。
      Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
      break;
  }

  delay(1000); // 等待一秒讓遙控器連線
  ATmode_On(true); // 開啟AT MODE
  if(ps2x_error == 0){ // 找到控制器時
    BT.print("AT+ROLE=1\r\n"); // 設定為master
    while(BT.available()) { 
      val = BT.read();
      Serial.print(val);
    }
  }else{ // 找不到控制器時
    BT.print("AT+ROLE=0\r\n"); // 設定為slave
    while(BT.available()) { 
      val = BT.read();
      Serial.print(val);
    }
  }
  BT.print("AT+CMODE=1\r\n");	   // 固定設定CMODE為單一對象
  ATmode_On(false); // 關閉AT MODE
  analogWrite(MotorA_PWMA,200);    // 設定馬達 (右) 轉速
  analogWrite(MotorB_PWMB,200);    // 設定馬達 (左) 轉速
}

void loop()
{	
	static int stick_lx = 128;  // 左搖桿 X 軸數值變數
	static int stick_ly = 128;  // 左搖桿 Y 軸數值變數
	static int stick_rx = 128;  // 右搖桿 Y 軸數值變數
	static int stick_ry = 128;  // 右搖桿 Y 軸數值變數
	static double  vec_length = 0;
	static double  speed_rate = 0;

  //moving_count();
	// 在serial可用的時候從電腦接受特殊指令
	if (Serial.available()) {
		val = Serial.read();
	if(val == 'e'){	// 啟動AT-MODE
		digitalWrite(BT_Vcc,LOW);
		digitalWrite(BT_Enable,HIGH); 
		delay(1000);
		digitalWrite(BT_Vcc,HIGH);
		ATmode_On(true);
	}
	else if(val == 'd'){	//關閉AT-MODE
		digitalWrite(BT_Vcc,LOW);
		digitalWrite(BT_Enable,LOW);
		delay(1000);
		digitalWrite(BT_Vcc,HIGH);
		ATmode_On(false);
	}else if(val == 'r'){	//將role設定為master
		BT.print("AT+ROLE=1\r\n");
	}else 
		BT.print(val);	//將指定下給藍牙
	}
	// 藍牙echo偵錯
	if(BT.available()) { 
		val = BT.read();
    Serial.print("BT:");
    Serial.println(val);
    if(val == 'C'){            
      Serial.println("Reading coordinate:");
      int temp_stick_lx = stick_lx;
      int temp_stick_ly = stick_ly;
      stick_lx = BT.read();
      delay(1);
      stick_ly = BT.read();
      delay(1);
      if(stick_ly < 0 || stick_ly > 255 || stick_lx < 0 || stick_lx > 255){ // 在讀取錯誤時讓車子停止移動
        stick_ly = temp_stick_ly;
        stick_lx = temp_stick_lx;
      }else{
        stick_L_moving = true;
        Serial.print("Stick_lx:");
        Serial.println(stick_lx, DEC);
        Serial.print("Stick_ly:");
        Serial.println(stick_ly, DEC);
      }
    }
	}
	// val目前即為移動指令 分為WASD 和搖桿控制指令C
	// 當有controller時
	if(ps2x_error == 0){
		// 每隔25ms讀取一次按鍵
		if(millis() - ps2x_tick > 25)
		{
			ps2x_tick = millis();  // 紀錄目前時間
			ps2x.read_gamepad();   // 讀取控制器
      if(ps2x.ButtonPressed(PSB_SQUARE))  // 若「方形」按鍵被按下
      {
        Serial.println("\r\nPS2 button [SQUARE] pressed");
        BT.print('I');
        val = 'I';
      }

      if(ps2x.ButtonPressed(PSB_TRIANGLE))  // 若「三角形」按鍵被按下
      {
        Serial.println("\r\nPS2 button [TRIANGLE] pressed");
        BT.print('P');
        val = 'P';
      }
			//--------------------------------------------------------------
			//  按鍵操控 : 「上」前進、「下」後退、「左」左轉、「右」右轉
			//--------------------------------------------------------------
			if(ps2x.ButtonPressed(PSB_PAD_UP))  // 若「上」按鍵被按下
			{
				Serial.println("\r\nPS2 button [UP] pressed");
				BT.print('W');
				val = 'W';
			}
			if(ps2x.ButtonReleased(PSB_PAD_UP))  // 若「上」按鍵被放開
			{
				delay(1);  // 延遲1ms防止誤判
				ps2x.read_gamepad();  // 再次讀取控制器
				if(ps2x.Button(PSB_PAD_UP) == false)  // 確認「上」按鍵被放開
				{
					Serial.println("\r\nPS2 button [UP] released");
					BT.print('Q');
					val = 'Q';
				}      
			}
			if(ps2x.ButtonPressed(PSB_PAD_DOWN))  // 若「下」按鍵被按下
			{
				Serial.println("\r\nPS2 button [DOWN] pressed");
				BT.print("S");
				val = 'S';
			}
			if(ps2x.ButtonReleased(PSB_PAD_DOWN))  // 若「下」按鍵被放開
			{
				delay(1);   // 延遲1ms防止誤判
				ps2x.read_gamepad();  // 再次讀取控制器
				if(ps2x.Button(PSB_PAD_DOWN) == false)  // 確認「下」按鍵被放開
				{
					Serial.println("\r\nPS2 button [DOWN] released");
					BT.print("Q");
					val = 'Q';
				}
			}
			if(ps2x.ButtonPressed(PSB_PAD_LEFT))  // 若「左」按鍵被按下
			{
				Serial.println("\r\nPS2 button [LEFT] pressed");
				BT.print("A");
				val = 'A';
			}
			if(ps2x.ButtonReleased(PSB_PAD_LEFT))  // 若「左」按鍵被放開
			{
				delay(1);   // 延遲1ms防止誤判
				ps2x.read_gamepad();  // 再次讀取控制器
				if(ps2x.Button(PSB_PAD_LEFT) == false)  // 確認「左」按鍵被放開
				{
					Serial.println("\r\nPS2 button [LEFT] released");
					BT.print("Q");
					val = 'Q';
				}
			}
			if(ps2x.ButtonPressed(PSB_PAD_RIGHT))  // 若「右」按鍵被按下
			{
				Serial.println("\r\nPS2 button [RIGHT] pressed");
				BT.print("D");
				val = 'D';
			}
			if(ps2x.ButtonReleased(PSB_PAD_RIGHT))  // 若「右」按鍵被放開
			{
				delay(1);   // 延遲1ms防止誤判
				ps2x.read_gamepad();  // 再次讀取控制器  
				if(ps2x.Button(PSB_PAD_RIGHT) == false)  // 確認「右」按鍵被放開
				{
					Serial.println("\r\nPS2 button [RIGHT] released");
					BT.print("Q");
					val = 'Q';
				}
			}
			if(ps2x.Button(0xFFFF) == false){  // 如沒有按鍵, 則偵測搖桿
				if(stick_lx != ps2x.Analog(PSS_LX) || stick_ly != ps2x.Analog(PSS_LY)){  // 若左搖桿 X 或 Y 軸有變化
					if((ps2x.Analog(PSS_LY) < 10) || (ps2x.Analog(PSS_LY) > 250)){ // 預防誤判再次讀取
  					delay(1);   // 延遲1ms防止誤判
  					ps2x.read_gamepad();  // 再次讀取控制器
					}
					if(stick_ly != ps2x.Analog(PSS_LY)){  // 確認左搖桿 Y 軸有變化
						stick_L_moving = true;      
						stick_ly = ps2x.Analog(PSS_LY);  // 紀錄左搖桿 Y 軸數值
						Serial.print("PS2 [LEFT] stick Y value : ");
						Serial.println(stick_ly, DEC); 
					}
					if(stick_lx != ps2x.Analog(PSS_LX)){  // 確認左搖桿 X 軸有變化
						stick_L_moving = true;
						stick_lx = ps2x.Analog(PSS_LX);  // 紀錄左搖桿 X 軸數值
						Serial.print("PS2 [LEFT] stick X value : ");
						Serial.println(stick_lx, DEC); 
					}
          if(stick_L_moving){ //傳輸C指令和x,y座標
            // write 和print的差別!!
  					BT.print('C');
            //BT.print(' '); // 用空白來區隔
            //print會印出'1','2','7'而非127
  					BT.write(stick_lx);
            //BT.print(' '); // 用空白來區隔
  					BT.write(stick_ly);
          }
				}
        if(stick_ry != ps2x.Analog(PSS_RY)){ // 讀取右搖桿 右搖桿只會單一操控左右馬達 發出T G Q 指令
          if((ps2x.Analog(PSS_RY) < 10) || (ps2x.Analog(PSS_RY) > 250)){ // 預防誤判再次讀取
            delay(1);   // 延遲1ms防止誤判
            ps2x.read_gamepad();  // 再次讀取控制器
          }
          if(stick_ry != ps2x.Analog(PSS_RY)){  // 確認右搖桿 Y 軸有變化
            stick_R_moving = true;      
            stick_ry = ps2x.Analog(PSS_RY);  // 紀錄右搖桿 Y 軸數值
            Serial.print("PS2 [RIGHT] stick Y value : ");
            Serial.println(stick_ry, DEC); 
          }
          if(stick_R_moving){ 
      			if(stick_ry < 120){ // 右輪往前
      				BT.print("T");
      				val = 'T';
      			}else if(stick_ry > 140){ //左輪往前
      				BT.print("G");
      				val = 'G';
      			}else{
      				BT.print("Q");
      				val = 'Q';
      			}
          }
        }else{
          stick_R_moving = false;
        }
			}
		}
	}

	if(val == 'W'){ // 開始根據指令去移動車子
		Car_Advance(0);
		setFastBoth();
	}else if(val == 'S'){
		Car_Back(0);
		setFastBoth();
	}else if(val == 'A'){
		Car_TurnL(0);
		setFastBoth();
	}else if(val == 'D'){
		Car_TurnR(0);
		setFastBoth();
	}else if(val == 'I'){
    // square button -> 不停地跑正方形
    RunSquare();
	}
  else if(val == 'P'){
    // Triangle button -> 不停的跑三角形
    RunTriangle();
  }
	else if(val == 'T'){
		forwardR();
		setFastBoth();
	}else if(val == 'G'){
		forwardL();
		setFastBoth();
	}else if(val == 'Q'){
		if(car_moving || stick_R_moving){
			Car_Stop(0);
			car_moving = false;
			val = -1;
		}
	}

	if (stick_L_moving == true){ // 根據搖桿去移動車子
		stick_L_moving = false;
		// 和中心點相減
		int sub_lx = stick_lx-128;
		int sub_ly = stick_ly-128;
		// 取出搖桿向量長度
		vec_length = pow(pow(sub_lx,2) + pow(sub_ly,2) , 0.5 );
		//速度為比值開根號 這樣好像比較不會忽略較小的數值
		speed_rate = pow(vec_length / 128,1);
		Serial.print("Speed rate:");
		Serial.println(speed_rate); 

		double ang = atan2(sub_ly, sub_lx) * 180 / M_PI;
		if(ang > 90)
		  ang = 180 - ang;
		else if(ang < -90)
		  ang = 180 + ang;
		else if(ang >= -90 && ang < 0)
		  ang = -ang;
		Serial.println(ang);      
		
		// y為負的時候兩者皆為往前
		// x為負的時候左輪較慢
		if(sub_ly < 0)
		  Car_Advance(0);
		else
			Car_Back(0);
		// 根據stick_lx和stick_ly改變車速
		if(abs(stick_ly - 128) < 5 && abs(stick_lx - 128) < 5){
			setFastBoth();
  		if(car_moving)
  			Car_Stop(delay_time);
		}else if(sub_lx < 0){ // 左輪較慢
			double speed_a = base_speed * speed_rate;
			double speed_b = base_speed * speed_rate * ang / 90;
      Serial.print("Speed_a:");
      Serial.println(speed_a); 
      Serial.print("Speed_b:");
      Serial.println(speed_b); 
			analogWrite(MotorA_PWMA,speed_a > 255 ? 255 : speed_a); // 防止溢出
			analogWrite(MotorB_PWMB,speed_b > 255 ? 255 : speed_b);
			car_moving = true;
		}else{ // 右輪較慢
			double speed_a = base_speed * speed_rate * ang / 90 + 100;
			double speed_b = base_speed * speed_rate + 100;
			analogWrite(MotorA_PWMA,speed_a > 255 ? 255 : speed_a);
			analogWrite(MotorB_PWMB,speed_b > 255 ? 255 : speed_b);
			car_moving = true;
		}
	}
}
void moving_count(){
  if(car_moving)
    moving_counting++;
  else
    moving_counting = 0;
  if(moving_counting > 300){
    moving_counting = 0;
    val = 'Q';
  }
  //Serial.println(moving_counting);
}
void ATmode_On(int b)
{
	digitalWrite(BT_Vcc,LOW);
	digitalWrite(BT_Enable,b);
	delay(1000);
	digitalWrite(BT_Vcc,HIGH);
	Serial.print("AT-mode:");
	Serial.println(b);
}
// 小車繞正方形(實際為「走一段後右轉」loop
void RunSquare(){
  int delayTime = 0;
  
  Serial.print("Square Clicked\n");
  while(!ps2x.ButtonPressed(PSB_CROSS)){ // 當按下 X 按鈕 跳出跑正方形的迴圈
      if(BT.available()) { 
        val = BT.read();
        if (val == 'Q') break;
      }
      if (delayTime == 4){
         // 若直線跑了4秒(4次) 向右轉
         Serial.print("Turn right \n");
         Car_TurnR(0);
         delay(400);
         delayTime = 0;
      } 
//      setCustomL(200); // 其中一台車子速度要不同才會走直線(左右輪轉不平衡)
//      setCustomR(170);
      setFastBoth();
      Car_Advance(0);
      Serial.print("Advance \n");
      delay(1000);
      delayTime++;
      ps2x.read_gamepad();   // 讀取控制器
  }
  val = 'Q'; // 跳出迴圈後車子停止
  BT.print('Q');
}

// 小車繞三角形(實際為「走一段後右轉」loop
void RunTriangle(){
  int delayTime = 0;
  int T = 0;
  Serial.print("Triangle Click\n");
  while (T != 3){ // 當按下 X 按鈕 跳出跑正方形的迴圈
      if(BT.available()) { 
        val = BT.read();
        Serial.print("Stop Triangle \n");
        if (val == 'Q') break;
      }
//      setCustomL(200);
//      setCustomR(170);
      setFastBoth();
      Car_Advance(0);
      Serial.print("Advance \n");
      delay(1000);
      delayTime++;
      ps2x.read_gamepad();   // 讀取控制器
      if (delayTime == 3){
         // 若直線跑了3秒(3次) 向右轉
         Serial.print("Turn right \n");
         Car_TurnR(0);
         delay(500);
         delayTime = 0;
         T++;
      } 
  }
  Serial.print("Out Triangle loop \n");
  val = 'Q'; // 跳出迴圈後車子停止
  BT.print('Q');
}
// 小車前進
void Car_Advance(int t)
{
  forwardR();
  forwardL();
  delay(t * 100);
}
// 小車後退
void Car_Back(int t)
{
  backR();
  backL();
  delay(t * 100);  
}
// 小車左旋轉
void Car_TurnL(int t)
{
  forwardR();
  backL();
  delay(t * 100);
}
// 小車右旋轉
void Car_TurnR(int t)
{
  backR();
  forwardL();
  delay(t * 100);
}
// 小車停止
void Car_Stop(int t)
{
  stopR();
  stopL();
  car_moving = false;
  delay(t * 100);
}
void setFastL()
{
  analogWrite(MotorB_PWMB,base_speed);    // 設定馬達 (左) 轉速
}
void setFastR()
{
  analogWrite(MotorA_PWMA,base_speed);    // 設定馬達 (左) 轉速
}
void setFastBoth()
{
  setFastL();
  setFastR();
}
void setSlowL()
{
  analogWrite(MotorB_PWMB,100);    // 設定馬達 (左) 轉速
}
void setSlowR()
{
  analogWrite(MotorA_PWMA,100);    // 設定馬達 (左) 轉速
}
void setSlowBoth()
{
  setSlowL();
  setSlowR();
}
void forwardR()  // 右輪往前
{
  car_moving = true;
  digitalWrite(MotorA_I1,HIGH);
  digitalWrite(MotorA_I2,LOW);
}
void backR()  // 右輪往後
{
  car_moving = true;
  digitalWrite(MotorA_I1,LOW);
  digitalWrite(MotorA_I2,HIGH);
}
void stopR()  //右輪停止
{
    digitalWrite(MotorA_I1,HIGH);
    digitalWrite(MotorA_I2,HIGH);
}
void backL()  // 左輪往後
{
  car_moving = true;
  digitalWrite(MotorB_I3,LOW);
  digitalWrite(MotorB_I4,HIGH);
}
void forwardL()  // 左輪往前
{
  car_moving = true;
  digitalWrite(MotorB_I3,HIGH);
  digitalWrite(MotorB_I4,LOW);
}
void stopL()  //左輪停止
{
  digitalWrite(MotorB_I3,HIGH);
  digitalWrite(MotorB_I4,HIGH);
}

void setCustomL(int t)
{
  analogWrite(MotorB_PWMB,t);    // 設定馬達 (左) 轉速
}
void setCustomR(int t)
{
  analogWrite(MotorA_PWMA,t);    // 設定馬達 (左) 轉速
}
