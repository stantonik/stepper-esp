/**
 * @author      : stanleyarn (stanleyarn@$HOSTNAME)
 * @file        : stepperesp
 * @created     : Wednesday Jul 31, 2024 02:46:40 CEST
 */

/******************************/
/*         Includes           */
/******************************/
#include "stepperesp.h"

#include <string.h>
#include <stddef.h>

#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_check.h"

/******************************/
/*     Static Variables       */
/******************************/
#define TAG "stepper-esp"

typedef struct
{
  gpio_num_t dir_pin;
  gpio_num_t step_pin;
  gpio_num_t en_pin;
  uint16_t steps_per_rev;
  uint16_t microsteps;
  char name;

  enum motor_state state;
  enum motor_dir dir;
  bool ready;
  bool is_en_pin; 

  uint32_t timer_us;
  uint32_t remaining_steps;
  uint32_t traveled_steps;
  uint32_t target_speed;
  uint32_t current_speed;
  enum motor_profile_type profile;
  uint32_t accel;
  uint32_t decel;
  uint32_t accel_inc;
  uint32_t decel_inc;
  uint32_t accel_steps;
  uint32_t decel_steps;
  uint32_t real_step_per_rev;
} motor_ctrl_t;

static gptimer_handle_t timer_handle;
static motor_handle_t *handles[MAX_MOTOR_COUNT] = {  };
static motor_ctrl_t *motors[MAX_MOTOR_COUNT] = {  };
static uint8_t motor_count = 0;

/******************************/
/*    Function Prototypes     */
/******************************/
static bool timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

/******************************/
/*   Function Definitions     */
/******************************/
esp_err_t motor_create(struct motor_config *config, motor_handle_t *handle)
{
  ESP_LOGI(TAG, "creating a new motor...");
  ESP_RETURN_ON_FALSE(config != NULL && handle != NULL, -1, TAG, "invalid parameters");
  ESP_RETURN_ON_FALSE(motor_count < MAX_MOTOR_COUNT, -1, TAG, "max motor count reached");

  /* Check if the config is legit */
  ESP_RETURN_ON_FALSE(config->steps_per_rev >= 20 && config->steps_per_rev <= 2000, -1, TAG, "invalid step angle");
  ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(config->dir_pin), -1, TAG, "dir gpio not valid");
  ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(config->step_pin), -1, TAG, "step gpio not valid");
  bool is_en_pin = true;
  if (config->en_pin == 0)
  {
    is_en_pin = false;
  }
  else
  {
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(config->en_pin), -1, TAG, "en gpio not valid");
  }

  /* If the first motor is created, initialize the timer */
  if (motor_count == 0)
  {
    ESP_LOGI(TAG, "initializing motor timer...");

    static gptimer_config_t timer_cfg =
    {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1 * 1000 * 1000,
      .intr_priority = 0
    };

    static gptimer_alarm_config_t alarm_cfg =
    {
      .alarm_count = 10,
      .reload_count = 0,
      .flags = { .auto_reload_on_alarm = true }
    };

    gptimer_event_callbacks_t timer_cb = { .on_alarm = timer_callback };

    ESP_RETURN_ON_ERROR(gptimer_new_timer(&timer_cfg, &timer_handle) , TAG, "failed to create the motor timer");
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(timer_handle, &timer_cb, NULL), TAG, "failed to pass motor timer callback");
    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(timer_handle, &alarm_cfg), TAG, "failed to set the motor timer alarm");
    ESP_RETURN_ON_ERROR(gptimer_enable(timer_handle), TAG, "failed to enable motor timer");
    ESP_RETURN_ON_ERROR(gptimer_start(timer_handle), TAG, "failed to start motor timer");
    ESP_LOGI(TAG, "motor timer initialized");
  }

  /* Create motor */
  motor_ctrl_t *motor = malloc(sizeof(motor_ctrl_t));
  ESP_RETURN_ON_FALSE(motor != NULL, -1, TAG, "cannot allocate memory");
  *handle = (void *)motor;
  for (int i = 0; i < MAX_MOTOR_COUNT; i++)
  {
    if (motors[i] == NULL)
    {
      handles[i] = handle;
      motors[i] = motor;
      break;
    }
  }

  /* Initialize GPIO */
  gpio_reset_pin(config->dir_pin);
  gpio_set_direction(config->dir_pin, GPIO_MODE_OUTPUT);
  gpio_reset_pin(config->step_pin);
  gpio_set_direction(config->en_pin, GPIO_MODE_OUTPUT);
  if (is_en_pin)
  {
    gpio_reset_pin(config->en_pin);
    gpio_set_direction(config->en_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(config->en_pin, 1);
  }

  /* Initialise values */
  *motor = (motor_ctrl_t){  };
  memcpy(motor, config, sizeof(struct motor_config));
  motor->is_en_pin = is_en_pin;
  motor->real_step_per_rev = config->steps_per_rev * config->microsteps;
  if (config->microsteps == 0) motor->microsteps = 1;

  motor_count++;
  ESP_LOGI(TAG, "motor %c created", config->name);
  return ESP_OK;
}

#define GET_MOTOR(handle, code) \
  (motor_ctrl_t *)handle; \
  ESP_RETURN_ON_FALSE(handle != NULL, code, TAG, "null motor");

esp_err_t motor_enable(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, -1);

  if (motor->is_en_pin) gpio_set_level(motor->en_pin, 0);
  motor->state = MOTOR_STATE_STILL;

  return ESP_OK;
}

esp_err_t motor_disable(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, -1);

  if (motor->is_en_pin) gpio_set_level(motor->en_pin, 1);
  motor->state = MOTOR_STATE_DISABLE;

  return ESP_OK;
}

esp_err_t motor_delete(motor_handle_t *handle)
{
  motor_ctrl_t *motor = GET_MOTOR(*handle, -1);

  char name_cp = motor->name;

  /* Delete timer if the last motor was removed */
  uint8_t motor_count_cp = motor_count - 1;
  if (motor_count_cp <= 0)
  {
    gptimer_stop(timer_handle);
    gptimer_disable(timer_handle);
    ESP_RETURN_ON_ERROR(gptimer_del_timer(timer_handle), TAG, "failed to delete motor timer");
  }

  motor_count--;

  for (int i = 0; i < MAX_MOTOR_COUNT; i++)
  {
    if (motor == motors[i])
    {
      free(motor);
      *handle = NULL;
      handles[i] = NULL;
      motors[i] = NULL;
      break;
    }
  }

  ESP_LOGW(TAG, "motor %c deleted", name_cp);
  return ESP_OK;
}

esp_err_t motor_delete_all()
{
  for (int i = 0; i < MAX_MOTOR_COUNT; i++)
  {
    if (handles[i] != NULL)
    {
      esp_err_t ret = motor_delete(handles[i]);
      if (ret != ESP_OK) return ret;
    }
  }

  return ESP_OK;
}



uint32_t motor_get_current_speed(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, 0);
  return motor->current_speed;
}

uint32_t motor_get_target_speed(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, 0);
  return motor->target_speed;
}

enum motor_state motor_get_state(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, -1);
  return motor->state;
}

uint32_t motor_get_remaining_steps(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, 0);
  return motor->remaining_steps;
}

uint32_t motor_get_traveled_steps(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, 0);
  return motor->traveled_steps;
}

uint16_t motor_get_microstepping(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, 0);
  return motor->microsteps;
}

uint16_t motor_get_steps_per_rev(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, 0);
  return motor->steps_per_rev;
}

char motor_get_name(motor_handle_t handle)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, '\0');
  return motor->name;
}

esp_err_t motor_set_profile(motor_handle_t handle, struct motor_profile_config *profile)
{
  motor_ctrl_t *motor = GET_MOTOR(handle, -1);
  ESP_RETURN_ON_FALSE(profile != NULL, -1, TAG, "profile is null");

  memcpy(motor + offsetof(motor_ctrl_t, profile), profile, sizeof(struct motor_profile_config));

  return ESP_OK;
}


// ISR
bool timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
  return true;
}
