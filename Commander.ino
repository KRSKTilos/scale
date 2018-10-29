#include <Q2HX711.h>

/**
 * Commander Scale.
 * @author krsktilos.
 */

Q2HX711 scale(2,3);

/* Задержка рабочего цикла */
#define LOOP_DELAY 60
/* Длина стека взвешиваний */
#define STACK_SIZE 7
/* Допустимое отклонение в граммах */
#define THRESHOLD 10;

/* Тестовая калибровка */
#define DEBUG_CALIBRATIONVALUE 8776400;
#define DEBUG_CALIBRATIONMASS 500.0;
#define DEBUG_ZEROVALUE 8663400;

/* Релизная калибровка */
#define PROD_CALIBRATIONVALUE 8875700;
#define PROD_CALIBRATIONMASS 1000.0;
#define PROD_ZEROVALUE 8805300;

/* Чистое значение с внешним давлением */
long CALIBRATIONVALUE = PROD_CALIBRATIONVALUE;
/* Стабилизационный вес в граммах */
float CALIBRATIONMASS = PROD_CALIBRATIONMASS;
/* Чистое значение без внешнего давления */
long ZEROVALUE = PROD_ZEROVALUE;
/* Коэффициент масштабирования веса */
float koef = 1.0;

/* ВКЛ/ВЫКЛ отладочного вывода */
bool DEBUG = false;

/* Стек взвешиваний */
long stack[STACK_SIZE];
long tmp[STACK_SIZE];

bool stabilized = true;
long average = 0;
long gain = 0;

void print(String message) {if (DEBUG) {Serial.print(message);}}
void println(String message) {if (DEBUG) {Serial.println(message);}}

/**
 * SET PARAMS.
 */
void setup() {
  Serial.begin(115200);
  println(">>> SETUP <<<");
  println("calculate koef...");
  koef = calcKoef();
  print("koef:");
  println(String(koef));
  println("clear stack...");
  stackClear();
  println("find zero value...");
  ZEROVALUE = getRawValue();
  println(String(ZEROVALUE));
  prepare();
  setGain();
  print("gain:");
  println(String(gain));
  println(">>> DONE <<<");
}

void stackClear() {
  for (int i=0; i<STACK_SIZE; i++) {
    stack[i] = 0;
    tmp[i] = 0;
  }
}

void stackPush(long value) {
  tmp[0] = value;
  for (int i=1; i<STACK_SIZE; i++) {
    tmp[i] = stack[i-1];
  }
  for (int i=0; i<STACK_SIZE; i++) {
    stack[i] = tmp[i];
  }
}

/**
 * Коэффициент масштабирования.
 */
float calcKoef() {
  return (CALIBRATIONVALUE - ZEROVALUE) / CALIBRATIONMASS;
}

void prepare() {
  println("init...");
  for (int i=0; i<STACK_SIZE*2; i++) {
    getValue();
    delay(LOOP_DELAY);
  }
}

/**
 * Иинициализации утяжеления.
 */
void setGain() {
  println("set gain...");
  gain += getValue();
}

/**
 * Возвращает текущий вес в граммах.
 */
long getValue() {
  long raw = scale.read();
  print("raw:");
  print(String(raw));
  print(";");
  long w = ((raw - ZEROVALUE) / koef) - gain;
  print("w:");
  print(String(w));
  print(";");
  print("stable:");
  print(String(stabilized));
  print(";");
  print("gain:");
  println(String(gain));
  return w;
}

/**
 * Возвращает чистое значение веса.
 */
long getRawValue() {
  long raw = scale.read();
  return raw;
}

/**
 * Определяет текущее состояние весов.
 */
void checkStatus() {
  average = 0;
  for (int i=0; i<STACK_SIZE; i++) {
    average += stack[i];
  }
  average /= STACK_SIZE;
  print("AVERAGE:");
  println(String(average));
  bool stable = false;
  for (int i=0; i<STACK_SIZE; i++) {
    stable = abs(average - stack[i]) <= THRESHOLD;
    if (!stable) {
      break;
    }
  }
  stabilized = stable;
}

/**
 * Обработка входных команд.
 */
void receiveCommands() {
  if (Serial.available() > 0) {
    char b = Serial.read();
    switch (b) {
      case ':': {
        /* Передаем текущее состояние */
        byte buf[5];
        buf[0] = average & 255;
        buf[1] = (average >> 8)  & 255;
        buf[2] = (average >> 16) & 255;
        buf[3] = (average >> 24) & 255;
        buf[4] = stabilized ? 1 : 0;
        Serial.write(buf, 5);
        break;
      }
      case ';': {
        /* Сброс в ноль */
        setGain();
        break;
      }
    }
  }
}

void step() {
  long weight = getValue();
  stackPush(weight);
}

/**
 * MAIN LOOP.
 */
void loop() {
  step();
  checkStatus();
  receiveCommands();
  delay(LOOP_DELAY);
}
