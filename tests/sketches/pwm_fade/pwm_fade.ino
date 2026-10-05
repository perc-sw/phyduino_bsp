// Fades LED_BUILTIN up and down with analogWrite().
void setup() {}

void loop() {
    for (int level = 0; level <= 255; level++) { analogWrite(LED_BUILTIN, level); delay(4); }
    for (int level = 255; level >= 0; level--) { analogWrite(LED_BUILTIN, level); delay(4); }
}
