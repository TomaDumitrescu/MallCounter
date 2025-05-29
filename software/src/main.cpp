#include <SPI.h>
#include <Ticker.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED screen size setup
#define WSIZE 128
#define HSIZE 64

// Control pins
#define OLED_GME12864_14_DC     D2
#define OLED_GME12864_14_RST    -1
#define OLED_GME12864_14_CS     D8

#define ANALOG_CHN      (uint8_t)17U

#define INIT_DELAY          1000
#define INIT_CURSOR_X 0
#define INIT_CURSOR_Y 0
#define SKIP_TEXT 20

// Semaphore
#define GO D1

// Closeness CNT_THRESHOLD for the sensor
#define CNT_THRESHOLD         150

#define BAUD_RATE 115200

// Using SPI protocol for displaying on the OLED
Adafruit_SSD1306 display(WSIZE, HSIZE, &SPI, OLED_GME12864_14_DC, OLED_GME12864_14_RST, OLED_GME12864_14_CS);

// Timer for generating interrupts when new input is detected
Ticker timer;

// Global variables that are constantly modified in the loop
volatile uint16_t input_sensor_val = 0;
volatile uint32_t cnt = 0;
volatile bool new_sensor_in = false;

// Interrupt for registering sensor input
IRAM_ATTR void onSample() {
  uint16_t registered_val = analogRead(ANALOG_CHN);

  input_sensor_val = registered_val;

  // A person is passing nearby
  if (registered_val >= CNT_THRESHOLD) {
    cnt++;
  }

  new_sensor_in = true;
}

void setup() {
  // The OLED and the Arduino will have same baud rates
  Serial.begin(BAUD_RATE);

  // Init the semaphore, red at the start
  pinMode(GO, OUTPUT);

  // Initializing SPI
  SPI.begin();

  delay(INIT_DELAY);

  // Sort of defensive programming
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SPI setup failed!"));

    while (true);
  }

  // Initializing OLED board with custom display formats
  display.clearDisplay();

  // size
  display.setTextSize(1);

  display.setTextColor(SSD1306_WHITE);

  // offset
  display.setCursor(0, 0);

  int init_v = analogRead(ANALOG_CHN);

  display.setCursor(0, 20);

  display.print(F("IR: "));

  display.println(init_v);

  display.display();

  // Init the timer
  timer.attach_ms(150, onSample);
}

bool first_green_switch = true;

void loop() {
  if (first_green_switch) {
    first_green_switch = false;

    delay(200);

    digitalWrite(GO, HIGH);
  }

  if (new_sensor_in) {
    // For avoiding read after write
    noInterrupts();

    uint16_t val = input_sensor_val;

    uint32_t count = cnt;

    new_sensor_in = false;

    if (val > CNT_THRESHOLD) {
      // Lock
      digitalWrite(GO, LOW);

      delay(1500);

      // Unlock
      digitalWrite(GO, HIGH);
    }

    interrupts();

    // display reset to print new values
    display.clearDisplay();

    display.setTextSize(1);

    display.setTextColor(SSD1306_WHITE);

    display.setCursor(INIT_CURSOR_X, INIT_CURSOR_Y);

    display.println(F("IR Distance Sensor:"));

    display.setCursor(INIT_CURSOR_X, SKIP_TEXT);

    display.print(F("Count: "));

    display.println(count);

    display.display();
  }
}
