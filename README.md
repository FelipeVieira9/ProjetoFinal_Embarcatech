# Projeto Final do EmbarcaTech
Foi desenvolvida uma aplicação que utiliza os periféricos da BitDogLab para interagir com o display oled em um jogo

## Periféricos utilizados
- Matriz de LEDs 5x5 RGB endereçável
- Display OLED 128 x 64 I2C
- Joystick analógico com botão

## Contexto e como jogar
Nesse jogo, você é um caça fantasmas que foi contratado para exterminar fantasmas em um cemitério. Você possui uma arma que mostra a proximidade do fantasma na matriz de led, indo de parcialmente vermelho para totalmente vermelho e enfim para verde.

Para jogar esse jogo, temos os seguintes pontos:

1. Você não enxerga o fantasma, somente o sensor na matriz de leds mostra a proximidade dele.

2. São 5 fantasmas que você precisa derrotar.

3. Para atacar o fantasma a matriz de led deve passar de vermelho para verde.

4. Atacar com a matriz vermelha ou sem cor você perde.

5. Você se movimenta com o Joystick

6. O botão A inicia ou pausa o jogo

## Para rodar a aplicação
Utilizando o ambiente do Vs Code, é necessário a instação da extensão Raspberry Pi Pico e em seguida, após o git clone dessse repositório, a importação dos arquivos como um projeto Pico W dessa extensão e por fim clicar em compilar.
