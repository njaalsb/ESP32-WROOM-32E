// Bruker kun kjerne 1 for demonstrasjon
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//#include <Arduino.h>

int btnPins[] = {12, 14, 27, 26};
int ledPins[] = {13, 33, 32, 25};

static SemaphoreHandle_t binSem;
static TaskHandle_t ledTasks[4];
static TaskHandle_t gameTaskHandle;

uint8_t difficulty = 1;
int failcount = 0;
int score = 0;
int currentLed = -1;
int roundCount = 0;
unsigned long prevMillis = 0;
TimerHandle_t gameTimer;

void IRAM_ATTR ISR_BTN(int btnId) {
  if (btnId == currentLed) {
    if (millis() - prevMillis <= difficulty * 500) {
      score += difficulty*500 - (millis() - prevMillis);
      Serial.println("Riktig knapp!");
    } 
  } 
  else {
    Serial.print("Feil knapp! Du har ");
    Serial.print(abs(failcount-5));
    Serial.println(" liv igjen.");

    failcount++;
  }
  currentLed = -1;
}

void IRAM_ATTR ISR_BTN_1() { 
  ISR_BTN(0); 
}

void IRAM_ATTR ISR_BTN_2() {
  ISR_BTN(1); 
}

void IRAM_ATTR ISR_BTN_3() {
  ISR_BTN(2); 
}

void IRAM_ATTR ISR_BTN_4() {
  ISR_BTN(3); 
}

void ledTask(void *parameters) {
  int id = *(int *)parameters;
  free(parameters);

  pinMode(ledPins[id], OUTPUT);

  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    digitalWrite(ledPins[id], HIGH);
    prevMillis = millis();
    vTaskDelay(difficulty * 500/portTICK_PERIOD_MS);
    digitalWrite(ledPins[id], LOW);
  }
}

void gameTimerCallback(TimerHandle_t xTimer) {
  if (failcount - 5 > 0) {
    Serial.print("Spillet ferdig. Du overlevde: ");
    Serial.print(roundCount);
    Serial.print(" Runder. Total score: ");
    Serial.println(score);
    vTaskSuspend(gameTaskHandle);
    return;
  }

  currentLed = random(0, 4);
  xTaskNotifyGive(ledTasks[currentLed]);
  roundCount++;
  Serial.print("Runde: ");
  Serial.println(roundCount);
}

void gameTask(void *params) {
  xSemaphoreTake(binSem, portMAX_DELAY);
  Serial.println("Starter spillet...");
  score = 0;
  roundCount = 0;
  vTaskDelay(1000/portTICK_PERIOD_MS);
  xTimerStart(gameTimer, 0);
  while (1) {
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void readDifficultyTask(void *params) {
  Serial.println("Velg vanskelighetsgrad (1-3):");
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c >= '1' && c <= '3') {
        difficulty = c - '0';
        Serial.print("Vanskelighetsgrad valgt: ");
        Serial.println(difficulty);
        xSemaphoreGive(binSem);
        vTaskDelete(NULL);
      } else {
        Serial.println("Velg et heltall fra 1-3");
      }
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  vTaskDelay(1000/portTICK_PERIOD_MS);

  Serial.println("---Reaksjonsspill---");

  binSem = xSemaphoreCreateBinary();

  for (int i = 0; i < 4; i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
  }

  attachInterrupt(btnPins[0], ISR_BTN_1, FALLING);
  attachInterrupt(btnPins[1], ISR_BTN_2, FALLING);
  attachInterrupt(btnPins[2], ISR_BTN_3, FALLING);
  attachInterrupt(btnPins[3], ISR_BTN_4, FALLING);

  for (int i = 0; i < 4; i++) {
    int *id = (int *)malloc(sizeof(int));
    *id = i;
    xTaskCreatePinnedToCore(
                        ledTask,
                        "LEDTask",
                        2048,
                        id,
                        1,
                        &ledTasks[i],
                        app_cpu);
  }

  xTaskCreatePinnedToCore(
                        readDifficultyTask,
                        "ReadDifficulty",
                        2048,
                        NULL,
                        2,
                        NULL,
                        app_cpu);

  xTaskCreatePinnedToCore(
                        gameTask,
                        "GameTask",
                        2048,
                        NULL,
                        1,
                        &gameTaskHandle,
                        app_cpu);

  gameTimer = xTimerCreate("GameTimer", random(1000, 3000)/portTICK_PERIOD_MS, pdTRUE, NULL, gameTimerCallback);
}

void loop() {

}
