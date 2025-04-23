// Bruker kun kjerne 1 for demonstrasjon
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

int btnPins[] = {12, 14, 27, 26};
int ledPins[] = {13, 33, 32, 25};

/* 
* Binær semafor for å sørge for at id-nummeret blir lageret i interactor-tasken. 
* Dette brukes for å identifisere hvilken knapp som har blitt trykket inn.
*/
static SemaphoreHandle_t binSem;

/* Identifikator for interactor-oppgaven */
static TaskHandle_t interactorTask;

/*
* Avbruddsrutine for knappetrykk. Vekker den påkoblete interactorTask-tråden. 
*/
void IRAM_ATTR ISR_BTN() {
  BaseType_t task_woken = pdFALSE;

  vTaskNotifyGiveFromISR(interactorTask, &task_woken);

  if (task_woken) {
    portYIELD_FROM_ISR();
  }
}

/*
* Interactor-oppgaven, som reagerer på knappetrykk og styrer en LED basert på dette. Laget for å kunne brukes for
* flere knappe-LED-par ved å ha en parameteriserbar ID.  En binær semafor brukes for å signalisere at IDen er lagret lokalt.
*/
void interactor(void *parameters) {
  uint8_t interactorId = *((uint8_t *)parameters);
  free(parameters);  // Frigjør én gang, med én gang

  int ledState = 0;

  Serial.print("Task ready:");
  Serial.println(interactorId);

  xSemaphoreGive(binSem);  // Signaler at ID er lagret

  while (1) {
    Serial.print("Task executing: ");
    Serial.println(interactorId);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    digitalWrite(ledPins[interactorId], ledState);
    ledState = !ledState;
  }
  //free(parameters);
}


void setup() {
    Serial.begin(115200);
    Serial.println("---Reaksjonsspill---");

    char task_name[14];
    uint8_t *interactorId = (uint8_t *)malloc(sizeof(uint8_t)); 
    *interactorId = 0;

    binSem = xSemaphoreCreateBinary();

    pinMode(btnPins[0], INPUT_PULLUP);
    pinMode(ledPins[0], OUTPUT);
    pinMode(btnPins[1], INPUT_PULLUP);
    pinMode(ledPins[1], OUTPUT);
    pinMode(btnPins[2], INPUT_PULLUP);
    pinMode(ledPins[2], OUTPUT);
    pinMode(btnPins[3], INPUT_PULLUP);
    pinMode(ledPins[3], OUTPUT);

    attachInterrupt(btnPins[0], ISR_BTN, FALLING);
    attachInterrupt(btnPins[1], ISR_BTN, FALLING);
    attachInterrupt(btnPins[2], ISR_BTN, FALLING);
    attachInterrupt(btnPins[3], ISR_BTN, FALLING);

    sprintf(task_name, "Interactor %i", *interactorId);
    xTaskCreatePinnedToCore(interactor,
                            task_name,
                            4092,
                            (void *)interactorId,
                            1,
                            &interactorTask,
                            app_cpu);
    xSemaphoreTake(binSem, portMAX_DELAY);
  
}

void loop() {

}
