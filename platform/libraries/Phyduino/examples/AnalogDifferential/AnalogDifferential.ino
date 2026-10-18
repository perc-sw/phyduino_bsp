// Measures the voltage between two analog pins, e.g. across a resistor or
// a sensor bridge, twice a second.
//
// A3 - A4: true differential. The on-board ADC samples both pins at the same
//   instant, so noise common to both cancels. Range ±1024 mV in 0.5 mV steps
//   (a larger difference is measured as two separate readings).
// A0 - A1: computed from readings taken one after the other, a few µs apart.
//   Fine for slow signals, but noise on both pins does not cancel.
//
// Keep every pin between 0 V and its full scale (A0-A2: 3.3 V, A3-A5: 2.048 V).

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    analogReadAveraging(16);  // steadier readings
}

void loop() {
    Serial.print("A3 - A4: ");
    Serial.print(analogReadDifferentialMillivolts(A3, A4));
    Serial.print(" mV    A0 - A1: ");
    Serial.print(analogReadDifferentialMillivolts(A0, A1));
    Serial.println(" mV");
    delay(500);
}
