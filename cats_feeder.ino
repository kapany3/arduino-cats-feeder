#include <Servo.h>
#include <EEPROM.h>
#include <iarduino_RTC.h>
#include <Wire.h>

// ARDUINO PRO MINI !!!

Servo myservo;  // create servo object to control a servo

int servoPin = 9;
int buttonPin = 8;
int ledPin = 4;

unsigned long servoMls;
unsigned long servoTime = 5000;
unsigned long setTime = 0;

int anglesPlus [5] = {0, 32, 61, 92, 124};
int anglesMinus [5] = {0, 27, 56, 88, 124};
int fixPlus = 1;
int fixMinus = -3;
String feedTimes[]={"08:00", "13:00", "18:00", "23:00"};
bool skipNext = false;

const bool debug = true;

struct ServoInfo {
  int pos; // 0 - 4
  int dir; // -1, 1
  int posStart; // 0 - 4
  int count; // 0 - 4
};

ServoInfo info;

iarduino_RTC time(RTC_DS1302, 5, 7, 6);  // RST, CLK, DAT

void setup() {
  time.begin();
  
  if (debug) {
    Serial.begin(9600);
    Serial.println("Start");
    Serial.println(time.gettime("H:i"));
  }

  // time.settime(0, 26, 17, 9, 03, 2019, 6);
  
  EEPROM.get(0, info);
  if (info.pos > 4 || info.pos < 0 || info.dir != 1 && info.dir != -1) {
    info.pos = 0;
    info.dir = 1;
    info.posStart = 0;
    info.count = 0;
    EEPROM.put(0, info);
    if (debug) {
      Serial.println("Init servo variable");
    }
  } else if (debug) {
    Serial.println("Servo variables:");
    Serial.print("Angle num:");
    Serial.println(String(info.pos));
    Serial.print("Direction:");
    Serial.println(String(info.dir));
    Serial.print("Start position:");
    Serial.println(String(info.posStart));
    Serial.print("Count:");
    Serial.println(String(info.count));
  }

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, info.count == 4 ? HIGH : LOW);
  
  pinMode(buttonPin, INPUT);
  myservo.attach(servoPin);
  myservo.write(info.dir > 0 ? anglesPlus[info.pos] : anglesMinus[info.pos]);
  servoMls = millis() + 1;
}

void putFeed() {
  if (debug) {
    Serial.println("Put feed");
  }
  info.posStart = info.pos;
  info.count = 0;
  EEPROM.put(0, info);
  digitalWrite(ledPin, LOW);
  skipNext = false;
  delay(5000);
}

void eatTime() {
  if (debug) {
    Serial.println("Eat time");
  }

  if (info.count >= 4) {
    if (debug) {
      Serial.println("No food");
    }
    return;
  }
  info.count += 1;

  int pos = info.pos;
  while (true) {
    pos += info.dir;
    if (pos > 4 || pos < 0) {
      info.dir = - info.dir;
      pos = info.posStart;
    } else {
      break;
    }
  }

  if (info.count == 4) {
    digitalWrite(ledPin, HIGH);
  }
  
  rotate(pos);
}

void rotate(int pos) {
  int angleCur = info.dir > 0 ? anglesPlus[info.pos] : anglesMinus[info.pos];
  int angleTo = info.dir > 0 ? anglesPlus[pos] : anglesMinus[pos];

  if (debug) {
    Serial.print("Ratate from ");
    Serial.print(String(info.pos));
    Serial.print(" to ");
    Serial.println(String(pos));
  }

  info.pos = pos;
  EEPROM.put(0, info);

  int inc = 0;
  if (angleCur > angleTo) {
    inc = -1;
  } else if (angleCur < angleTo) {
    inc = 1;
  } else {
    return;
  }

  if (debug) {
    Serial.print("Step is ");
    Serial.println(String(inc));
  }
  
  myservo.attach(servoPin);
  for (int i = angleCur; i != angleTo; i += inc) {
    if (debug) {
      Serial.print(String(i));
      Serial.print(' ');
    }
    myservo.write(i);
    delay(50);
  };
  myservo.write(angleTo);
  if (debug) {
    Serial.println(String(angleTo));
  }
  servoMls = millis();
}

void loop() {
  unsigned long cur = millis();
  
  if (servoMls > 0) {
    // Servo started
    if (cur > servoTime && cur < servoTime * 2 ||  cur > servoMls + servoTime) {
      servoMls = 0;
      myservo.detach();
    }
    return;
  }

  if (digitalRead(buttonPin) == HIGH) {
    delay(30);
    if (digitalRead(buttonPin) == HIGH) {
      // Real button press
      delay(500);
      if (digitalRead(buttonPin) == HIGH) {
        // Long press
        putFeed();
      } else {
        // Short press
        eatTime();
        skipNext = true;
      }
    }
  }

  if (cur % 60000 < 10) {
    // Вдруг будет пропуск тиков из-за чего-то
    String curTime = time.gettime("H:i");
    Serial.println(curTime);

    for (int i = 0; i < 5; i += 1) {
      if (feedTimes[i] == curTime) {
        if (skipNext) {
          skipNext = false;
          // Мог застрять на проблестковости, что идет пропуск и остаться гореть
          digitalWrite(ledPin, info.count == 4 ? HIGH : LOW);
          if (debug) {
            Serial.println("Skip eat");
          }
        } else {
          eatTime();
        }
        break;
      }
    }

    if (curTime == '00:00') {
      for (int i = 0; i < 5; i += 1) {
        digitalWrite(ledPin, HIGH);
        delay(500);
        digitalWrite(ledPin, LOW);
        delay(500);
      }
    }
    // Обождать, чтоб таймер опять не сработал
    delay(10);
  }

  if (info.count != 4) {
    if ((cur + 500) % 10000 < 10 && skipNext) {
      digitalWrite(ledPin, HIGH);
    }
    if (cur % 10000 < 10 && skipNext) {
      digitalWrite(ledPin, LOW);
    }
  }
}
