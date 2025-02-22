// -- Inclusão de bibliotecas
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

// -- Definição de constantes
// GPIO
#define button_A 5 // Botão A GPIO 5
#define button_B 6 // Botão B GPIO 6
#define buzzer_A 21 // Buzzer A GPIO 21
#define buzzer_B 10 // Buzzer B GPIO 10
#define LED_Green 11 // LED Verde GPIO 11
#define LED_Red 13 // LED Vermelho GPIO 13

// Display I2C
#define display_i2c_port i2c1 // Define a porta I2C
#define display_i2c_sda 14 // Define o pino SDA na GPIO 14
#define display_i2c_scl 15 // Define o pino SCL na GPIO 15
#define display_i2c_endereco 0x3C // Define o endereço do I2C
ssd1306_t ssd; // Inicializa a estrutura do display

// -- Variáveis globais
volatile bool motor_activate = false; // Define se o motor está ou não ligado
static volatile uint32_t last_time = 0; // Armazena o tempo do último clique dos botões
volatile bool confirm_B = false; // Armazena se o botão B foi clicado nos ultimos 2s
volatile int segundos = 0; // Guarda o valor dos segundos desde a partida
volatile int horimetro = 1485; // Guarda o valor do horimetro
char str_horimetro[4]; // Guarda o valor do horímetro em string
volatile int man_prev = 1500; // Guarda o valor do horímetro da próxima manutenção preventiva
char str_man_prev[4]; // Guarda o valor da próxima manutenção preventiva em string

char str_alerta[15]; // Guarda o valor da mensagem de alerta

// Função de callback do alarme
int64_t alarm_callback(alarm_id_t id, void *user_data){
    ssd1306_draw_string(&ssd, "               ", 3, 13);
    confirm_B = false;
    return 0;
}

// Função de alerta
void alerta(int tipo, int n){
    if(!confirm_B){
        switch (tipo){
        case 1:
            // Tipo de alerta para manutenção preventiva perto
            ssd1306_draw_string(&ssd, "Prev proxima", 15, 13);
            break;
        case 2:
            // Tipo de alerta para manutenção preventiva vencida
            ssd1306_draw_string(&ssd, "Prev vencida", 15, 13);
        default:
            break;
        }
        add_alarm_in_ms(5000, alarm_callback, NULL, false);
    }
}

// Função de callback do temporizador
bool repeating_timer_callback(struct repeating_timer *t) {
    if(motor_activate){
        segundos += 1800;
        // Adiciona 1 hora ao horimetro a cada 3600 segundos (1 hora)
        if (segundos % 3600 == 0){
            horimetro++;
            if(horimetro % 10 == 0 && man_prev - horimetro <= 100){
                if(man_prev - horimetro <= 0){
                    alerta(2, 2);
                }else{
                    alerta(1, 1);
                }
            }
        }
    }

    printf("Segundos: %d Horimetro: %d\n", segundos, horimetro);

    // Retorna true para manter o temporizador repetindo
    return true;
}

//Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botao 22

// Função de interrupção dos botões
void gpio_irq_handler(uint gpio, uint32_t events){
    //Debouncing
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Pega o tempo atual e transforma em us
    if(current_time - last_time > 1000000){
        if(gpio == button_A){
            motor_activate = !motor_activate;
            gpio_put(LED_Green, motor_activate);
            gpio_put(LED_Red, !motor_activate);
        }else if(gpio == button_B){
            if(confirm_B){
                man_prev += 500;
            }else{
                confirm_B = true;
                ssd1306_draw_string(&ssd, "Confirma?", 27, 13);
                add_alarm_in_ms(2000, alarm_callback, NULL, false);
            }
        }else if(gpio == botao){
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

    sprintf(str_horimetro, "%d", horimetro); // Converte o inteiro em string
    ssd1306_draw_string(&ssd, str_horimetro, 5, 34); // Desenha uma string
    sprintf(str_man_prev, "%d", man_prev); // Converte o inteiro em string
    ssd1306_draw_string(&ssd, str_man_prev, 5, 54); // Desenha uma string

    ssd1306_draw_string(&ssd, "090C", 47, 34); // Desenha uma string
    ssd1306_draw_string(&ssd, "100", 51, 54); // Desenha uma string
    ssd1306_draw_string(&ssd, "20LM", 90, 34); // Desenha uma string
    ssd1306_draw_string(&ssd, "070", 94, 54); // Desenha uma string

    ssd1306_send_data(&ssd); // Atualiza o display
}

// Função principal
int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botao);
    gpio_set_dir(botao, GPIO_IN);
    gpio_pull_up(botao);
    gpio_set_irq_enabled_with_callback(botao, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

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

    // Declaração de uma estrutura de temporizador de repetição.
    struct repeating_timer timer;

    // Configura o temporizador para chamar a função de callback a cada 3 segundos.
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer);

    // Interrupção dos botões
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        if(motor_activate){
            atualizar_display();
        }
        sleep_ms(100);
    }
}
