#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/*
 * HC-SR04 ultrasonic sensor
 *
 * Trigger: send a 10 µs HIGH pulse → sensor emits ultrasonic burst.
 * Echo:    pin goes HIGH for a duration proportional to measured distance.
 *          distance_cm = echo_duration_µs * 0.017
 *
 * Adjust pins to match your wiring.
 */
#define TRIGGER_PIN     2   /* PTB2 */
#define ECHO_PIN        3   /* PTB3 */
#define SENSOR_GPIO     DT_NODELABEL(gpiob)

#define MEASURE_INTERVAL_MS     100     /* 10 measurements per second */
#define ECHO_TIMEOUT_US         30000   /* ~5 m max range */

static const struct device *gpiob = DEVICE_DT_GET(SENSOR_GPIO);

static volatile uint32_t echo_start_cycles;
static volatile uint32_t echo_end_cycles;
static volatile bool     echo_received;

static struct gpio_callback echo_cb;

static void echo_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (gpio_pin_get(dev, ECHO_PIN)) {
        echo_start_cycles = k_cycle_get_32();
    } else {
        echo_end_cycles = k_cycle_get_32();
        echo_received   = true;
    }
}

static int measure_distance_cm(void)
{
    echo_received = false;

    /* Trigger: 10 µs HIGH pulse */
    gpio_pin_set(gpiob, TRIGGER_PIN, 1);
    k_busy_wait(10);
    gpio_pin_set(gpiob, TRIGGER_PIN, 0);

    /* Wait for echo with timeout */
    uint32_t deadline = k_uptime_get_32() + ECHO_TIMEOUT_US / 1000 + 5;
    while (!echo_received) {
        if (k_uptime_get_32() > deadline) {
            return -1;  /* timeout — no object in range */
        }
        k_usleep(10);
    }

    /* Convert cycle delta to microseconds */
    uint32_t delta_cycles = echo_end_cycles - echo_start_cycles;
    uint32_t cycles_per_us = sys_clock_hw_cycles_per_sec() / 1000000U;
    uint32_t echo_us = delta_cycles / cycles_per_us;

    /* distance = echo_us * speed_of_sound / 2 = echo_us * 0.017 cm */
    return (int)(echo_us * 17 / 1000);
}

int main(void)
{
    if (!device_is_ready(gpiob)) {
        return -1;
    }

    gpio_pin_configure(gpiob, TRIGGER_PIN, GPIO_OUTPUT_LOW);
    gpio_pin_configure(gpiob, ECHO_PIN,    GPIO_INPUT);

    /* Configure echo interrupt on both edges */
    gpio_pin_interrupt_configure(gpiob, ECHO_PIN, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&echo_cb, echo_isr, BIT(ECHO_PIN));
    gpio_add_callback(gpiob, &echo_cb);

    printk("=== Sensor Ultrassonico HC-SR04 ===\n");
    printk("Trigger: PTB%d | Echo: PTB%d\n", TRIGGER_PIN, ECHO_PIN);

    while (1) {
        int dist = measure_distance_cm();
        if (dist < 0) {
            printk("Distancia: timeout (sem objeto em alcance)\n");
        } else {
            printk("Distancia: %d cm\n", dist);
        }
        k_msleep(MEASURE_INTERVAL_MS);
    }

    return 0;
}
