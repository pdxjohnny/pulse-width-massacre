#include <adc.h>
#include <gpio.h>
#include <flextimus.h>
#include "config.h"
#include "hd44780.h"

/* The state of Flextimus Prime. This controls the main loop. */
typedef enum {
  FLEXTIMUS_PRIME_ON,
  FLEXTIMUS_PRIME_OFF,
  FLEXTIMUS_PRIME_SLEEP,
} flextimus_prime_state_t;

/* The ADC state type lets us keep track of what we should be doing with the ADC
 * when we get interrupts. */
typedef enum {
  ADC_IDLE,
  ADC_READY,
  ADC_CONVERTED,
} adc_state_t;

/* One global to rule them all */
struct {
  bool paused;
  struct {
    uint32_t volts;
    uint32_t millivolts;
    adc_state_t state;
  } adc;
  flextimus_prime_state_t state;
} flextimus_prime;

void assert_failed(uint8_t* file, uint32_t line) {
  while (1) {}
}

void delay(int dly) {
  while (dly--);
}

/* Called when the ADC finishes a conversion */
adc_status_t adc_convert_async_callback(adc_status_t adc_status) {
  flextimus_prime.adc.volts = ADC_VOLTS(adc_status.data);
  flextimus_prime.adc.millivolts = ADC_MILLIVOLTS(adc_status.data);
  gpio_off(PAUSE_LED);
  flextimus_prime.state = FLEXTIMUS_PRIME_OFF;
  return ADC_OK;
}

/* Called when the ADC is ready */
adc_status_t adc_adrdy_callback(adc_status_t adc_status) {
  adc_status = adc_convert_async(FLEX_SENSOR, adc_convert_async_callback);
  if (ADC_ERROR(adc_status)) {
    assert_failed(__FILE__, __LINE__);
  }

  return ADC_OK;
}

void flextimus_prime_init() {
  flextimus_prime.adc.state = ADC_IDLE;
}

int main(void) {
  bool running = true;
  adc_status_t adc_status;

  __enable_irq();

  gpio_up(PAUSE_LED);

  adc_status = adc_up(FLEX_SENSOR, adc_adrdy_callback);
  if (ADC_ERROR(adc_status)) {
    assert_failed(__FILE__, __LINE__);
  }

  // gpio_on(PAUSE_LED);

  //LCD Test code. NOTE: WILL HANG FOREVER IF YOU DON'T HAVE AN LCD ATTACHED
  //TODO_MAX: Update documentation with physical set up. Replicate final schematic set up.

  /*HD44780_Setup();
  HD44780_PowerOn();
  HD44780_Puts((uint8_t *)"Hello");
  HD44780_GotoXY(2,1);
  HD44780_Puts((uint8_t *)"World!");
  delay(5000000);
  HD44780_Clear();*/

  while (running) {
    switch (flextimus_prime.state) {
    case FLEXTIMUS_PRIME_SLEEP:
      PWR_EnterSleepMode(PWR_SLEEPEntry_WFI);
    case FLEXTIMUS_PRIME_OFF:
      running = false;
    case FLEXTIMUS_PRIME_ON:
    default:
      continue;
    }
  }

  gpio_down(PAUSE_LED);

  adc_status = adc_down(FLEX_SENSOR);
  if (ADC_ERROR(adc_status)) {
    assert_failed(__FILE__, __LINE__);
  }

  /* The fall of Flextimus Prime */
  assert_param(NULL);

  return 0;
}

// Function to pause the alert system
void flextimus_prime_pause_pressed() {
  if (flextimus_prime.paused == 0) {
    flextimus_prime.paused = true;
    gpio_on(PAUSE_LED);
  } else {
    flextimus_prime.paused = false;
    gpio_off(PAUSE_LED);
  }
}

void flextimus_prime_config_pressed() {
  if (flextimus_prime.paused == 0) {
    flextimus_prime.paused = true;
    gpio_on(PAUSE_LED);
  } else {
    flextimus_prime.paused = false;
    gpio_off(PAUSE_LED);
  }
}

// IRQ handler for both button interrupts
void button_irq_handler() {
  if (gpio_asserted(PAUSE_BUTTON)) {
    flextimus_prime_pause_pressed();
  } else if (gpio_asserted(CONFIG_BUTTON)) {
    flextimus_prime_config_pressed();
  }
}
