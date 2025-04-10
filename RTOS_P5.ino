/*
 *  RTOS video 5:
 *
 * Denne koden er hovedsaklig kopiert fra LF
 *
 *  2 Tasks:
 *  TASK A:
 *  - Printe fra queue 2
 *  - Lese Serielt input fra bruker
 *  - Echoe inputet fra bruker tilbake til serie-terminal
 *  - if "delay xxx" send xxx (tall) til queue 1
 *
 *  TASK B:
 *  - Blinke LED med t-delay
 *  - Oppdaterer t med nye verdier fra queue 1
 *  - Hver gang led'en blinka 100 gonga, send
 *    string til queue 2
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint8_t buf_len = 255;     // Size of buffer to look for command
static const char command[] = "delay "; // Note the space!
static const int delay_queue_len = 5;   // Size of delay_queue
static const int msg_queue_len = 5;     // Size of msg_queue
static const uint8_t blink_max = 100;   // Num times to blink before message

// Pins (change this if your Arduino board does not have LED_BUILTIN defined)
static const int led_pin = 2;

// Meldings-struct: Brukt her for meldingene, hvor body bestemmer størrelse og count teller antall meldinger sendt til queue
typedef struct Message {
  char body[20];
  int count;
} Message;

// Globale variabler
static QueueHandle_t delay_queue;
static QueueHandle_t msg_queue;

//*****************************************************************************
// Tasks

// Task A: Her kalt CLI ettersom den etterlinger linux cli 
void doCLI(void *parameters) {
  // Instans av message-structen
  Message rcv_msg;

  char c;
  char buf[buf_len];
  uint8_t idx = 0;
  uint8_t cmd_len = strlen(command);
  int led_delay;

  // Memset tømmer bufferen
  memset(buf, 0, buf_len);

  // Uendelig løkke:
  while (1) {

    // See if there's a message in the queue (do not block)
    if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE) {
      Serial.print(rcv_msg.body);
      Serial.println(rcv_msg.count);
    }

    // Leser fra serie-monitior
    if (Serial.available() > 0) {
      c = Serial.read();

      // Lagrer mottatt Char i buf, gitt at den ikke overskrider størrelsen på buf
      if (idx < buf_len - 1) {
        buf[idx] = c;
        idx++ ;
      }

      // Printer linjeskift når melding er sendt
      if ((c == '\n') || (c == '\r')) {

        // linjeskift til serie-monitor
        Serial.print("\r\n");

        // Sjekker om de første 6 karakterene tilsvarer 'delay' 
        if (memcmp(buf, command, cmd_len) == 0) {

          // Konverterer siste delen av skringen til positiv int
          char* tail = buf + cmd_len;
          led_delay = atoi(tail);
          led_delay = abs(led_delay);

          // Sender int til andre task via queue
          if (xQueueSend(delay_queue, (void *)&led_delay, 10) != pdTRUE) {
            Serial.println("ERROR: Could not put item on delay queue.");
          }
        }

        // Tømmer bufferen igjen med memset 
        memset(buf, 0, buf_len);
        idx = 0;

      // Dersom stringen ikke starter med 'delay', echo inputet:
      } else {
        Serial.print(c);
      }
    } 
  }
}

// Task A: Blink LED med delay hentet fra queue, gi tilbakemelding hvert 100. blink
void blinkLED(void *parameters) {
  // Ny instans av Message-struct
  Message msg;
  int led_delay = 500;
  uint8_t counter = 0;

  // Setter pinne 2 som output
  pinMode(2, OUTPUT);

  // Evig løkke 
  while (1) {
    // Sjekker om det er en melding tilgjengelig i queue
    if (xQueueReceive(delay_queue, (void *)&led_delay, 0) == pdTRUE) {

      // Best practice: use only one task to manage serial comms
      strcpy(msg.body, "Message received ");
      msg.count = 1;
      xQueueSend(msg_queue, (void *)&msg, 10);
    }

    // Blink
    digitalWrite(led_pin, HIGH);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);

    // Dersom vi har blinket LED'en 100 ganger, send melding til queue
    counter++;
    if (counter >= blink_max) {
      
      // Lager melding og sender til queue
      strcpy(msg.body, "Blinked: ");
      msg.count = counter;
      xQueueSend(msg_queue, (void *)&msg, 10);

      // Resetter counter
      counter = 0;
    }
  }
}

//*****************************************************************************
// Main Kjører som sin egen task med prioritet 1

void setup() {

  // Konfigurerer Serial
  Serial.begin(115200);

  // Lite delay
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Queue Solution---");
  Serial.println("Enter the command 'delay xxx' where xxx is your desired ");
  Serial.println("LED blink delay time in milliseconds");

  // Lager queue 1 og 2 
  delay_queue = xQueueCreate(delay_queue_len, sizeof(int));
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

  // Starter CLI task
  xTaskCreatePinnedToCore(doCLI,
                          "CLI",
                          2048,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // Starter blink task
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          2048,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // sletter "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}
