#include <stdio.h>

#include "stepperesp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


motor_handle_t xmotor;
motor_handle_t ymotor;
motor_handle_t zmotor;
motor_handle_t emotor;

void app_main(void)
{
  struct motor_config motor_config = { .dir_pin = 26, .step_pin = 25, .steps_per_rev = 48, .name = 'X' };

  motor_create(&motor_config, &xmotor);
  motor_enable(xmotor);

  /* motor_create(&motor_config, &ymotor); */
  /* motor_enable(ymotor); */

  /* motor_create(&motor_config, &zmotor); */
  /* motor_enable(zmotor); */

  /* motor_create(&motor_config, &emotor); */
  /* motor_enable(emotor); */
  
  struct motor_profile_config profile_cfg = 
  {
    .type = MOTOR_PROFILE_LINEAR,
    .accel = 7000 * 16,
    .decel = 7000 * 16,
  };


  motor_set_profile(xmotor, &profile_cfg);
  /* motor_set_profile(ymotor, &profile_cfg); */
  /* motor_set_profile(zmotor, &profile_cfg); */
  /* motor_set_profile(emotor, &profile_cfg); */
  while(1)
  {
    if (motor_get_state(xmotor) == MOTOR_STATE_STILL) 
    {
      motor_turn(xmotor, (motor_get_direction(xmotor) == MOTOR_DIR_CW ? -1 : 1) * 48 * 7 * 16, 48 * 15 * 16);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  /* motor_turn(xmotor,  48 * 10 * 16, 48 * 10 * 16); */
  /* motor_turn(ymotor, 200 * 10, 1000); */
  /* motor_turn(zmotor, 200 * 10, 1000); */
  /* motor_turn(emotor, 200 * 10, 1000); */


  for(;;)
  {
    /* printf("state=%i\nspeed=%.2f\nremaining steps=%i\n", (int)motor_get_state(xmotor), motor_get_current_speed(xmotor), (int)motor_get_remaining_steps(xmotor)); */
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
