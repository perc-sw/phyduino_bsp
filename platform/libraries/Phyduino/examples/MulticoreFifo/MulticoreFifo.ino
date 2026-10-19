// Hands slow work to the second core and gets the answers back through
// rp2040.fifo. Core 0 sends four numbers; core 1 counts the primes below each
// one and sends back the count and how long it took. Core 0 is free in the
// meantime: it keeps the LED blinking and prints each answer as it arrives.

const int JOBS = 4;
const uint32_t LIMITS[JOBS] = {1000, 10000, 100000, 1000000};

// --- Core 0 ---------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < JOBS; i++) {
        rp2040.fifo.push(LIMITS[i]);  // core 1 works through them in order
    }
    Serial.println("Work sent to core 1.");
}

void loop() {
    static int done = 0;
    digitalWrite(LED_BUILTIN, (millis() / 250) % 2);  // still blinking while core 1 works

    if (done < JOBS && rp2040.fifo.available() >= 2) {
        const uint32_t count = rp2040.fifo.pop();
        const uint32_t ms = rp2040.fifo.pop();
        Serial.print("Primes below ");
        Serial.print(LIMITS[done]);
        Serial.print(": ");
        Serial.print(count);
        Serial.print(" (");
        Serial.print(ms);
        Serial.println(" ms on core 1)");
        done++;
    }
}

// --- Core 1 ---------------------------------------------------------------------
bool isPrime(uint32_t n) {
    if (n < 2) return false;
    for (uint32_t d = 2; d * d <= n; d++) {
        if (n % d == 0) return false;
    }
    return true;
}

void loop1() {
    const uint32_t limit = rp2040.fifo.pop();  // waits for the next job
    const unsigned long start = millis();
    uint32_t count = 0;
    for (uint32_t n = 2; n < limit; n++) {
        if (isPrime(n)) count++;
    }
    rp2040.fifo.push(count);
    rp2040.fifo.push(millis() - start);
}
