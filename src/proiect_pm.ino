#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#define PH_SENSOR_PIN           (A0)
#define NO_SAMPLES              (40)
#define ONE_WIRE_BUS            (13)
#define RED_PIN                 (7)
#define GREEN_PIN               (6)
#define BLUE_PIN                (5)
#define DELAY_BETWEEN_PROBES    (20)
#define LOOP_DELAY				(50)
#define NIL                     (0)

static LiquidCrystal_I2C lcd(0x27, 16, 4);
static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);
static SoftwareSerial BTserial(10,11);
static size_t measured_values;
static int ph_values[NO_SAMPLES];

typedef struct rgb_ph {
	size_t r;
	size_t g;
	size_t b;
} rgb_ph_t;

static const rgb_ph_t rgb_ph_values[15] = {
	255, 0, 0,      //red
	255, 20, 147,   //pink
	255, 140, 0,    //orange
	255, 165, 0,    //beige
	255, 255, 0,    //yellow
	50, 205, 50,    //lime green
	0, 128, 0,      //green
	1, 50, 32,      //dark green
	64, 224, 208,   //turquoise
	30, 144, 255,   //pale blue
	0, 0, 255,      //blue
	0, 0, 139,      //dark blue
	138, 43, 226,   //violet
	139,0,139       //indigo
};


void setup() {
	Serial.begin(9600);
	BTserial.begin(9600);
	lcd.begin();
	sensors.begin();
	lcd.backlight();
	setColorRGB(NIL, NIL, NIL);
}

static volatile bool start_measurement;
static volatile bool end_measurement;
static char bt_input[7];

void loop() {
	if (end_measurement == false) {
		if (start_measurement == false) {
			lcd.print("Loading...");
			int cnt = 0;
			char c = '\0';
			while (c != '!') {
				c = BTserial.read();
				Serial.println(c);
				bt_input[cnt++] = c;
			}

			if (strncmp(bt_input, "start", 5) == 0) {
				start_measurement = true;
				memset(bt_input, 0, sizeof(bt_input));
			}
		}

		if (start_measurement == true) {
			for (int i = 0; i < NO_SAMPLES; ++i) {
				ph_values[i] = analogRead(PH_SENSOR_PIN);
				delay(DELAY_BETWEEN_PROBES);
			}

			qsort(ph_values,
				sizeof(ph_values) / sizeof(*ph_values),
				sizeof(*ph_values),
				[](const int a, const int b) {
				return a > b;
			});

			measured_values = 0;

			for (int i = 2; i < NO_SAMPLES - 2; ++i) {
				measured_values += ph_values[i];
			}

			float ph_val = (float) (measured_values / (NO_SAMPLES - 4));
			Serial.println(ph_val);
			ph_val = (3.0 * ph_val - 877) / 59;
			lcd.setCursor(NIL, NIL);
			sensors.requestTemperatures();
			lcd.print("Temperature: ");
			lcd.print(sensors.getTempCByIndex(0));
			BTserial.print(sensors.getTempCByIndex(0));
			BTserial.print(",");
			lcd.setCursor(NIL, 2);
			lcd.print("pH Value:");
			lcd.print(ph_val);
			BTserial.print(ph_val);
			BTserial.print(";");
			convert_ph_value_to_rgb(ph_val);

			if (BTserial.read() == 'x') {
				lcd.clear();
				lcd.setCursor(NIL, NIL);
				lcd.print("Stop...");
				setColorRGB(NIL, NIL, NIL);
				end_measurement = true;
			}
			
			delay(LOOP_DELAY);
		}
	} else {
		if (BTserial.read() == 'r') {
			end_measurement = false;
			start_measurement = true;
		}
	}
}

static void setColorRGB(unsigned int red,
						unsigned int green,
						unsigned int blue) {
	analogWrite(RED_PIN, red);
	analogWrite(GREEN_PIN, green);
	analogWrite(BLUE_PIN, blue);
}

static void convert_ph_value_to_rgb(const float ph_val) {
	size_t index = floor(ph_val);
	setColorRGB(rgb_ph_values[index].r, rgb_ph_values[index].g, rgb_ph_values[index].b);
}