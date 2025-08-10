#include <Stepper.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// Declaration for SSD1306 display connected using software SPI (default case):
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define readpot1 A0 // Right Silver Pot
#define readpot2 A1 // Left Black Pot
#define servopin 3 // Servo for endeffector
#define lim1 19 // limit switch for motor 1
#define lim2 18 // limit switch for motor 2
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define p1 22 // Up Red
#define p2 23 // Down Orange
#define p3 24 // Left Blue
#define p4 25 // Unused
#define p5 26 // Center Yellow
#define p6 27 // Right Green

enum robot_states{
  MENU,
  MANUAL_M,
  AUTO_M,
  HOMING,
  PROGRAMMING,
};

robot_states robot_state = MENU;

int file_number = 0;
int pos = 0;    // variable to store the servo position
// Define the number of steps per revolution for your motors (e.g., 2048 for 28BYJ-48)
const int stepsPerRevolution = 2048;
int stepcount = 0;
int max1step = 1700; // Analog manual control max value for motor 1 (X axis motor)
int min1step = 100; // Analog manual control min value for motor 1 (X axis motor)
int max2step = 2000; // Analog manual control max value for motor 2 (Y axis motor)
int min2step = 0; // Analog manual control min value for motor 2 (Y axis motor)
int desm1location = 0; // Desired Motor 1 location
int desm2location = 0; // Desired Motor 2 location
int prevdesm1location = 0; // Previous desired Motor 1 location
int prevdesm2location = 0; // Previous desired Motor 2 location
int m1location = 0;
int m2location = 0;
int m1moveval = 0;
int m2moveval = 0;
int m1velocity = 10;
int m2velocity = 8;
int refreshrate = 50;
int refreshcount = 0;
float currentpointX = 0;
float currentpointY = 0;
bool runonce = false;
bool savetime = false;
int servomode = 0;
int show_ops = true;
int prog_num = 0;
float waittime = 0.5;

const int ROWS = 30;
const int COLS = 3;
float progArray[ROWS][COLS] = {-1};
// const int EEPROM_SIZE = 1024;  // Adjust based on your EEPROM size
// Define the starting addresses for the three EEPROM arrays
const int EEPROM_ARRAY1_START = 0;
const int EEPROM_ARRAY2_START = EEPROM_ARRAY1_START + sizeof(progArray);
const int EEPROM_ARRAY3_START = EEPROM_ARRAY2_START + sizeof(progArray);


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo myservo;  // create servo object to control a servo
// Initialize the stepper library on the pins
Stepper motor1(stepsPerRevolution, 8, 10, 9, 11);  // Motor 1
Stepper motor2(stepsPerRevolution, 4, 6, 5, 7);    // Motor 2

void movemotors(int motor1Steps, int motor2Steps);
int sign(int numb);


void setup() {
  Serial.begin(9600);
  pinMode(p1, INPUT_PULLUP);
  pinMode(p2, INPUT_PULLUP);
  pinMode(p3, INPUT_PULLUP);
  pinMode(p4, INPUT_PULLUP);
  pinMode(p5, INPUT_PULLUP);
  pinMode(p6, INPUT_PULLUP);
  // initialize with the I2C addr 0x3C (for the 128x64)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  // Clear the buffer
  display.clearDisplay();
  
  // Test code
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,20);     // Start at top-left corner
  display.println(F("Starting  Robot..."));
  
  display.display();      // Show initial text

  display.clearDisplay();
  
  // Set the speed of the motors (you can adjust this to suit your motors and application)
  motor1.setSpeed(10);  // 10 RPM
  motor2.setSpeed(10);  // 10 RPM
 
  pinMode(servopin,OUTPUT);
  pinMode(lim1,INPUT_PULLUP);
  pinMode(lim2,INPUT_PULLUP);
    myservo.attach(3);  // attaches the servo on pin 9 to the servo object
    myservo.write(15);

    // Move motor 1 the specified number of steps
    motor1.step(-100);
    // Move motor 2 the specified number of steps
    motor2.step(-100);

  while(digitalRead(lim1)==1){
    int motor2Steps=1;
    motor2.step(motor2Steps);
  }
  delayMicroseconds(10000);
  while(digitalRead(lim2)==1){
    int motor1Steps=1;
    motor1.step(motor1Steps);
  }
  delayMicroseconds(10000);
  motor1.step(-100);


  display.clearDisplay();
  display.display();
  delayMicroseconds(10000);
  display.setTextSize(1);
}

void loop() {
 


  switch (robot_state){
    case MENU:
        myservo.write(15); 
        display.clearDisplay();
        display.setCursor(0,0);     // Start at top-left corner
        display.println("Main Menu");
        display.setCursor(45,17); 
        display.print("MANUAL");
        display.setCursor(58,27); 
        display.print("O");
        display.setCursor(10,35); 
        display.print("HOME O");
        display.setCursor(75,35); 
        display.print("O AUTO");
        display.setCursor(58,47); 
        display.print("O");
        display.setCursor(42,57); 
        display.print("PROGRAM");
        display.display();      // Show initial text
        if (!digitalRead(p1)){
          robot_state = MANUAL_M;
          display.setTextSize(2);
          display.clearDisplay();
          display.setCursor(0,17);     // Start at top-left corner
          display.println("Switching to Manual Mode");
          display.display();      // Show initial text
          delay(1500);
          display.clearDisplay();
          display.display();
          display.setTextSize(1);
          runonce = false;
          }
      else if (!digitalRead(p3)){
        home_fun();
      }
      else if (!digitalRead(p2)){
        robot_state = PROGRAMMING;
        display.setTextSize(2);
        display.clearDisplay();
        display.setCursor(0,0);     // Start at top-left corner
        display.println("Switching to ");
        display.println("Program   Mode");
        display.display();      // Show initial text
        delay(1500);
        display.clearDisplay();
        display.display();
        display.setTextSize(1);
        runonce = false;
      }
      else if (!digitalRead(p6)){
        robot_state = AUTO_M;
        display.setTextSize(2);
        display.clearDisplay();
        display.setCursor(0,0);     // Start at top-left corner
        display.println("Switching to ");
        display.println("Auto Mode");
        display.display();      // Show initial text
        delay(1500);
        display.clearDisplay();
        display.display();
        display.setTextSize(1);
        runonce = false;
      }
      break;
    case MANUAL_M:
      movemotorsman();
      if (!digitalRead(p5)){
        robot_state = MENU;
        myservo.write(15); 
        display.clearDisplay();
        display.setCursor(0,17);     // Start at top-left corner
        display.setTextSize(2);
        display.println("Switching to");
        display.println("Main Menu");
        display.display();      // Show initial text
        delay(1500);
        display.clearDisplay();
        display.display();
        display.setTextSize(1);
      }
      else if (!digitalRead(p1)){
        myservo.write(120); 
      }
      else if (!digitalRead(p6)){
        myservo.write(15); 
      }
      else if (!digitalRead(p2)){
        myservo.write(120); 
        delay(250);
        myservo.write(15); 
      }
      else if (!digitalRead(p3)){
        home_fun();
      }
      else if (runonce==false){
        myservo.write(15);
        motor1.setSpeed(10);  // 10 RPM
        motor2.setSpeed(10);  // 10 RPM
        display.clearDisplay();
        display.setCursor(0,0);     // Start at top-left corner
        display.println("Manual Mode");
        display.setCursor(1,17); 
        display.print("YMot-O");
        display.setCursor(45,17); 
        display.print("PUSH");
        display.setCursor(81,17); 
        display.print("O-XMot");
        display.setCursor(53,27); 
        display.print("O");
        display.setCursor(6,35); 
        display.print("HOME O");
        display.setCursor(48,35); 
        display.print("esc");
        display.setCursor(70,35); 
        display.print("O RELEASE");
        display.setCursor(53,47); 
        display.print("O");
        display.setCursor(34,57); 
        display.print("QUICK TAP");
        display.display();      // Show initial text
        runonce = true;
      }
      
      break;
    case AUTO_M:
      if (runonce == false){
        zeroprog();
        myservo.write(15); 
        display.clearDisplay();
        display.setCursor(0,0);     // Start at top-left corner
        display.setTextSize(1);
        display.println("Auto Mode");
        display.setCursor(0,16); 
        display.print("Select A File:");
        display.setCursor(47,25); 
        display.print("O Program 1");
        display.setCursor(6,37); 
        display.print("esc O");
        display.setCursor(47,37); 
        display.print("O Program 2");
        display.setCursor(47,49); 
        display.print("O Program 3");
        display.display();
        file_number = 0;
      }
      while (file_number==0){
        if (!digitalRead(p3)){
          robot_state = MENU;
          file_number = 4;
          runonce = false;
          display.clearDisplay();
          display.setCursor(0,0); 
          display.setTextSize(2); 
          display.println("Exiting");
          display.println("Auto Mode");
          display.display();
          display.setTextSize(1); 
          delay(1000);
          break;
        }
        else if (!digitalRead(p1)){
          file_number = 1;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.println("Loading Program 1");
          display.display();
          delay(1000);
        }
        else if (!digitalRead(p5)){
          file_number = 2;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.println("Loading Program 2");
          display.display();
          delay(1000);
        }
        else if (!digitalRead(p2)){
          file_number = 3;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.println("Loading Program 3");
          display.display();
          delay(1000);
        }
      }
      if (file_number!=4){
        display.clearDisplay();
        display.setCursor(0,0);     // Start at top-left corner
        display.setTextSize(1);
        display.println("Auto Mode");
        display.setCursor(0,16); 
        display.print("Run File:");
        display.print(file_number);
        display.setCursor(47,25); 
        display.print("O Once");
        display.setCursor(6,37); 
        display.print("esc O");
        display.setCursor(47,37); 
        display.print("O Twice");
        display.setCursor(47,49); 
        display.print("O Forever");
        display.display();
        bool pickoption = true;

        int runtimes = 0;
        while(pickoption){
          if(!digitalRead(p1)){
            // Run once
            pickoption = false;
            runtimes = 1;
            zeroprog();
            delay(500);
          }
          if(!digitalRead(p5)){
            // Run twice
            pickoption = false;
            runtimes = 2;
            zeroprog();
            delay(500);
          }
          if(!digitalRead(p2)){
            // Run forever
            pickoption = false;
            runtimes = 3;
            zeroprog();
            delay(500);
          }
          if(!digitalRead(p3)){
            display.clearDisplay();
            display.setCursor(0,0);     // Start at top-left corner
            display.setTextSize(2);
            display.println("Quitting");
            display.print("Auto Mode");
            display.display();
            display.setTextSize(1); 
            delay(1000);
            robot_state = MENU;
            home_fun();
            myservo.write(15); 
            display.clearDisplay();
            display.setCursor(0,17);     // Start at top-left corner
            display.setTextSize(2);
            display.println("Switching to");
            display.println("Main Menu");
            display.display();      // Show initial text
            delay(1500);
            display.clearDisplay();
            display.display();
            display.setTextSize(1);
            show_ops = true;
            prog_num = 0;
            servomode = 0;
            file_number = 4;
            pickoption = false;

          }
        }
        if (file_number!= 4){
          home_fun();
          readArrayFromEEPROM(progArray, file_number);
          motor1.setSpeed(10);  // 10 RPM
          motor2.setSpeed(10);  // 10 RPM
          display.clearDisplay();
          display.setCursor(0,0);     // Start at top-left corner
          display.println("Running Auto Mode");
          display.display();

          if (runtimes<3){
            delay(1500);
            for (int k = 0;k<runtimes;k++){
              for (prog_num = 0;prog_num<ROWS;prog_num++){
                float goalx = progArray[prog_num][0];
                float goaly = progArray[prog_num][1];
                int goalservo = 0;
                float waiter = 0;
                if (progArray[prog_num][2]<0.5){
                  goalservo = int(progArray[prog_num][2]);
                }
                else{
                  waiter = progArray[prog_num][2]*1000.0;
                }
                if (goalx>0){
                display.clearDisplay();
                display.setCursor(0,0);     // Start at top-left corner
                display.println("Running Auto Mode ");
                display.print(runtimes);
                display.println(" Times");
                display.print("So far ");
                display.print(k);
                display.println( " Times run");
                display.println("Next Goal Point: ");
                display.print(goalx);
                display.print(" ");
                display.print(goaly);
                display.print(" ");
                if (progArray[prog_num][2]<0.5){
                  if (goalservo==0){display.print("Up");}
                  else if (goalservo==-1){display.print("Down");}
                  else if (goalservo==-2){display.print("Quick");}
                  }
                else{
                  display.print("W ");
                  display.print(progArray[prog_num][2]);
                  display.print("s");
                  }
                display.display(); 
                trajmove(goalx,goaly);
                if (progArray[prog_num][2]<0.5){
                  runservo(goalservo);
                  }
                else{delay(waiter);}
                }
                delay(350);
          }}}
          else{
            int k = 0;
            bool quitenact = true;
            while(digitalRead(p5)&& quitenact){
              if (k%5==0){
                home_fun();
              }
              for (prog_num = 0;prog_num<ROWS;prog_num++){
                float goalx = progArray[prog_num][0];
                float goaly = progArray[prog_num][1];
                int goalservo = 0;
                float waiter = 0;
                if (progArray[prog_num][2]<0.5){
                  goalservo = int(progArray[prog_num][2]);
                }
                else{
                  waiter = progArray[prog_num][2]*1000.0;
                }
                if (goalx>0){
                display.clearDisplay();
                display.setCursor(0,0);     // Start at top-left corner
                display.println("Running Auto Mode Forever");
                display.print("So far ");
                display.print(k);
                display.println( " Times run");
                display.println("Next Goal Point: ");
                display.print(goalx);
                display.print(" ");
                display.print(goaly);
                display.print(" ");

                if (!digitalRead(p5)){
                  quitenact = false;
                }
                if (progArray[prog_num][2]<0.5){
                  if (goalservo==0){display.print("Up");}
                  else if (goalservo==-1){display.print("Down");}
                  else if (goalservo==-2){display.print("Quick");}
                  }
                else{
                  display.print("W ");
                  display.print(progArray[prog_num][2]);
                  display.print("s");
                  }
                display.println(" ");
                display.println(" ");
                display.println("Press and hold");
                display.print("middle O to exit");
                display.display(); 
                if (!digitalRead(p5)){
                  quitenact = false;
                }
                trajmove(goalx,goaly);
                if (progArray[prog_num][2]<0.5){
                  runservo(goalservo);
                  }
                else{delay(waiter);}
                
                if (!digitalRead(p5)){
                  quitenact = false;
                }
                }
                delay(350);
              }
              k++;
            }
          }
          display.clearDisplay();
          display.setCursor(0,0); 
          display.println("Ending Auto Mode");
          display.println(" ");
          display.println("Trajectory");
          display.println("Completed/Exited");
          display.display(); 
          delay(2000);
          home_fun();
          display.clearDisplay();
          display.setCursor(0,0);     // Start at top-left corner
          display.setTextSize(2);
          display.println("Switching to");
          display.println("Main Menu");
          display.display();      // Show initial text
          delay(1500);
          display.clearDisplay();
          display.display();
          display.setTextSize(1);
          prog_num = 0;
          robot_state = MENU;
          file_number = 0;
          runonce = false;
        }
      }
      break;
    case HOMING:
      home_fun();
      myservo.write(15); 
      display.clearDisplay();
      display.setCursor(0,17);     // Start at top-left corner
      display.setTextSize(2);
      display.println("Switching to");
      display.println("Main Menu");
      display.display();      // Show initial text
      delay(1500);
      display.clearDisplay();
      display.display();
      display.setTextSize(1);
      break;
    
    case PROGRAMMING:
    if (runonce == false){
      zeroprog();
      myservo.write(15); 
      display.clearDisplay();
      display.setCursor(0,0);     // Start at top-left corner
      display.setTextSize(1);
      display.println("Programming Mode");
      display.setCursor(0,16); 
      display.print("Edit A File:");
      display.setCursor(47,25); 
      display.print("O Program 1");
      display.setCursor(6,37); 
      display.print("esc O");
      display.setCursor(47,37); 
      display.print("O Program 2");
      display.setCursor(47,49); 
      display.print("O Program 3");
      display.display();
      file_number = 0;
      while (file_number==0){
        if (!digitalRead(p3)){
          robot_state = MENU;
          file_number = 4;
          runonce = false;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.setTextSize(2); 
          display.println("Exiting");
          display.println("program   mode");
          display.display();
          display.setTextSize(1); 
          delay(1000);
          break;
        }
        else if (!digitalRead(p1)){
          file_number = 1;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.println("Loading Program 1");
          display.display();
          delay(1000);
        }
        else if (!digitalRead(p5)){
          file_number = 2;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.println("Loading Program 2");
          display.display();
          delay(1000);
        }
        else if (!digitalRead(p2)){
          file_number = 3;
          display.clearDisplay();
          display.setCursor(0,0);  
          display.println("Loading Program 3");
          display.display();
          delay(1000);
        }
      }
      runonce = true;
    }
      if (file_number!=4){
        
        movemotorsman();
        if (show_ops==true && savetime==false){
          display.clearDisplay();
          display.setCursor(0,0);     // Start at top-left corner
          display.setTextSize(1);
          display.println("Programming Mode");
          display.setCursor(0,16); 
          display.println("Editing");
          display.print("File:");
          display.print(file_number);
          display.setCursor(47,17); 
          if (servomode==0 || servomode==-2){
            display.print("PUSH");
          }
          else if (servomode==-1){
            display.print("RELEASE");
          }
          display.setCursor(85,48); 
          display.print("Slot:");
          display.setCursor(85,57); 
          display.print(prog_num+1);
          display.print("/");
          display.print(ROWS);
          display.setCursor(55,27); 
          display.print("O");
          display.setCursor(6,37); 
          display.print("DONE O");
          display.setCursor(45,37); 
          display.print("save");
          display.setCursor(71,37); 
          display.print("O WAIT");
          display.setCursor(53,48); 
          display.print("O");
          display.setCursor(25,57); 
          display.print("QUICK TAP");
          display.display();      // Show initial text
          show_ops = false;
        }

        if (!digitalRead(p5) || savetime==true){
          progArray[prog_num][0] = currentpointX;
          progArray[prog_num][1] = currentpointY;
          if (savetime==false){
            progArray[prog_num][2] = float(servomode);
          }
          else{
            progArray[prog_num][2] = waittime;
          }
          display.clearDisplay();
          display.setCursor(0,8);     // Start at top-left corner
          display.println("Move saved");
          display.print("to program ");
          display.println(file_number);
          char buffer[10];
          // Convert float to string with 2 decimal places
          dtostrf(currentpointX, 6, 2, buffer);  // (float, min width, precision, buffer)
          display.print(buffer);
          display.print(" ");
          dtostrf(currentpointY, 6, 2, buffer);  // (float, min width, precision, buffer)
          display.print(buffer);
          display.println(" in");
          if (savetime==false){
            if (servomode==0){display.println("Servo Up");}
            else if (servomode==-1){display.println("Servo Down");}
            else if (servomode==-2){
              display.println("Servo Quick Tap");
              servomode = 0;
            }
          }
          else{
            display.print("Wait time: ");
            display.print(waittime);
            display.println("s");
            waittime = 0.5;
            savetime=false;
          }
          display.println(" ");
          display.print("Saved in slot: ");
          display.print(prog_num+1);
          display.print("/");
          display.println(ROWS);
          prog_num++;
          display.display();      // Show initial text
          delay(1500);
          display.clearDisplay();
          display.display();
          if (prog_num==ROWS){
            display.setCursor(0,0);     // Start at top-left corner
            display.println("Moves Full");
            display.print(ROWS);
            display.println(" Moves Saved");
            display.setCursor(20,20); 
            display.println("Save or Restart?");
            display.setCursor(6,37); 
            display.print("Save O");
            display.setCursor(71,37); 
            display.print("O Restart");
            display.display();      // Show initial text
            bool waiting = true;
            while(waiting){
              if(!digitalRead(p6)){
                show_ops = true;
                waiting = false;
                prog_num = 0;
                servomode = 0;
                zeroprog();
                delay(500);
              }
              if(!digitalRead(p3)){
                display.clearDisplay();
                display.setCursor(0,17);     // Start at top-left corner
                display.setTextSize(2);
                display.print("Saving to File ");
                display.print(file_number);
                display.display();
                saveArrayToEEPROM(progArray, file_number);
                delay(500);
                robot_state = MENU;
                home_fun();
                myservo.write(15); 
                display.clearDisplay();
                display.setCursor(0,17);     // Start at top-left corner
                display.setTextSize(2);
                display.println("Switching to");
                display.println("Main Menu");
                display.display();      // Show initial text
                delay(1500);
                display.clearDisplay();
                display.display();
                display.setTextSize(1);
                show_ops = true;
                waiting = false;
                prog_num = 0;
                servomode = 0;
              }
            }
          }
          display.setTextSize(1);
          show_ops = true;
        }
        else if (!digitalRead(p1)){
          if (servomode==0 || servomode==-2){
            myservo.write(120); 
            servomode = -1;
            show_ops = true;
            delay(500);
            
          }
          else if (servomode==-1){
            myservo.write(15); 
            servomode = 0;
            show_ops = true;
            delay(500);
          }
        }
        else if (!digitalRead(p6)){
          delay(500);
          bool waiting = true;
          savetime = false;
          waittime = 0.5;
          while(waiting){
            display.clearDisplay();
            display.setCursor(0,0);     // Start at top-left corner
            display.setTextSize(1);
            display.println("Set a wait time");
            display.setCursor(48,17); 
            display.print("SET");
            display.setCursor(80,17); 
            display.print(waittime);
            display.print(" s");
            display.setCursor(55,26); 
            display.print("O");
            display.setCursor(28,37); 
            display.print("- O");
            display.setCursor(70,37); 
            display.print("O +");
            display.setCursor(55,48); 
            display.print("O");
            display.setCursor(45,57); 
            display.print("CANCEL");
            display.display();      // Show initial text
            if (!digitalRead(p6)){
              waittime+=0.5;
              if (waittime>15){
                waittime=15;
              }
              delay(250);
            }
            else if (!digitalRead(p3)){
              waittime-=0.5;
              if (waittime<0.5){
                waittime=0.5;
              }
              delay(250);
            }
            else if (!digitalRead(p1)){
              savetime = true;
              waiting = false;
              show_ops = true;
              delay(500);
            }
            else if (!digitalRead(p2)){
              savetime = false;
              waiting = false;
              show_ops = true;
              delay(500);
            }
          }
          
        }
        else if (!digitalRead(p2)){
          myservo.write(120); 
          delay(250);
          myservo.write(15); 
          servomode = -2;
        }
        else if (!digitalRead(p3)){
          delay(500);
          display.clearDisplay();
          display.setCursor(0,17);     // Start at top-left corner
          display.setTextSize(1);
          display.println("Are you done");
          display.println("Programming?");
          display.setCursor(6,37); 
          display.print("Yes O");
          display.setCursor(71,37); 
          display.print("O No");
          display.display();      // Show initial text
          bool waiting = true;
          while(waiting){
            if(!digitalRead(p6)){
              show_ops = true;
              waiting = false;
              delay(500);
            }
            else if(!digitalRead(p3)){
              bool waitsave = true;
              display.clearDisplay();
              display.display();
              display.setCursor(0,0);     // Start at top-left corner
              display.println("Moves collected");
              display.print(prog_num);
              display.println(" Moves");
              display.setCursor(20,20); 
              display.println("Save or Cancel?");
              display.setCursor(6,37); 
              display.print("Save O");
              display.setCursor(71,37); 
              display.print("O Cancel");
              display.display();      // Show initial text
              delay(500);
              while(waitsave){
                if(!digitalRead(p6)){
                  waitsave = false;
                  delay(500);
                }
                else if(!digitalRead(p3)){
                  display.clearDisplay();
                  display.setCursor(0,17);     // Start at top-left corner
                  display.setTextSize(2);
                  display.print("Saving to File ");
                  display.print(file_number);
                  display.display();
                  waitsave = false;
                  saveArrayToEEPROM(progArray, file_number);
                  delay(500);
                }
              }
              robot_state = MENU;
              home_fun();
              myservo.write(15); 
              display.clearDisplay();
              display.setCursor(0,17);     // Start at top-left corner
              display.setTextSize(2);
              display.println("Switching to");
              display.println("Main Menu");
              display.display();      // Show initial text
              delay(1500);
              display.clearDisplay();
              display.display();
              display.setTextSize(1);
              show_ops = true;
              waiting = false;
              prog_num = 0;
              servomode = 0;
            }
          }
        }
      }
      
      break;
    default:
      break;
  }
      

}

void runservo(int servosetval){
  if (servosetval==-1){
    myservo.write(120); 
  }
  else if (servosetval==0){
    myservo.write(15); 
  }
  else if (servosetval==-2){
    myservo.write(120); 
    delay(250);
    myservo.write(15); 
  }
}

// Function to read a 10x3 float array from EEPROM
void readArrayFromEEPROM(float array[ROWS][COLS], int arrayNum) {
  int startAddress;
  switch (arrayNum) {
    case 1:
      startAddress = EEPROM_ARRAY1_START;
      break;
    case 2:
      startAddress = EEPROM_ARRAY2_START;
      break;
    case 3:
      startAddress = EEPROM_ARRAY3_START;
      break;
    default:
      break;
  }

  int address = startAddress;
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      float value;
      EEPROM.get(address, value);
      array[i][j] = value;
      address += sizeof(float);
    }
  }}


// Function to save a 10x3 float array to EEPROM
void saveArrayToEEPROM(float array[ROWS][COLS], int arrayNum) {
  int startAddress;
  switch (arrayNum) {
    case 1:
      startAddress = EEPROM_ARRAY1_START;
      break;
    case 2:
      startAddress = EEPROM_ARRAY2_START;
      break;
    case 3:
      startAddress = EEPROM_ARRAY3_START;
      break;
    default:
      break;
  }

  int address = startAddress;
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      float value = array[i][j];
      EEPROM.put(address, value);
      address += sizeof(float);
    }
  }}



void zeroprog(){
    for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < 3; j++) {
      progArray[i][j] = -1.0;
    }
  }}
void movemotorsman(){

  int m1an  = 1023-analogRead(readpot1);
  int m2an = 1023-analogRead(readpot2);
  desm1location = int(float(m1an)*1.564+100.0);
  if (desm1location>m1location){
    if (desm1location==max1step){
      desm1location=max1step;
    }
    if (abs(desm1location-m1location)>20){
    prevdesm1location = desm1location;
    m1location = m1location+1*m1velocity;
    m1moveval = -1*m1velocity;
    }
    else{ m1moveval = 0;}
  }
  else if(desm1location<m1location){
    if (desm1location==min1step){
      desm1location=min1step;
    }
    if (abs(desm1location-m1location)>20){
    prevdesm1location = desm1location;
    m1location = m1location-1*m1velocity;
    m1moveval = 1*m1velocity;
    }
    else{ m1moveval = 0;}
  }
  else{
    m1moveval = 0;
  }

  desm2location = int(float(m2an)*1.955);
  if (desm2location>m2location){
    if (desm2location==max2step){
      desm2location=max2step;
    }
    if (abs(desm2location-m2location)>20){
    prevdesm2location = desm2location;
    m2location = m2location+1*m2velocity;
    m2moveval = -1*m2velocity;
    }
    else{ m2moveval = 0;}
  }
  else if(desm2location<m2location){
    if (desm2location==min2step){
      desm2location=min2step;
    }
    if (abs(desm2location-m2location)>20){
    prevdesm2location = desm2location;
    m2location = m2location-1*m2velocity;
    m2moveval = 1*m2velocity;
    }
    else{ m2moveval = 0;}
  }
  else{
    m2moveval = 0;
  }

  if (abs(m1moveval)>abs(m2moveval)){
    stepcount = abs(m1moveval);
  }
  else{
    stepcount = abs(m2moveval);
  }
  int m1step = 1;
  int m2step = 1;
  for (int i = 0;i<stepcount;i++){
    
    if (m1moveval!=0){
      m1step = 1;
      m1step = sign(m1moveval)*m1step;
      m1moveval = m1moveval-sign(m1moveval);
    }
    else{
      m1step = 0;
    }

    if (m2moveval!=0){
      m2step = 1;
      m2step = sign(m2moveval)*m2step;
      m2moveval = m2moveval-sign(m2moveval);
    }
    else{
      m2step = 0;
    }
    
    // Move motor 1 the specified number of steps
    motor1.step(m1step);
    // Move motor 2 the specified number of steps
    motor2.step(m2step);
    currentpointX = float(m1location)/814.873;
    currentpointY = float(m2location)/407.436;}}



float norm_fun(float giv_vec_X, float giv_vec_Y){
    float mag = sqrt((giv_vec_X*giv_vec_X)+(giv_vec_Y*giv_vec_Y));
    return mag;}


int sign(int numb){

  int signer = 0;
  if (numb<0){
    signer = -1;
  }
  else if (numb>0){
    signer = 1;
  }
  return signer;}





void trajmove(float despointX, float despointY){

  float max_vel = 10; // Max loop step count 
  float desstepsX = int(814.873*despointX);
  float desstepsY = int(407.436*despointY);
  if (desstepsX>=max1step){
    desstepsX = max1step;
  }
  else if (desstepsX<=min1step){
    desstepsX = min1step;
  }
  if (desstepsY>=max2step){
    desstepsY = max2step;
  }
  else if (desstepsY<=min2step){
    desstepsY = min2step;
  }
  float Xtrans = desstepsX-m1location;
  float Ytrans = desstepsY-m2location;
  float error = norm_fun(Xtrans, Ytrans);

  while(error>10.0){

  Xtrans = desstepsX-m1location;
  Ytrans = desstepsY-m2location;
  error = norm_fun(Xtrans, Ytrans);
  if (error>max_vel){
    Xtrans = max_vel*(Xtrans/error);
    Ytrans = max_vel*(Ytrans/error);
    desm1location = m1location+Xtrans;
    desm2location = m2location+Ytrans;
  }
  else{
    desm1location = desstepsX;
    desm2location = desstepsY;
  }
  
  if (desm1location>m1location){
    if (desm1location>=max1step){
      desm1location=max1step;
    }
    Xtrans = desm1location-m1location;
    prevdesm1location = desm1location;
    m1location = m1location+Xtrans;
    m1moveval = -Xtrans;
  }
  else if(desm1location<m1location){
    if (desm1location==min1step){
      desm1location=min1step;
    }
    Xtrans = m1location-desm1location;
    prevdesm1location = desm1location;
    m1location = m1location-Xtrans;
    m1moveval = Xtrans;
  }
  else{
    m1moveval = 0;
    Xtrans = 0;
  }

  
  if (desm2location>m2location){
    if (desm2location==max2step){
      desm2location=max2step;
    }
    Ytrans = desm2location-m2location;
    prevdesm2location = desm2location;
    m2location = m2location+Ytrans;
    m2moveval = -Ytrans;
  }
  else if(desm2location<m2location){
    if (desm2location==min2step){
      desm2location=min2step;
    }
    Ytrans = m2location-desm2location;
    prevdesm2location = desm2location;
    m2location = m2location-Ytrans;
    m2moveval = Ytrans;
  }
  else{
    m2moveval = 0;
    Ytrans = 0;
  }
  
  // Move motor 1 the specified number of steps
  motor1.step(m1moveval);
  // Move motor 2 the specified number of steps
  motor2.step(m2moveval);
  currentpointX = float(m1location)/814.873;
  currentpointY = float(m2location)/407.436;
  }


    }
  
void home_fun(){
    myservo.write(15); 
    display.clearDisplay();
    display.setCursor(0,17);     // Start at top-left corner
    display.setTextSize(2);
    display.println("Homing");
    display.println("Robot...");
    display.display();      // Show initial text

    
    // Move motor 1 the specified number of steps
    motor1.step(-100);
    // Move motor 2 the specified number of steps
    motor2.step(-100);

  while(digitalRead(lim1)==1){
    int motor2Steps=1;
    motor2.step(motor2Steps);
  }
  delayMicroseconds(10000);
  while(digitalRead(lim2)==1){
    int motor1Steps=1;
    motor1.step(motor1Steps);
  }
  delayMicroseconds(10000);
  motor1.step(-100);


  stepcount = 0;
  desm1location = 0; // Desired Motor 1 location
  desm2location = 0; // Desired Motor 2 location
  prevdesm1location = 0; // Previous desired Motor 1 location
  prevdesm2location = 0; // Previous desired Motor 2 location
  m1location = 0;
  m2location = 0;
  m1moveval = 0;
  m2moveval = 0;
  m1velocity = 10;
  m2velocity = 8;
  currentpointX = 0;
  currentpointY = 0;
  runonce = false;
  delay(1500);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);}
