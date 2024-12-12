#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

const uint LED_PIN = 25; // pico/pico 2
//const uint LED_PIN = 7; // feather

const uint ADDR_WIDTH = 10;
const uint DATA_WIDTH = 4;

const uint MIN_ADDR_PIN = 0;
const uint MAX_ADDR_PIN = ADDR_WIDTH - 1;
const uint NOT_W_PIN    = MAX_ADDR_PIN + 1;
const uint NOT_S_PIN    = MAX_ADDR_PIN + 2;
const uint MIN_DATA_PIN = NOT_S_PIN    + 1;
const uint MAX_DATA_PIN = MIN_DATA_PIN + DATA_WIDTH - 1;

const uint MIN_PIN      = MIN_ADDR_PIN;
const uint MAX_PIN      = MAX_DATA_PIN;

const uint32_t ADDR_MASK = (1 << ADDR_WIDTH) - 1;
const uint32_t DATA_MASK = ((1 << DATA_WIDTH) - 1) << MIN_DATA_PIN;
const uint32_t ALL_MASK  = (1 << MAX_PIN + 1) - 1;

void set_address(uint32_t addr) {
  gpio_put_masked(ADDR_MASK, addr);
  gpio_set_dir_out_masked(ADDR_MASK);
}

void float_address() {
  gpio_set_dir_in_masked(ADDR_MASK);
}

// NOTE: timings for for -45 chip. We will be way above all of those!
void write_data(uint32_t addr, uint16_t data) {
  set_address(addr);
  gpio_put(NOT_W_PIN, 0); // address setup time 0 ns
  gpio_put(NOT_S_PIN, 0);
  gpio_put_masked(DATA_MASK, data << MIN_DATA_PIN);
  gpio_set_dir_out_masked(DATA_MASK); // data setup time 200 ns
  sleep_us(2);
  gpio_put(NOT_W_PIN, 1); // write pulse width 200 ns
  gpio_put(NOT_S_PIN, 1); // chip select setup time 200 ns
  gpio_set_dir_in_masked(DATA_MASK); // data hold time 0 ns
  float_address(); // address hold time 20 ns
}

uint16_t read_data(uint32_t addr) {
  set_address(addr);
  gpio_put(NOT_W_PIN, 1); // should already be the case!!!
  gpio_put(NOT_S_PIN, 0); // access times: 120 ns from /CS, 450 ns from address
  sleep_us(2);
  uint16_t result = (gpio_get_all() & DATA_MASK) >> MIN_DATA_PIN;
  gpio_put(NOT_S_PIN, 1);
  float_address();
  return result;
}

int main() {

//  bi_decl(bi_program_description("This is a test binary."));
//  bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

  stdio_init_all();

  // The blinking LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  //  Everything in our range is an I/O
  gpio_init_mask(ALL_MASK);
  gpio_set_dir_in_masked(ALL_MASK);  // all in at first so that nothing is out on the address and data buses
  gpio_set_dir(NOT_W_PIN, GPIO_OUT); // but /W and /S must be out and high
  gpio_set_dir(NOT_S_PIN, GPIO_OUT);
  gpio_put(NOT_W_PIN, 1);
  gpio_put(NOT_S_PIN, 1);

  sleep_ms(2000);

  uint16_t addresses_checked = 0;
  uint16_t values_checked    = 0;
  uint16_t incorrect         = 0;

  for (uint32_t address = 0; address < 1024; address++) {
    for (uint16_t data = 0; data < 16; data++) {
      write_data(address, data);
      uint16_t r = read_data(address);
      values_checked++;
      if (data != r) {
        incorrect++;
//        printf("incorrect value read back for address %#010x and data %#010x\n", address, data);
        putchar('X');
      } else {
        putchar('.');
//        printf("ok for address %#010x and data %#010x\n", address, data);
      }
//      sleep_ms(1);
    }
    putchar('\n');
    addresses_checked++;
  }

  printf("DONE: checked %d addresses, %d total values, with %d incorrect reads.\n", addresses_checked, values_checked, incorrect);

  // Done
  uint32_t delay = incorrect > 0 ? 1000: 125;
  while (1) {
    gpio_put(LED_PIN, 0);
    sleep_ms(delay);
    gpio_put(LED_PIN, 1);
    sleep_ms(delay);
  }
}
