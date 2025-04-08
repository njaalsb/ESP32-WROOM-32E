# Denne koden fungerer ikke helt som den skal, men det var dette han i digikey-videoen kom frem til.

// use only one core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings:
static const uint8_t buf_len = 255;

// Globale variabler:
static char *msg_ptr = NULL;
static volatile uint8_t msg_flag = 0;


// Tasks:

void readSerial(void*parameter){
  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  // Tømmer bufferen:
  memset(buf, 0, buf_len);

  // uendelig løkke:
  while(1){
    if(Serial.available() > 0){
      c = Serial.read();

      if(idx < buf_len - 1){
        buf[idx] = c;
        idx++;
      }
      if(c == '\n'){

        buf[idx-1] = '\0';

        if(msg_flag == 0){
          
          msg_ptr = (char*)pvPortMalloc(idx * sizeof(char));

          // Dersom malloc returnerer 0, gi feilmelding og reset
          configASSERT(msg_ptr);

          // Kopier meldingen:
          memcpy(msg_ptr, buf, idx);

          // Gi beskjed til den andre tasken at resultatet er klart. 
          msg_flag = 1;
        } 

        memset(buf, 0, buf_len);
        idx = 0;
      }
    }
  }
}

void printMessage(void*parameter){
  // Gjør ingenting før flagget indikerer at bufferen er full
  while(1){
    if(msg_flag == 1){
      Serial.println(msg_ptr);

      // Gir mengde fri minne på heapen:
      Serial.print("Free heap (bytes): ");
      Serial.println(xPortGetFreeHeapSize());
      //Serial.println(xPortGetFreeHeapSize());

      // Tøm buffer:
      vPortFree(msg_ptr);
      msg_ptr = NULL;
      msg_flag = 0;
    } 
  }
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Lite delay i starten:
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Heap Demo---");
  Serial.println("Enter a string");

  // Serial recieve task:
  xTaskCreatePinnedToCore(readSerial, "Read Serial", 1024, NULL, 1, NULL, app_cpu);
  // Serial print task:
  xTaskCreatePinnedToCore(printMessage, "Print Message", 1024, NULL, 1, NULL, app_cpu);

  // delete setup and loop task:
  vTaskDelete(NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
