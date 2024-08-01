/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : stepperesp
 * @created     : Wednesday Jul 31, 2024 02:46:48 CEST
 */

#ifndef STEPPERESP_H
#define STEPPERESP_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*          INCLUDES          */
/******************************/
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "driver/gptimer.h"

/******************************/
/*      Macro Definitions     */
/******************************/
#define MAX_MOTOR_COUNT 4

/******************************/
/*   Typedefs, Struct, Enums  */
/******************************/
enum motor_state : uint8_t
{
  MOTOR_STATE_DISABLE,
  MOTOR_STATE_STILL,
  MOTOR_STATE_ACCEL,
  MOTOR_STATE_DECEL,
  MOTOR_STATE_CRUISE,
};

enum motor_dir : uint8_t
{
  MOTOR_DIR_CW,
  MOTOR_DIR_CCW
};

enum motor_profile_type : uint8_t
{
  MOTOR_PROFILE_CONSTANT,
  MOTOR_PROFILE_LINEAR,
};

struct motor_profile_config
{
  enum motor_profile_type type;
  uint32_t accel;
  uint32_t decel;
};

struct motor_config
{
  gpio_num_t dir_pin;
  gpio_num_t step_pin;
  gpio_num_t en_pin;
  uint16_t steps_per_rev;
  uint16_t microsteps;
  char name;
};

typedef void* motor_handle_t;

/******************************/
/*     Global Variables       */
/******************************/

/******************************/
/*   Function Declarations    */
/******************************/
extern esp_err_t motor_create(struct motor_config *config, motor_handle_t *handle);
extern esp_err_t motor_enable(motor_handle_t handle);
extern esp_err_t motor_disable(motor_handle_t handle);
extern esp_err_t motor_delete(motor_handle_t *handle);
extern esp_err_t motor_delete_all();

extern esp_err_t motor_turn_mm(motor_handle_t handle, float x, float speed);
extern esp_err_t motor_turn_full_step(motor_handle_t handle, int32_t steps, float speed);
extern esp_err_t motor_turn(motor_handle_t handle, float steps, float speed);

extern float motor_get_current_speed(motor_handle_t handle);
extern float motor_get_target_speed(motor_handle_t handle);
extern enum motor_state motor_get_state(motor_handle_t handle);
extern uint32_t motor_get_remaining_steps(motor_handle_t handle);
extern uint16_t motor_get_microstepping(motor_handle_t handle);
extern uint16_t motor_get_steps_per_rev(motor_handle_t handle);
extern char motor_get_name(motor_handle_t handle);
extern enum motor_dir motor_get_direction(motor_handle_t handle);
extern int32_t motor_get_position_fullstep(motor_handle_t handle);
extern float motor_get_position(motor_handle_t handle);
extern float motor_get_position_mm(motor_handle_t handle);

extern esp_err_t motor_set_profile(motor_handle_t handle, struct motor_profile_config *profile);
extern esp_err_t motor_set_lead(motor_handle_t handle, float lead_mm);

// TODO :
extern esp_err_t motor_stop(motor_handle_t handle);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STEPPERESP_H */
