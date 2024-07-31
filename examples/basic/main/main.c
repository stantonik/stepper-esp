#include <stdio.h>

#include "stepperesp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


motor_handle_t xmotor;

void app_main(void)
{
  struct motor_config motor_config = { .dir_pin = 26, .step_pin = 25, .steps_per_rev = 48, .name = 'X' };

  motor_create(&motor_config, &xmotor);
  motor_enable(xmotor);
  
  struct motor_profile_config profile_cfg = 
  {
    .type = MOTOR_PROFILE_LINEAR,
    .accel = 100,
    .decel = 100,
  };

  motor_set_profile(xmotor, &profile_cfg);
  motor_turn(xmotor, 48 * 10, 40);

  for(;;)
  {
    printf("state : %i, remaining steps : %i\n",(int)motor_get_state(xmotor), (int)motor_get_remaining_steps(xmotor));
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
