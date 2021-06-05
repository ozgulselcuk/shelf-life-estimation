#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd( 0x27, 16, 2);

#define co2_sensor A0    // MQ135 sensor analog input value
#define temp_sensor  A1  // LM35 temperature sensor analog input value
#define buzzer A2        // BUZZER analog input value
#define co2zero 55       // MQ135 calibrate value 


const int red_led = 8;    //  RGB led variable
const int blue_led = 9;   //  RGB led variable
const int green_led = 10; //  RGB led variable

int co2now;   // co2_measure func variable
int co2comp;  // co2_measure func variable
int co2lvl;   // co2_measure func variable

int readtemp_value;    // temperature_measure func variable
float temp_volt = 0;   // temperature_measure func variable
float temperature = 0; // temperature_measure func variable

int RGB[3] = {red_led, blue_led, green_led}; // RGB led variable list

unsigned long now_second = 0;  // times func variable second counter
unsigned long past_second = 0; // times func variable second counter
unsigned long now_minute = 0;  // times func variable minute counter
unsigned long past_minute = 0; // times func variable minute counter
unsigned long oneday = 86400000; // the value of a day in microseconds

unsigned long now_error = 0;  // sensor error func variable time counter
unsigned long past_error = 0; // sensor error func variable time counter

int minute = 0;  // time func variable
int counter = 0; // time func variable

int sum_1min_co2 = 0;  // carbon dioxide values  sum measured over 1 minute
int sum_co2_10min = 0; // carbon dioxide values  sum measured over 10 minute
int temp_sum = 0;      // temperature values  sum measured over 1 minute
int temp_data;         // incoming value in func temperature_measure 
int co2_value;         // incoming value in func co2_measure 
int average1_min;      // times func variable 

int temp_mode;         

int temp_level;    // kind of temperature level freezer,fridge,room 0,1,2,3
int co2_level;     // carbon dioxida value

bool min_co2;     // machine_learn func variable 
bool max_co2;     // machine_learn func variable

int change = 0;      // machine_update func variable
int update_data = 0; // machine_update func variable

int co2_measure()  // This function calculate average co2 level
{    
  co2now = analogRead(co2_sensor); // value from sensor
  co2comp = co2now - co2zero;  // calibrated value
  co2lvl  = map(co2comp,0,1023,400,5000); // concentration of atmosfer levels
  return co2lvl; // measured carbone dioxade value
}


int temperature_measure() // This function measured temperature value (celcius)
{ 
  readtemp_value = analogRead(temp_sensor); // value from sensor
  temp_volt= (5000.0/1023.0) * readtemp_value;
  temperature = temp_volt/(10.0); // temperature value(Celcius)
  return temperature; 
}


int co2_control(int co2_value){
    /*
   This function check wrong karbon dioksit value of co2 sensor
   carbone dioxide value less 200 ppm or more 2000 ppm wrong value
    */
   bool wrong_measure = false;
  if ((co2_value < 200) or (co2_value > 2000)){
    wrong_measure = true ;
    return 0; // 0 value means sensor is error
  }
  else if (wrong_measure == false);{
    return co2_value; 
  }  
}


int temperature_control(int temp)
{
  /*
   {min_temperature, max temperature, return value}
    {-55, -1, "mode1" = > 1} between -55, -1 temp   mode1  => Freezer mode
    {-1, 10 , "mode1" = > 2} between -1, 10  temp   mode2  => Fridge mode
    {10, 55 , "mode1" = > 3} between 10, 55  temp   mode3  => room conditions mode   
   */
  bool wrong_measure = true;
  int conditions_temp [3][3] = {{-55, -1, 1},
                                {-1, 10 , 2},
                                {10, 55 , 3}};
     for (int a=0; a<3; a++)
   {
      if ((conditions_temp [a][0] < temp) and (temp <= conditions_temp [a][1]))
      {   wrong_measure = false;
          return conditions_temp[a][2];
      }
   }
   if (wrong_measure == true);{
    return 0; // 0 value means sensor is error   
   }                              
}

int sensor_error(int error_code)
{
  /*
     {101} error code means temperature sensor error
     {102} error code means carbone dioxide sensor error
     {103} error code means all  sensor error
   */
  lcd.home();
  now_error = millis();
  while (now_error-past_error > 180000) // 3 dakika boyunca 
  {
    if (error_code == 101)
        {
        lcd.print("Temp_sens_error");
        analogWrite(buzzer,255);
        }
    else if (error_code == 102)
        {
        lcd.print("co2_sens_error");
        analogWrite(buzzer,255);
        }
    else(error_code == 103);
        {
        lcd.print("All sensor error");
        analogWrite(buzzer,255);    
        }
  }
  past_error = now_error;
  analogWrite(buzzer,0); 
}


int sensor_control(int temp_value, int co2_value)
{
  if ((temp_value == 0) and (co2_value !=  0)) // temperature sensor error
    {
    return sensor_error(101); // 101 code means temperature sensor error
    }
  else if ((temp_value !=  0) and (co2_value == 0)) // carbondioksit sensor error
    {
    return sensor_error(102);  // 102 code means carbondioksit sensor error
    }
  else if ((temp_value == 0) and (co2_value ==  0)) // two sensor error
    {
    return sensor_error(103); // 103 code means two sensor error
    }
  else ((temp_value !=  0) and (co2_value !=  0)); // sensors working properly
    {
    return remain_co2_data(temp_value, co2_value); // no sensor error! 
    }  
}

int memory_save(int save_data, int temp_level)
/*
 This function performs saving according to the storage 
 space available according to the mode type.
 */
{ 
  int interval[4][2] = {{0, 0}, {0, 200}, {300, 500}, {600, 800}}; // areas to save in memory
  
    for (int k =interval[temp_level][0]; k<interval[temp_level][1]; k++)
    {
      if (EEPROM.read(k) == 0)
      {
        EEPROM.write(k,save_data);
        break; // not to write to memory constantly using break command
      }
    }  
}


int machine_learn(int temp_mode, int co2_value, int exp_data){
  if ( co2_value < 450){
    min_co2 = true;
  }
  if ( co2_value > 1000){
    max_co2 = true;
  }
  if ((min_co2 == true) and (max_co2 == true)) // both true exp_data must save EEEPROM memory
  {
    max_co2 = false; 
    min_co2 = false;
    return memory_save(exp_data,temp_level);  
  }
}


int time_check() // this function counter time ekle birşeyler daha
{
   now_minute = millis();
   now_second = millis();
   if (now_second - past_second >= 10000) // one per minute measuring
  { 
    co2_value = co2_measure(); // co2 level fonk 
     Serial.print(co2_value);
     Serial.println( " ppm");
    sum_1min_co2 = sum_1min_co2 + co2_value;     
    counter = counter+1;
    past_second = now_second;
         if (counter == 6)
        {
         average1_min = (sum_1min_co2/6);
         counter = 0;
         sum_1min_co2 = 0;
        }
    } 
    
            if (now_minute-past_minute >= 60000) // dakikada bir 
            { 
              sum_co2_10min = (sum_co2_10min + average1_min);
              temp_data = temperature_measure();
              temp_sum = temp_data+temp_sum;
              past_minute = now_minute; 
              minute++; 
               Serial.print(minute);
               Serial.print(".minute");
               Serial.println();
              if (minute == 10)
                  {
                    int average_temperature = 0;
                    int average_co2lvl = 0;
                    
                        average_temperature = temp_sum/10; 
                        average_co2lvl = (sum_co2_10min / 10); 
                         Serial.print("average temperature:");
                         Serial.println(average_temperature);
                        temp_level = temperature_control(average_temperature); // check sensor error
                        co2_level  = co2_control(average_co2lvl);  // check sensor error
  
                    minute = 0;
                    temp_sum = 0;
                    sum_co2_10min = 0;
                    if ((co2_level<450) or (co2_level > 1000))
                    {
                      if ((co2_level != 0) and (temp_level != 0)) // 0 means sensor error code
                      {
                      Serial.println("exp data");
                      int exp_data;
                      exp_data = (now_second/oneday);
                      machine_learn(co2_level, temp_level, exp_data);
                      }
                    }
                    
                    return sensor_control(temp_level, co2_level);
                  } 
            } 
}


int machine_data(int temp_mode){
  int interval[4][2] = {{0, 0}, {0, 200}, {300, 500}, {600, 800}};
  int sayac1 = 0;
  int sum_exp = 0;
  int update_data;
  for (int x = interval[temp_mode][0]; x < interval[temp_mode][1]; x++)
  {
    if (EEPROM.read(x) != 0)
    {
      sum_exp = sum_exp + EEPROM.read(x);
      sayac1++;     
    }
    else (EEPROM.read(x) == 0);
    {
      if (sum_exp == 0)
      {
         return 0;
      }
      else (sum_exp != 0);
      {
      update_data = (sum_exp/sayac1)*24;
        return update_data;
      }
    }
  }
}


int machine_update(int remain_list[12][3], int remain_data, int co2_lvl, int temp_lvl)
{
  update_data = machine_data(temp_lvl);
  if (update_data == 0)
  {
    for (int y=0; y<12; y++)
      {
      if (remain_list[y][0] < co2_lvl and co2_lvl <= remain_list[y][1])
          {
            return day_hour(remain_list[y][2], co2_lvl, temp_lvl);
          }
      }
  }
  else (update_data != 0); // güncellenmeli liste
  {
    remain_list[0][2] = update_data;
    for (int x = 1; x<12; x++)
      {
      change = remain_data-remain_list[0][x];
      remain_list[0][x] = update_data - change;    
      }

    for (int y=0; y<12; y++)
      {
      if (remain_list[y][0] < co2_lvl and co2_lvl <= remain_list[y][1])
          {
            return day_hour(remain_list[y][2], co2_lvl, temp_lvl);
          }
      } 
  }  
}


int remain_co2_data(int temp_lvl, int co2_lvl) // this function calculate  remainig time 
                                               // according to temp level
{
 Serial.print("co2: ");
 Serial.print(co2_lvl);
 Serial.print(" ppm");
 Serial.println();  
  int fridge[12][3];  // fridge mode
  int freezer[12][3]; // freezer mode
  int room[12][3];    // room mode
  if (temp_lvl == 1) //  freezer mode active
  {
    int remain_freezer = 2160; // 2160 hours equal 90 day
                               // value changes according to the status of data stored in memory
    
   int freezer[12][3] = {{300, 400, 2160},  {400, 450 , 1920}, //alt sınır-üst sınır-saat
                         {450, 500, 1680},  {500, 550 , 1440},
                         {550, 600, 1200},  {600, 650 ,  960},
                         {650, 700,  720},  {700, 750 ,  480},
                         {750, 800,  240},  {850, 900 ,  168},
                         {900, 950,   72},  {950, 1000,  24}};
   return machine_update(freezer, remain_freezer, co2_lvl, temp_lvl);
    
  }
  else if (temp_lvl == 2) // fridge mode active
  { 
     int remain_fridge = 168; // 168 hours equal 7 day
                              // value changes according to the status of data stored in memory
     int frigde[12][3] = {{300, 400, 168}, {400, 450, 144}, //{minco2, maxco2, remaining time}
                          {450, 500, 132}, {500, 550, 120}, 
                          {550, 600, 108}, {600, 650,  96},
                          {650, 700,  88}, {700, 750,  72},
                          {750, 800,  64}, {850, 900,  40},
                          {900, 950,  24}, {950, 1000, 12}};
     return machine_update(fridge, remain_fridge, co2_lvl, temp_lvl);
  }
                        
  else if (temp_lvl == 3) // room conditions mode active
  {
    int remain_room = 72; // 72 hours equal 3 day
                          // value changes according to the status of data stored in memory
    int room[12][3] =    {{300, 400, 72}, {400, 450, 64}, //{minco2, maxco2, remaining time}
                          {450, 500, 56}, {500, 550, 48},
                          {550, 600, 40}, {600, 650, 32},
                          {650, 700, 24}, {700, 750, 20},
                          {750, 800, 16}, {850, 900,  8},
                          {900, 950,  4}, {950, 1000, 1}};
         return machine_update(room, remain_room, co2_lvl, temp_lvl);
    
  }
}


int day_hour(int w,int co2_lvl, int temp_lvl){ // this function convert day,and hour
  int day_value  = w/24;
  int hour_value = w%24;
  return lcd_screen(day_value, hour_value, co2_lvl, temp_lvl);
}


int lcd_screen(int day, int hour,int co2_lvl, int temp_lvl){
  lcd.home();
  lcd.print("rt:");
  lcd.print(day);
  lcd.print(" Day ");
  lcd.print(hour);
  lcd.print(" Hour ");
  lcd.setCursor(0,1);
  lcd.print("co2: ");
  lcd.print(co2_lvl);
  lcd.print(" ppm ");
  lcd.print("md");
  lcd.print(temp_lvl);
  return RGB_func(co2_lvl);
}


int RGB_func(int co2_lvl)
{
  
        if ((co2_lvl > 800) and (co2_lvl < 1000))
        {
          digitalWrite(red_led,HIGH);
          digitalWrite(green_led,HIGH);
        }
        else if(co2_lvl < 800)
        {
          digitalWrite(blue_led,HIGH);
          digitalWrite(red_led,HIGH);
          digitalWrite(green_led,HIGH);
        }
        else if(co2_lvl > 1000)
        {
            digitalWrite(red_led,HIGH);
        }
}


void setup(){
  lcd.begin();
  Serial.begin(9600);
  pinMode(temp_sensor,INPUT);
  pinMode(co2_sensor,INPUT);
  pinMode(buzzer,INPUT);

  for (int led=0; led<3;led++)
    {
    pinMode(RGB[led],OUTPUT);
    }
 }


void loop(){
  time_check(); // main function of this project
}
