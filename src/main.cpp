#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "BluetoothHFClient.hpp"

// Define the GPIO pin for the LED (GPIO 2 is common for onboard LEDs)
#define BLINK_GPIO GPIO_NUM_2
#define STACK_SIZE 2048

void blinkLEDLoop(void * pvParameters);

// extern "C" is used to prevent name mangling when using C++ compiler, so that the ESP-IDF can find the app_main function.
extern "C" void app_main(void)
{
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    // Create the task, storing the handle.  Note that the passed parameter ucParameterToPass
    // must exist for the lifetime of the task, so in this case is declared static.  If it was just an
    // an automatic stack variable it might no longer exist, or at least have been corrupted, by the time
    // the new task attempts to access it.
    xTaskCreate( blinkLEDLoop, "BlinkLEDTask", STACK_SIZE, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle );
        configASSERT( xHandle );

    // Use the handle to delete the task.
    // if( xHandle != NULL )
    // {
    //  vTaskDelete( xHandle );
    // }
    BluetoothHFClient::Initialize();
}

void blinkLEDLoop(void * pvParameters)
{
    // Configure the GPIO pin
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // Blink loop
    while (1) {
        // Turn LED ON
        printf("LED ON\n");
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 second

        // Turn LED OFF
        printf("LED OFF\n");
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 second
    }
}
