extern "C" void pucaos_setup(void);
extern "C" void pucaos_loop(void);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Hello");
    pucaos_setup();
}

void loop()
{
    pucaos_loop();
}