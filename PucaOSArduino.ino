#include "mongoose.h"

extern "C" void pucaos_setup(void);
extern "C" void pucaos_loop(void);

static void mongoose_print_callback(char ch, void *param)
{
    if (Serial)
    {
        //Serial.print(ch);
    }
}

void setup()
{
    delay(500);
    Serial.begin(115200);
    delay(500);
    if (Serial)
    {
        Serial.println("Setup start.");
    }

    mg_log_set(MG_LL_VERBOSE);
    mg_log_set_fn(mongoose_print_callback, NULL);

    pucaos_setup();
}

void loop()
{
    pucaos_loop();
}