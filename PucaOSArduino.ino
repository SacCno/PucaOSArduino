extern "C" void pucaos_setup(void);
extern "C" void pucaos_loop(void);

void setup()
{
    pucaos_setup();
}

void loop()
{
    pucaos_loop();
}