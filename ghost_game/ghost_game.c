#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "funcoes/mudar_LED.c"
#include "pio_matrix.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define VRX_PIN 26
#define VRY_PIN 27
#define SW_PIN 22

#define btn_A 5

// Animação do 'sensor' na matriz de led
double animation[5][25] = {
    {0.0, 0.0, 0.0, 0.0, 0.0, // RESET
    0.0, 0.0, 0.0, 0.0, 0.0, 
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0},

    {0.5, 0.5, 0.5, 0.5, 0.5, // Inimigo próximo 1 de 3
    0.5, 0.0, 0.0, 0.0, 0.5, 
    0.5, 0.0, 0.0, 0.0, 0.5,
    0.5, 0.0, 0.0, 0.0, 0.5,
    0.5, 0.5, 0.5, 0.5, 0.5},
    
    {0.5, 0.5, 0.5, 0.5, 0.5, // Inimigo próximo 2 de 3
    0.5, 0.5, 0.0, 0.5, 0.5, 
    0.5, 0.0, 0.0, 0.0, 0.5,
    0.5, 0.5, 0.0, 0.5, 0.5,
    0.5, 0.5, 0.5, 0.5, 0.5},

    {0.5, 0.5, 0.5, 0.5, 0.5, // Inimigo próximo 3 de 3
    0.5, 0.5, 0.5, 0.5, 0.5, 
    0.5, 0.5, 0.5, 0.5, 0.5,
    0.5, 0.5, 0.5, 0.5, 0.5,
    0.5, 0.5, 0.5, 0.5, 0.5},

    {0.8, 0.8, 0.8, 0.8, 0.8, // Atire, o inimigo está no alcance !
    0.8, 0.8, 0.8, 0.8, 0.8, 
    0.8, 0.8, 0.8, 0.8, 0.8,
    0.8, 0.8, 0.8, 0.8, 0.8,
    0.8, 0.8, 0.8, 0.8, 0.8},
};

// Variáveis globais
int tree[7] = {50, 180, 200, 250, 300, 320, 400};
int fantasm[5] = {120, 205, 280, 380, 460};
int position = 10;
int relative_position = 0;
int fantasmPoints = 0;

// Variáveis globais voláteis, que serão utilizadas em interrupções
static volatile uint32_t last_time = 0;
static volatile uint32_t fantasm_attack_delay;
volatile int gameOn = 0;
volatile int fantasm_attacked = 0; 
volatile int playerAlive = 1;
volatile int attack_animation = 0;

// Funções
void moveRight();
void moveLeft();

// Funções de interrupção de botão e de hardware com temporizador
static void gpio_irq_handler(uint gpio, uint32_t events);
int64_t alarm_callback(alarm_id_t id, void *user_data);

int main()
{
    // Variáveis para matriz de leds
    PIO pio = pio0; 
    uint16_t i;
    bool ok;
    uint32_t valor_led;
    double r,b,g;

    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    ok = set_sys_clock_khz(128000, false);

    // Para utilizar printf
    stdio_init_all();

    // Display 128x64
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_t ssd; // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;

    //Configurações da PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    // Iniciando o ADC e o botão do joystick
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);

    gpio_init(btn_A);
    gpio_set_dir(btn_A, GPIO_IN);
    gpio_pull_up(btn_A);

    // Adicionado interrupções no botão A e no botão do joystick
    gpio_set_irq_enabled_with_callback(btn_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    printf("\n\n\n\n\n\n\n\n\n\n\n");
    while (true) {
        if (gameOn) {
            // Rotina para caso o jogador perca
            if (playerAlive == 0) {
                printf("Voce perdeu\n");
                ssd1306_fill(&ssd, !cor);
                ssd1306_draw_string(&ssd, "Voce Perdeu", 20, 30);
                ssd1306_send_data(&ssd); // Atualiza o display
                position = 10;
                relative_position = 0;
                gameOn = 0;
                fantasmPoints = 0;
                attack_animation = 0;
                fantasm[0] = 120;
                fantasm[1] = 205;
                fantasm[2] = 280;
                fantasm[3] = 380;
                fantasm[4] = 460;
                sleep_ms(2000);
            }

            // Rotina para caso o jogador ganhe
            if (fantasmPoints == 5) {
                printf("Voce ganhou\n");
                ssd1306_fill(&ssd, !cor);
                ssd1306_draw_string(&ssd, "Voce Venceu", 20, 30);
                ssd1306_send_data(&ssd); // Atualiza o display
                position = 10;
                relative_position = 0;
                gameOn = 0;
                sleep_ms(2000);
            }

            ssd1306_fill(&ssd, !cor); // Limpa o display

        if (attack_animation > 0) {
            attack_animation--;
        }

        ssd1306_rect(&ssd, 45 - attack_animation, position - attack_animation / 2, 8 + attack_animation, 8 + attack_animation, cor, cor); // Desenha um retângulo
        ssd1306_rect(&ssd, 53, 0, 128, 11, cor, cor);

        for (int i = 0; i < 7; i++) { // Carregar cenário
            if (tree[i] - relative_position > 0 && tree[i] - relative_position < 128) {
                int control = 0;
                if (tree[i] - relative_position - 8 < 0) {
                    control = -1 * (tree[i] - relative_position - 8);
                }

                ssd1306_rect(&ssd, 28, tree[i] - relative_position, 8, 25, cor, cor);
                ssd1306_rect(&ssd, 32, tree[i] - relative_position - 8  + control, 24, 8, cor, cor);
            }
        }

        ssd1306_draw_char(&ssd, (char)fantasmPoints + 48, 105, 5);
        ssd1306_draw_char(&ssd, '5', 115, 5);
        ssd1306_rect(&ssd, 3, 113, 1, 10, cor, cor);

        ssd1306_send_data(&ssd); // Atualiza o display

        adc_select_input(0); // Para o pino 26 do VRX
        uint16_t vrx_value = adc_read();

        adc_select_input(1); // Para o pino 26 do VRX
        uint16_t vry_value = adc_read();

        if (vrx_value > 2500) {
            moveRight();
        } else if (vrx_value < 1500) {
            moveLeft();
        }

        int hasFantasm = 0; // Caso necessário limpar matriz
        for (int i = 0; i < 5; i++) { // Carregar cenário
            if (fantasm[i] == -1) {
                continue;
            }

            // Verificar a posição do fantasma e carregar a cor correspondente da distância na matriz de leds
            if (fantasm[i] - relative_position - position >= 0 && fantasm[i] - relative_position - position <= 15 && !fantasm_attacked) {
                hasFantasm = 1;
                if (fantasm[i] - relative_position - position == 0) {
                desenho_pio(animation[4], valor_led, pio, sm, 0, 1, 0);
                fantasm[i] = -1;
                fantasm_attacked = 1;
                fantasm_attack_delay = to_us_since_boot(get_absolute_time());
                break;
            } else if (fantasm[i] - relative_position - position <= 5) {
                desenho_pio(animation[3], valor_led, pio, sm, 1, 0, 0);
                break;
            } else if (fantasm[i] - relative_position - position <= 10) {
                desenho_pio(animation[2], valor_led, pio, sm, 1, 0, 0);
                break;
            } else if (fantasm[i] - relative_position - position <= 15) {
                desenho_pio(animation[1], valor_led, pio, sm, 1, 0, 0);
                break;
                
            }
            }
        }

        // Rotina para limpar a matriz se não tiver fantasmas por perto
        if (!hasFantasm) {
            desenho_pio(animation[0], valor_led, pio, sm, 0, 1, 0);
        }

        // Verificar se passou 1 segundo desde que a matriz de leds ficou verde
        uint32_t current_time = to_us_since_boot(get_absolute_time());
        if (current_time - fantasm_attack_delay >= 1000000 && fantasm_attacked) {
            fantasm_attacked = 0;
            playerAlive = 0;
        }
        } else {
            if (fantasmPoints == 5) {
                position = 10;
                relative_position = 0;
                gameOn = 0;
                fantasmPoints = 0;
                attack_animation = 0;
                fantasm[0] = 120;
                fantasm[1] = 205;
                fantasm[2] = 280;
                fantasm[3] = 380;
                fantasm[4] = 460;
            }

            ssd1306_fill(&ssd, !cor); // Limpa o display
            if (position != 10) {
                printf("Jogo Pausado, pressione A para continuar\n");
                ssd1306_draw_string(&ssd, "Pausado", 35, 30);
            } else {
                printf("Pressione A para continuar\n");
                ssd1306_draw_string(&ssd, "A para iniciar", 10, 30);
            }
            ssd1306_send_data(&ssd); // Atualiza o display
        }

        sleep_ms(100);
    }
}

void moveRight() {
    if ((position + 8) + 5 > 128) {
        position = 128 - 8 - 5;

        if (relative_position <= 380) {
            relative_position += 5;
        }
    }

    position += 5;
};

void moveLeft() {
    if (position - 5 < 5) {
        position = 5;

        if (relative_position >= 0) {
            relative_position -= 5;
        }
    }
    
    position -= 5;
};

static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time >= 200000) {
        last_time = current_time;

        if (gpio == 5) {
            if (playerAlive == 0) {
                playerAlive = 1;
            }

            gameOn = gameOn == 0 ? 1: 0;

        } else {
            // Utiliza o botão do joystick para eliminar o fantasma
            if (current_time - fantasm_attack_delay >= 1000000) {
                fantasm_attacked = 0;
                playerAlive = 0;
                return;
            }

            if (fantasm_attacked == 1) {
                add_alarm_in_ms(200, alarm_callback, NULL, true);
                printf("Fantasma derrotado, adicionado 1 ponto");
                fantasmPoints++;
                attack_animation = 8;
            } else {
                playerAlive = 0;
            }
        }
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    fantasm_attacked = 0;
    return 0;
}