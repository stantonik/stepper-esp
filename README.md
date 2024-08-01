# stepper-esp

ESP-IDF library for bipolar stepper motor drivers with STEP/DIR interface.

## Features
- Unlimited stepper motor
- Non-blocking control
- Constant speed mode
- Linear (accelerated) speed mode

## Known limitations
- Lack of testing, could be unstable
- Unlimited stepper motor but performance and precision decrease
- Max speed of ~100.000steps/sec for 240MHz clock config (ex: 400mm/s or 24.000mm/min for 1.8° step angle motor with 8mm lead screw)
- No control of the motor during a move in accelerated mode (apart from the stop)

## Example of usage

```c
#include "stepperesp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

motor_handle_t motor;

void app_main(void)
{
    struct motor_config motor_config =
    {
        .dir_pin = 25,
        .step_pin = 26,
        .en_pin = -1,
        .steps_per_rev = 200,
        .microsteps = 16,
        .name = 'X'             // especially for debugging
    };

    motor_create(&motor_config, &motor);
    esp_err_t ret = motor_init(motor);
    if (ret != ESP_OK)
    {
        return;
    }

    motor_set_profile(motor, MOTOR_PROFILE_LINEAR, 1000, 1000);

    motor_turn(motor, 200 * 10, 10);    // rotate 10 turns at 10step/sec (microstepping already taken into account)

    for(;;)
    {
        if (motor_get_state == MOTOR_STATE_STILL) printf("Motor %c has finished running !", motor_get_name(motor));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```
