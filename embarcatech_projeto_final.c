// -- Inclusão de bibliotecas
#include <stdio.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "pico/bootrom.h"

// -- Definição de constantes
// GPIO
#define button_A 5 // Botão A GPIO 5
#define button_B 6 // Botão B GPIO 6
#define matriz_leds 7 // Matriz de LEDs GPIO 7
#define NUM_LEDS 25 // Número de LEDs na matriz
#define buzzer_A 21 // Buzzer A GPIO 21
#define buzzer_B 10 // Buzzer B GPIO 10
#define LED_Green 11 // LED Verde GPIO 11
#define LED_Red 13 // LED Vermelho GPIO 13
#define joystick_PB 22 // Botão do joystick GPjoystick_PB
#define joystick_Y 26 // VRY do Joystick GPIO 26
#define joystick_X 27 // VRX do Joystick GPIO 27
#define mic 28 // Mic GPIO 28

// Display I2C
#define display_i2c_port i2c1 // Define a porta I2C
#define display_i2c_sda 14 // Define o pino SDA na GPIO 14
#define display_i2c_scl 15 // Define o pino SCL na GPIO 15
#define display_i2c_endereco 0x3C // Define o endereço do I2C
ssd1306_t ssd; // Inicializa a estrutura do display

// -- Variáveis globais
static volatile uint32_t last_time = 0; // Armazena o tempo do último clique dos botões

// Funcionalidade 1
volatile bool motor_activate = false; // Define se o motor está ou não ligado
volatile bool confirm_B = false; // Armazena se o botão B foi clicado nos ultimos 2s
volatile int segundos = 0; // Guarda o valor dos segundos desde a partida
volatile int horimetro = 1485; // Guarda o valor do horimetro
volatile int man_prev = 1500; // Guarda o valor do horímetro da próxima manutenção preventiva

// Funcionalidade 2
uint16_t value_vry; // Valor analógico do eixo Y jo Joystick
volatile float temperatura = 0; // Guarda o valor da temperatura do motor
volatile bool temp_alerta = false; // Guarda a informação se algum alerta de temperatura, desempenho ou vibrção foi ativado nos ultimos 5s

// Funcionalidade 3
uint16_t value_vrx; // Valor analógico do eixo X jo Joystick
volatile float combustivel; // Guarda o valor do nível de combustível
volatile float last_combustivel; // Guarda o valor do último nível de combustível
volatile float consumo; // Guarda o valor do consumo de combustível em l/h

// Funcionalidade 4
uint16_t value_mic; // Valor analógico do mic
volatile float vibracao = 0; // Guarda o valor da vibração do motor

// Strings para o display
char str_alerta[15]; // Guarda o valor da mensagem de alerta
char str_horimetro[4]; // Guarda o valor do horímetro em string
char str_man_prev[4]; // Guarda o valor da próxima manutenção preventiva em string
char str_temperatura[3]; // Guarda o valor da temperatura em string
char str_combustivel[3]; // Guarda o valor do nível de combustível em string
char str_consumo[3]; // Guarda o valor do consumo em string
char str_vibracao[3]; // Guarda o valor da vibração em string



// --- Funções necessária para a manipulação da matriz de LEDs

// Estrutura do pixel GRB (Padrão do WS2812)
struct pixel_t {
    uint8_t G, R, B; // Define variáveis de 8-bits (0 a 255) para armazenar a cor
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Permite declarar variáveis utilizando apenas "npLED_t"

// Declaração da Array que representa a matriz de LEDs
npLED_t leds[NUM_LEDS];

// Variáveis para máquina PIO
PIO np_pio;
uint sm;

// Função para definir a cor de um LED específico
void cor(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Função para desligar todos os LEDs
void desliga() {
    for (uint i = 0; i < NUM_LEDS; ++i) {
        cor(i, 0, 0, 0);
    }
}

// Função para enviar o estado atual dos LEDs ao hardware  - buffer de saída
void buffer() {
    for (uint i = 0; i < NUM_LEDS; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    // sleep_us(100);
}

// Função para converter a posição da matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

// --- Final das funções necessária para a manipulação da matriz de LEDs



// --- Funções de alarme

// Função de callback do alarme
int64_t alarm_callback(alarm_id_t id, void *user_data){
    ssd1306_draw_string(&ssd, "               ", 3, 13);
    desliga();
    buffer();
    confirm_B = false;
    temp_alerta = false;
    return 0;
}

// Função de callback do alarme do buzzer
int64_t alarm_callback_buzzer(alarm_id_t id, void *user_data){
    pwm_buzzer(buzzer_A, false);
    pwm_buzzer(buzzer_B, false);
    return 0;
}

// Função de alarme da temperatura
int64_t alarm_callback_temp(alarm_id_t id, void *user_data){
    motor_activate = false;
    gpio_put(LED_Green, motor_activate);
    gpio_put(LED_Red, !motor_activate);
    return 0;
}

// --- Final das funções de alarme



// --- Funções para configurar e beepar o buzzer

// Função para definir a frequência do som do buzzer
void pwm_freq(uint gpio, uint freq) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint clock_div = 4; // Define o divisor do clock
    uint wrap_value = (125000000 / (clock_div * freq)) - 1; // Define o valor do Wrap

    pwm_set_clkdiv(slice, clock_div); // Define o divisor do clock
    pwm_set_wrap(slice, wrap_value); // Define o contador do PWM
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), wrap_value / 2); // Duty cycle de 50%
}

// Função para ativar/desativar o buzzer
void pwm_buzzer(uint gpio, bool active){
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(slice, active);
}

// Função para beepar o buzzer
void beep_buzzer(uint time){
    pwm_buzzer(buzzer_A, true);
    pwm_buzzer(buzzer_B, true);
    add_alarm_in_ms(time, alarm_callback_buzzer, NULL, false);
}

// --- Final das funções para configurar e beepar o buzzer



// Função de alerta
void alerta(int tipo, int matriz, int time){
    if(!confirm_B){
        switch (tipo){
        case 1:
            // Tipo de alerta para manutenção preventiva perto
            ssd1306_draw_string(&ssd, "Prev proxima", 15, 13);
            break;
        case 2:
            // Tipo de alerta para manutenção preventiva vencida
            ssd1306_draw_string(&ssd, "Prev vencida", 15, 13);
            break;
        case 3:
            // Tipo de alerta para temperatura critica
            ssd1306_draw_string(&ssd, "Temp critica", 15, 13);
            break;
        case 4:
            // Tipo de alerta para temperatura alta
            ssd1306_draw_string(&ssd, "Temp alta", 27, 13);
            break;
        case 5:
            // Tipo de alerta para consumo elevado
            ssd1306_draw_string(&ssd, "Cons elevado", 15, 13);
            break;
        case 6:
            // Tipo de alerta para vibração anormal
            ssd1306_draw_string(&ssd, "Vib anormal", 19, 13);
            break;
        default:
            break;
        }

        switch (matriz){
        case 1:
            int frame1[5][5][3] = {
                {{0, 0, 0}, {0, 0, 0}, {100, 100, 0}, {0, 0, 0}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 0}, {100, 100, 0}, {0, 0, 0}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 0}, {100, 100, 0}, {0, 0, 0}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 0}, {100, 100, 0}, {0, 0, 0}, {0, 0, 0}}
            };
            for(int linha = 0; linha < 5; linha++){
                for(int coluna = 0; coluna < 5; coluna++){
                  int posicao = getIndex(linha, coluna);
                  cor(posicao, frame1[coluna][linha][0], frame1[coluna][linha][1], frame1[coluna][linha][2]);
                }
            };
            buffer();
            break;
        case 2:
            int frame2[5][5][3] = {
                {{0, 0, 0}, {100, 0, 0}, {100, 0, 0}, {100, 0, 0}, {0, 0, 0}},
                {{100, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {100, 0, 0}},  
                {{100, 0, 0}, {100, 0, 0}, {100, 0, 0}, {100, 0, 0}, {100, 0, 0}},
                {{100, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {100, 0, 0}},      
                {{0, 0, 0}, {100, 0, 0}, {100, 0, 0}, {100, 0, 0}, {0, 0, 0}} 
            };
            for(int linha = 0; linha < 5; linha++){
                for(int coluna = 0; coluna < 5; coluna++){
                  int posicao = getIndex(linha, coluna);
                  cor(posicao, frame2[coluna][linha][0], frame2[coluna][linha][1], frame2[coluna][linha][2]);
                }
            };
            buffer();
            break;
        case 3:
            int frame3[5][5][3] = {
                {{0, 0, 0}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 100}, {0, 0, 0}, {0, 0, 100}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 100}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
                {{0, 0, 0}, {0, 0, 100}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
            };
            for(int linha = 0; linha < 5; linha++){
                for(int coluna = 0; coluna < 5; coluna++){
                  int posicao = getIndex(linha, coluna);
                  cor(posicao, frame3[coluna][linha][0], frame3[coluna][linha][1], frame3[coluna][linha][2]);
                }
            };
            buffer();
            break;
        case 4:
            int frame4[5][5][3] = {
                {{100, 0, 100}, {100, 0, 100}, {100, 0, 100}, {0, 0, 0}, {100, 0, 100}},
                {{100, 0, 100}, {0, 0, 0}, {100, 0, 100}, {0, 0, 0}, {100, 0, 100}},
                {{100, 0, 100}, {100, 0, 100}, {100, 0, 100}, {0, 0, 0}, {100, 0, 100}},
                {{100, 0, 100}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
                {{100, 0, 100}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {100, 0, 100}}
            };
            for(int linha = 0; linha < 5; linha++){
                for(int coluna = 0; coluna < 5; coluna++){
                  int posicao = getIndex(linha, coluna);
                  cor(posicao, frame4[coluna][linha][0], frame4[coluna][linha][1], frame4[coluna][linha][2]);
                }
            };
            buffer();
            break;
        default:
            break;
        }

        beep_buzzer(time);
        add_alarm_in_ms(5000, alarm_callback, NULL, false);
    }
}

// Função de callback do temporizador (horímetro + consumo)
bool repeating_timer_callback(struct repeating_timer *t) {
    if(motor_activate){
        segundos += 1800;
        // Adiciona 1 hora ao horimetro a cada 3600 segundos (1 hora)
        if (segundos % 3600 == 0){
            horimetro++;
            if(horimetro % 10 == 0 && man_prev - horimetro <= 100){
                if(man_prev - horimetro <= 0){
                    alerta(2, 4, 500);
                }else{
                    alerta(1, 3, 100);
                }
            }
        }

        last_combustivel = combustivel;
        combustivel = (value_vrx/4095.0)*100.0; // Transforma a escala de 0 a 4095 para 0 a 305
        consumo = (last_combustivel - combustivel)/(1800/3600.0);
        if(!temp_alerta){
            if(consumo > 10){
                alerta(5, 1, 100);
                temp_alerta = true;
            }
        }
    }

    printf("Segundos: %d Horimetro: %d\n", segundos, horimetro);

    // Retorna true para manter o temporizador repetindo
    return true;
}

// Função para definir a temperatura e a vibração do motor
void temp_vib_motor(uint16_t value_vry, uint16_t value_mic){
    // Temperatura
    temperatura = 75.0 + ((value_vry/4095.0)*30.0); // Transforma a escala de 0 a 4095 para 75 a 105
    if(!temp_alerta){
        if(temperatura > 100){
            alerta(3, 2, 1000);
            add_alarm_in_ms(5000, alarm_callback_temp, NULL, false);
            temp_alerta = true;
        }else if(temperatura > 95){
            alerta(3, 2, 500);
            temp_alerta = true;
        }else if(temperatura > 92){
            alerta(4, 1, 100);
            temp_alerta = true;
        }
    }

    // Vibração
    vibracao = (value_mic/4095.0)*100; // Transforma a escala de 0 a 4095 para 0 a 100
    if(!temp_alerta && vibracao > 75){
        alerta(6, 1, 100);
        temp_alerta = true;
    }
}

// Função de interrupção dos botões
void gpio_irq_handler(uint gpio, uint32_t events){
    //Debouncing
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Pega o tempo atual e transforma em us
    if(current_time - last_time > 1000000){
        if(gpio == button_A){
            motor_activate = !motor_activate;
            gpio_put(LED_Green, motor_activate);
            gpio_put(LED_Red, !motor_activate);
            temp_alerta = false;
        }else if(gpio == button_B){
            if(confirm_B){
                man_prev += 500;
            }else{
                confirm_B = true;
                ssd1306_draw_string(&ssd, "Confirma?", 27, 13);
                add_alarm_in_ms(2000, alarm_callback, NULL, false);
            }
        }else if(gpio == joystick_PB){
            reset_usb_boot(0, 0);
        }
    }
}

// Função para atualizar o display
void atualizar_display(){

    // Cria as bordas do display
    ssd1306_rect(&ssd, 0, 0, 127, 63, true, false); // Borda principal
    ssd1306_line(&ssd, 1, 11, 126, 11, true); // Desenha uma linha horizontal
    ssd1306_line(&ssd, 1, 22, 126, 22, true); // Desenha uma linha horizontal
    ssd1306_line(&ssd, 1, 43, 126, 43, true); // Desenha uma linha horizontal
    ssd1306_line(&ssd, 42, 23, 42, 62, true); // Desenha uma linha vertical
    ssd1306_line(&ssd, 85, 23, 85, 62, true); // Desenha uma linha vertical

    // Escritas fixas
    ssd1306_draw_string(&ssd, "MANUTENCAO", 23, 2); // Desenha uma string
    ssd1306_draw_string(&ssd, "HOR", 9, 24); // Desenha uma string
    ssd1306_draw_string(&ssd, "PRED", 5, 45); // Desenha uma string
    ssd1306_draw_string(&ssd, "TEMP", 47, 24); // Desenha uma string
    ssd1306_draw_string(&ssd, "VIB", 51, 45); // Desenha uma string
    ssd1306_draw_string(&ssd, "CONS", 90, 24); // Desenha uma string
    ssd1306_draw_string(&ssd, "COMB", 90, 45); // Desenha uma string

    // Escritas variáveis

    // Horimetro
    sprintf(str_horimetro, "%d", horimetro); // Converte o inteiro em string
    ssd1306_draw_string(&ssd, str_horimetro, 5, 34); // Desenha uma string
    // Horimetro da proxima manutenção preventiva
    sprintf(str_man_prev, "%d", man_prev); // Converte o inteiro em string
    ssd1306_draw_string(&ssd, str_man_prev, 5, 54); // Desenha uma string
    // Temperatura
    ssd1306_draw_string(&ssd, "    ", 47, 34); // Desenha uma string
    sprintf(str_temperatura, "%.0f", temperatura); // Converte o float em string
    if(temperatura > 99.5){
        ssd1306_draw_string(&ssd, str_temperatura, 51, 34); // Desenha uma string
    }else{
        ssd1306_draw_string(&ssd, str_temperatura, 55, 34); // Desenha uma string
    }
    // Vibração
    ssd1306_draw_string(&ssd, "   ", 51, 54); // Desenha uma string
    sprintf(str_vibracao, "%0.f", vibracao);
    if(vibracao > 99.5){
        ssd1306_draw_string(&ssd, str_vibracao, 51, 54); // Desenha uma string
    }else if(vibracao > 9.5){
        ssd1306_draw_string(&ssd, str_vibracao, 55, 54); // Desenha uma string
    }else{
        ssd1306_draw_string(&ssd, str_vibracao, 59, 54); // Desenha uma string
    }
    // Consumo
    ssd1306_draw_string(&ssd, "    ", 90, 34); // Desenha uma string
    sprintf(str_consumo, "%.0f", consumo); // Converte float em string
    if(consumo > 99.5){
        ssd1306_draw_string(&ssd, str_consumo, 94, 34); // Desenha uma string
    }else if(consumo > 9.5){
        ssd1306_draw_string(&ssd, str_consumo, 98, 34); // Desenha uma string
    }else if(consumo > 0.5){
        ssd1306_draw_string(&ssd, str_consumo, 102, 34); // Desenha uma string
    }else{
        ssd1306_draw_string(&ssd, "0", 102, 34); // Desenha uma string
    }
    // Combustivel
    ssd1306_draw_string(&ssd, "   ", 94, 54); // Desenha uma string
    sprintf(str_combustivel, "%.0f", combustivel); // Converte o float em string
    if(combustivel > 99.5){
        ssd1306_draw_string(&ssd, str_combustivel, 94, 54); // Desenha uma string
    }else if(combustivel > 9.5){
        ssd1306_draw_string(&ssd, str_combustivel, 98, 54); // Desenha uma string
    }else{
        ssd1306_draw_string(&ssd, str_combustivel, 102, 54); // Desenha uma string
    }

    ssd1306_send_data(&ssd); // Atualiza o display
}

// Função principal
int main()
{
    // -- Inicializações
    // Monitor serial
    stdio_init_all();
    
    // GPIO
    gpio_init(button_A); // Inicia a GPIO 5 do botão A
    gpio_set_dir(button_A, GPIO_IN); // Define a direção da GPIO 5 do botão A como entrada
    gpio_pull_up(button_A); // Habilita o resistor de pull up da GPIO 5 do botão A
    
    gpio_init(button_B); // Inicia a GPIO 6 do botão B
    gpio_set_dir(button_B, GPIO_IN); // Define a direção da GPIO 6 do botão B como entrada
    gpio_pull_up(button_B); // Habilita o resistor de pull up da GPIO 6 do botão B

    gpio_init(joystick_PB); // Inicia a GPIO 22 do botão do Joystick
    gpio_set_dir(joystick_PB, GPIO_IN); // Define a direção da GPIO 22 do botão do Joystick como entrada
    gpio_pull_up(joystick_PB); // Habilita o resistor de pull up da GPIO 22 do botão do Joystick

    gpio_init(LED_Green); // Inicia a GPIO 11 do LED Verde
    gpio_set_dir(LED_Green, GPIO_OUT); // Define a direção da GPIO 11 do LED Verde como saída
    gpio_put(LED_Green, false); // Estado inicial do LED apagado

    gpio_init(LED_Red); // Inicia a GPIO 13 do LED Vermelho
    gpio_set_dir(LED_Red, GPIO_OUT); // Define a direção da GPIO 13 do LED Vermelho como saída
    gpio_put(LED_Red, true); // Estado inicial do LED aceso

    // Display I2C
    i2c_init(display_i2c_port, 400 * 1000); // Inicializa o I2C usando 400kHz
    gpio_set_function(display_i2c_sda, GPIO_FUNC_I2C); // Define o pino SDA (GPIO 14) na função I2C
    gpio_set_function(display_i2c_scl, GPIO_FUNC_I2C); // Define o pino SCL (GPIO 15) na função I2C
    gpio_pull_up(display_i2c_sda); // Ativa o resistor de pull up para o pino SDA (GPIO 14)
    gpio_pull_up(display_i2c_scl); // Ativa o resistor de pull up para o pino SCL (GPIO 15)
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, display_i2c_endereco, display_i2c_port); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd); // Atualiza o display

    // PWM
    gpio_set_function(buzzer_A, GPIO_FUNC_PWM); // Define a função da porta GPIO como PWM
    gpio_set_function(buzzer_B, GPIO_FUNC_PWM); // Define a função da porta GPIO como PWM
    pwm_freq(buzzer_A, 200); // Define a frequência do buzzer A
    pwm_freq(buzzer_B, 200); // Define a frequência do buzzer B

    // ADC
    adc_init();
    adc_gpio_init(joystick_Y); // Inicia o ADC para o GPIO 26 do VRY do Joystick
    adc_gpio_init(joystick_X); // Inicia o ADC para o GPIO 27 do VRX do Joystick
    adc_gpio_init(mic); // Inicia o ADC para o GPIO 28 do Mic

    // PIO
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    uint offset = pio_add_program(pio0, &ws2818b_program);
    ws2818b_program_init(np_pio, sm, offset, matriz_leds, 800000);
    desliga(); // Para limpar o buffer dos LEDs
    buffer();

    // Declaração de uma estrutura de temporizador de repetição.
    struct repeating_timer timer;

    // Configura o temporizador para chamar a função de callback a cada 3 segundos.
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer);

    // Interrupção dos botões
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(joystick_PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        if(motor_activate){
            atualizar_display();

            // Faz a leitura ADC dos joysticks e do mic
            adc_select_input(0); // Seleciona o ADC0 referente ao VRY do Joystick (GPIO 26)
            value_vry = adc_read(); // Ler o valor do ADC selecionado (ADC0 - VRY) e guarda
            adc_select_input(1); // Seleciona o ADC1 referente ao VRX do Joystick (GPIO 27)
            value_vrx = adc_read(); // Ler o valor do ADC selecionado (ADC1 - VRX) e guarda
            adc_select_input(2); // Seleciona o ADC1 referente ao VRX do Joystick (GPIO 28)
            value_mic = adc_read(); // Ler o valor do ADC selecionado (ADC2 - MIC) e guarda

            // Função para definir a temperatura e vibração do motor
            temp_vib_motor(value_vry, value_mic);

        }
        sleep_ms(100);
    }
}