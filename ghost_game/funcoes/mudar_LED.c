#define NUM_PIXELS 25//número de LEDs
#define OUT_PIN 7//pino da matriz de LEDs

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        valor_led = matrix_rgb(desenho[24-i]*b, desenho[24-i]*r, desenho[24-i]*g);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}