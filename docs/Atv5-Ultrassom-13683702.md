# Relatório — Sensor Ultrassônico HC-SR04 (RELATÓRIO PARCIAL — bloqueado por hardware externo)

## Nome

Felipe Beserra de Oliveira

---

## Número USP

13683702

---

## Status: bloqueado por hardware externo

Esta atividade **não pode ser concluída nem testada** sem o módulo ultrassônico físico (HC-SR04 ou equivalente). As seções abaixo documentam o que já foi levantado e exatamente o que falta.

## O que o enunciado pede

1. Gerar o pulso de *trigger* do HC-SR04 usando **PWM** (pulso de pelo menos 10µs), com frequência/duty cycle escolhidos conforme a taxa de medição desejada.
2. Medir a largura do pulso de *echo* usando **Input Capture** (módulo TPM), baseado no tutorial "Input Capture (Captura)".
3. Calcular a distância a partir da largura do pulso (`distância_cm ≈ tempo_echo_µs × 0.017`).
4. Encapsular o código validado em uma biblioteca reutilizável (`lib/`), como nas demais atividades (ex.: `lib/pwm/pwm_z42`).

## O que já existe no repositório (e por que não está validado)

O `src/main.c` atual já implementa trigger e echo, **mas usando a API genérica de GPIO do Zephyr + interrupção** (`gpio_pin_interrupt_configure`, medindo a largura do pulso com `k_cycle_get_32()`), em vez de PWM + Input Capture via registradores do TPM como pede o enunciado. Funcionalmente é uma abordagem válida para medir o eco, mas não é o método pedido, então:

- A fiação proposta (PTB2 = trigger, PTB3 = echo) está correta e deve ser mantida — esses pinos correspondem a `TPM2_CH0`/`TPM2_CH1`, exatamente os canais necessários para a implementação via TPM.
- A lógica de trigger/echo deve ser reescrita para usar o TPM2 em modo PWM (canal 0, trigger) e Input Capture (canal 1, echo), em vez da API de GPIO.
- Nada disso pode ser validado sem o módulo físico — escrever a versão register/TPM "no escuro" e empacotá-la como pronta seria arriscado (a HC-SR04 é sensível a timing, e um erro de configuração do TPM só aparece com o sensor real conectado).

### TODO — pendências para finalizar este relatório (em ordem)

1. Obter o módulo HC-SR04 (ou equivalente) e os jumpers/protoboard necessários.
2. Conectar: `VCC`→5V, `GND`→GND, `Trig`→PTB2, `Echo`→PTB3 **com um divisor de tensão** (o echo do HC-SR04 opera em 5V; o pino do KL25Z tolera no máximo 3.3V — ligar direto pode danificar o microcontrolador).
3. Reescrever `src/main.c` usando TPM2 (PWM no canal 0 para o trigger, Input Capture no canal 1 para o echo), com base no padrão de registradores já usado nas Atividades 2/3/4.
4. Flashar e validar com objetos a distâncias conhecidas, capturando as leituras para o relatório.
5. Finalizar o relatório com os dados reais e, se tudo funcionar, extrair a lógica para uma biblioteca em `lib/`.

---

## Repositório

```text
https://github.com/Beserrovsky/PSI3441-ATV5_ultrassom-zephyr
```
