# Relatório — Sensor Ultrassônico HC-SR04

## Nome
Felipe Beserra de Oliveira

---

## Número USP
13683702

---

## Respostas, comentários e análises

### Descrição da Atividade

O experimento implementa uma biblioteca para medir distâncias com o sensor ultrassônico HC-SR04 conectado à placa FRDM-KL25Z. O módulo utiliza um pulso de *trigger* e mede a duração do pulso de *echo* para calcular a distância.

### Funcionamento do HC-SR04

| Sinal | Pino | Direção | Descrição |
|---|---|---|---|
| Trigger | PTB2 | Saída | Pulso HIGH de ≥ 10 µs inicia medição |
| Echo | PTB3 | Entrada | Pulso HIGH com duração proporcional à distância |

**Sequência de operação:**
1. Enviar pulso HIGH de 10 µs no pino *trigger*
2. O módulo emite 8 pulsos ultrassônicos a 40 kHz
3. O pino *echo* fica HIGH durante o tempo de ida e volta do som
4. Calcular a distância a partir da duração do pulso

### Cálculo da distância

A velocidade do som no ar (~20 °C) é ≈ 343 m/s = 0,0343 cm/µs.
Como o som percorre a distância duas vezes (ida e volta):

```
distância (cm) = duração_echo (µs) × 0,0343 / 2
               = duração_echo (µs) × 0,017
```

Implementação em inteiro (evita ponto flutuante):

```c
dist_cm = echo_us * 17 / 1000;
```

### Medição de tempo via GPIO interrupt + `k_cycle_get_32()`

A borda de subida do *echo* registra `echo_start_cycles` e a borda de descida registra `echo_end_cycles` via ISR (`GPIO_INT_EDGE_BOTH`). A diferença é convertida para microsegundos usando `sys_clock_hw_cycles_per_sec()`:

```c
uint32_t cycles_per_us = sys_clock_hw_cycles_per_sec() / 1000000U;
uint32_t echo_us = (echo_end_cycles - echo_start_cycles) / cycles_per_us;
```

Essa abordagem é mais precisa que polling puro e não depende da resolução de `k_uptime_get_32()` (1 ms).

### Timeout e alcance

O HC-SR04 tem alcance máximo de ~4–5 m, correspondendo a ~29 000 µs de *echo*. Um timeout de 30 ms é implementado para evitar bloqueio indefinido quando não há objeto na frente.

### Taxa de medição

Uma medição a cada 100 ms (10 Hz) é suficiente para a maioria das aplicações de robótica. O intervalo mínimo recomendado pelo fabricante é 60 ms para evitar interferência entre pulsos.

---

## Código (main.c)

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define TRIGGER_PIN     2
#define ECHO_PIN        3
#define SENSOR_GPIO     DT_NODELABEL(gpiob)
#define MEASURE_INTERVAL_MS  100
#define ECHO_TIMEOUT_US      30000

static const struct device *gpiob = DEVICE_DT_GET(SENSOR_GPIO);
static volatile uint32_t echo_start_cycles, echo_end_cycles;
static volatile bool echo_received;
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
    gpio_pin_set(gpiob, TRIGGER_PIN, 1);
    k_busy_wait(10);
    gpio_pin_set(gpiob, TRIGGER_PIN, 0);

    uint32_t deadline = k_uptime_get_32() + ECHO_TIMEOUT_US / 1000 + 5;
    while (!echo_received) {
        if (k_uptime_get_32() > deadline) return -1;
        k_usleep(10);
    }

    uint32_t delta_cycles = echo_end_cycles - echo_start_cycles;
    uint32_t cycles_per_us = sys_clock_hw_cycles_per_sec() / 1000000U;
    uint32_t echo_us = delta_cycles / cycles_per_us;
    return (int)(echo_us * 17 / 1000);
}

int main(void)
{
    gpio_pin_configure(gpiob, TRIGGER_PIN, GPIO_OUTPUT_LOW);
    gpio_pin_configure(gpiob, ECHO_PIN,    GPIO_INPUT);
    gpio_pin_interrupt_configure(gpiob, ECHO_PIN, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&echo_cb, echo_isr, BIT(ECHO_PIN));
    gpio_add_callback(gpiob, &echo_cb);

    while (1) {
        int dist = measure_distance_cm();
        if (dist < 0) printk("Distancia: timeout\n");
        else          printk("Distancia: %d cm\n", dist);
        k_msleep(MEASURE_INTERVAL_MS);
    }
    return 0;
}
```

---

## Repositório

```text
https://github.com/Beserrovsky/Atividade-4
```
