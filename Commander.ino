#include <Q2HX711.h>

/**
 * Commander Scale.
 * @author krsktilos.
 */

Q2HX711 scale(7,6);

/* Задержка рабочего цикла */
#define LOOP_DELAY 80
/* Длина стека взвешиваний */
#define STACK_SIZE 5
/* Допустимое отклонение в граммах */
#define THRESHOLD 3;

/* Чистое значение давления в 500 грамм */
long STANDART500VALUE = 8776400;
/* Стабилизационный вес в граммах */
float STANDART500 = 500.0;
/* Чистое значение без внешнего давления */
long ZEROVALUE = 8663400;
/* Коэффициент масштабирования веса */
float koef = 1.0;
/* ВКЛ/ВЫКЛ отладочного вывода */
bool DEBUG = false;

/* Стек взвешиваний */
long stack[STACK_SIZE];
long tmp[STACK_SIZE];

bool stabilized = true;
long weight = 0;
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
  clearStack();
  println("find zero value...");
  ZEROVALUE = getRawValue();
  println(String(ZEROVALUE));
  gain = calcGain();
  print("gain:");
  println(String(gain));
  println(">>> DONE <<<");
}

void clearStack() {
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
  return (STANDART500VALUE - ZEROVALUE) / STANDART500;
}

/**
 * Иинициализации утяжеления.
 */
long calcGain() {
  println("calculate gain...");
  long tmp = 0;
  for (int i=0; i<20; i++) {
    tmp = getValue();
    delay(100);
  }
  return tmp;
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
  println(String(stabilized));
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
  long average = 0;
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
    if (b == ':') {
      byte buf[4];
      buf[0] = weight & 255;
      buf[1] = (weight >> 8)  & 255;
      buf[2] = (weight >> 16) & 255;
      buf[3] = (weight >> 24) & 255;
      Serial.write(buf, 4);
      Serial.write(stabilized ? 1 : 0);
    }
  }
}

void step() {
  weight = getValue();
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
