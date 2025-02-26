<div align="center">
    <img src="https://moodle.embarcatech.cepedi.org.br/pluginfile.php/1/theme_moove/logo/1733422525/Group%20658.png" alt="Logo Embarcatech" height="100">
</div>

<br>

# DAMP - Dispositivo Auxiliar de Manutenção Preventiva/Preditiva

## Sumário

- [Descrição](#descrição)
- [Funcionalidades Implementadas](#funcionalidades-implementadas)
- [Ferramentas utilizadas](#ferramentas-utilizadas)
- [Objetivos](#objetivos)
- [Instruções de uso](#instruções-de-uso)
- [Vídeo de apresentação](#vídeo-de-apresentação)
- [Aluno e desenvolvedor do projeto](#aluno-e-desenvolvedor-do-projeto)
- [Licensa](#licença)

## Descrição

Este projeto tem como objetivo principal: consolidar os conceitos estudados sobre desenvolvimento e programação de sistemas embarcados, estudados durante a fase 1 da residência tecnológica em sistemas embarcados, EmbarcaTech.

Este projeto implementa um sistema embarcado para auxiliar na manutenção preventiva e preditiva de máquinas pesadas, utilizando a placa de desenvolvimento BitDogLab. O sistema monitora variáveis essenciais, como tempo de operação (horímetro), temperatura do motor, vibração e consumo de combustível, simuladas por componentes integrados da BitDogLab.

A medição do horímetro permite calcular o tempo restante para a próxima manutenção preventiva e exibir alertas à medida que a manutenção se aproxima ou está vencida. Já os parâmetros de temperatura, vibração e consumo de combustível são analisados para detectar padrões que possam indicar falhas futuras, possibilitando a manutenção preditiva.

Os alertas são exibidos em tempo real no display OLED e na matriz de LEDs 5x5, utilizando diferentes símbolos e cores para cada tipo de alerta. Além disso, sinais sonoros são emitidos via buzzer para reforçar as notificações. O sistema também conta com um mecanismo de liga/desliga para suspender medições e alertas quando a máquina estiver desligada.

O desenvolvimento do projeto envolveu a implementação de interrupções e debouncing para os botões de controle, temporizadores e alarmes para os alertas, e a utilização de PWM para controle da frequência do buzzer. Todo o código foi estruturado para garantir eficiência e confiabilidade no funcionamento do sistema.

## Funcionalidades Implementadas

1. Manutenção Preventiva:

   - Mede o tempo de operação da máquina utilizando um temporizador.
   - Calcula o tempo restante até a próxima manutenção preventiva.
   - Exibe um alerta a cada 10 horas quando faltarem 100 horas para a manutenção.
   - Exibe um alerta crítico a cada 10 horas após a manutenção estar vencida.
   - O botão B é utilizado para confirmar a realização da manutenção:
     -  Um clique inicia a confirmação.
     -  Um segundo clique dentro de 2 segundos confirma a manutenção e adiciona 500 horas ao horímetro da próxima manutenção.

2. Monitoramento da Temperatura do Motor:

   - Mede a temperatura do motor (simulada pelo eixo Y do joystick).
   - Exibe um alerta se a temperatura estiver entre 92°C e 95°C.
   - Exibe um alerta crítico se a temperatura estiver entre 95°C e 100°C.
   - Exibe um alerta crítico e desliga a máquina se a temperatura ultrapassar 100°C.

3. Monitoramento do Consumo de Combustível:

   - Mede o nível de combustível no tanque (simulado pelo eixo X do joystick).
   - Calcula o consumo de combustível da máquina em litros por hora (l/h).
   - Exibe um alerta se o consumo ultrapassar 25 l/h.
    
4. Monitoramento da Vibração do Motor:

   - Mede o nível de vibração do motor (simulado pelo microfone).
   - Exibe um alerta se a vibração ultrapassar 75.
  
5. Exibição de Informações no Display:

   - O display OLED exibe em tempo real:
     - Horímetro da máquina.
     - Horímetro da próxima manutenção preventiva.
     - Temperatura do motor.
     - Nível de vibração do motor.
     - Consumo de combustível.
     - Nível de combustível.
     - Alertas ativos.

6. Sistema de Alertas:

   - Os alertas são exibidos na matriz de LEDs 5x5 e possuem diferentes símbolos e cores:
     - Manutenção próxima: Exibe "P" em azul.
     - Manutenção vencida: Exibe "P!" em lilás.
     - Alerta preventivo: Exibe "!" em amarelo.
     - Alerta crítico: Exibe "θ" em vermelho.
   - Além do display e matriz de LEDs, os buzzers emitem sinais sonoros:
     - 1000 Hz com a variação de tempo de duração do sinal sonoro para cada tipo de alerta.

7. Sistema de Liga/Desliga:

   - O botão A é responsável por ligar e desligar a máquina (LED Verde aceso para máquina ligada, LED Vermelho aceso para máquina desligada).
   - Quando desligada, nenhuma medição é realizada, e os alertas são suspensos.

8. Outros:

   - Os botões foram implementados com interrupções e debouncing.
   - Foi utilizado temporizadores e alarmes para implementar alertas.
   - Foi utilizado PWM para configurar a frequência dos buzzers.

## Ferramentas utilizadas

- **Simulador de eletrônica wokwi**: Ambiente utilizado para simular o hardware e validar o funcionamento do sistema.
- **Ferramenta educacional BitDogLab (versão 6.3)**: Placa de desenvolvimento utilizada para programar o microcontrolador.
- **Microcontrolador Raspberry Pi Pico W**: Gerencia a lógica do sistema, lê sensores e controla os atuadores.
- **Joystick**: Simula temperatura (eixo Y) e nível de combustível (eixo X).
- **Microfone**: Simula o sensor de vibração do motor.
- **Buzzer**: Emite alertas sonoros de diferentes durações.
- **Matriz de LEDs 5x5**: Exibe alertas visuais de manutenção e problemas no motor.
- **LED RGB**: Indica o funcionamento da máquina.
- **Display OLED SSD1306**: Apresenta os dados em tempo real (horímetro, temperatura, vibração, etc.).
- **Botão A**: Liga e desliga a máquina.
- **Botão B**: Confirma a manutenção preventiva realizada.
- **Visual Studio Code (VS Code)**: IDE utilizada para o desenvolvimento do código com integração ao Pico SDK.
- **Pico SDK**: Kit de desenvolvimento de software utilizado para programar o Raspberry Pi Pico W em linguagem C.
- **Monitor serial do VS Code**: Ferramenta utilizada para realizar testes e depuração.

## Objetivos

1. Consolidar os conceitos estudados sobre desenvolvimento e programação de sistemas embarcados, estudados durante a fase 1 da residência tecnológica em sistemas embarcados, EmbarcaTech.
2. Fazer a medição do horímetro da máquina, comparar com o horímetro da próxima manutenção preventiva e emitir alerta caso necessário.
3. Atualizar o horímetro da próxima manutenção preventiva.
4. Fazer a medição da temperatura e vibração do motor e do consumo de combustível da máquina para manutenção preditiva.
5. Exibir alertas para cada medição fora do padrão.

## Instruções de uso

1. **Clonar o Repositório**:

```bash
git clone https://github.com/bigodinhojf/embarcatech_projeto_final.git
```

2. **Compilar e Carregar o Código**:
   No VS Code, configure o ambiente e compile o projeto com os comandos:

```bash	
cmake -G Ninja ..
ninja
```

3. **Interação com o Sistema**:
   - Conecte a placa ao computador.
   - Clique em run usando a extensão do raspberry pi pico.
   - Clique no botão A para ligar a máquina.
   - Aguarde um tempo para observar os alertas de manutenção preventiva.
   - Clique no botão B duas vezes para confirmar uma manutenção e adicionar 500 horas para próxima preventiva.
   - Mova o Joystick para cima e para baixo para observar a simulção do sensor de temperatura, observe os alertas.
   - Mova o Joystick para direira e para esquerda para observar a simulação do sensor de nível de combustível e alteração do consumo, observe os alertas.
   - Emita sons variados perto do microfone para observar a simulação do sensor de vibração, observe os alertas.

## Vídeo de apresentação

O vídeo apresentando o projeto pode ser assistido [clicando aqui](https://youtu.be/mVY0w1Lo0Og).

## Aluno e desenvolvedor do projeto

<a href="https://github.com/bigodinhojf">
        <img src="https://github.com/bigodinhojf.png" width="150px;" alt="João Felipe"/><br>
        <sub>
          <b>João Felipe</b>
        </sub>
</a>

## Licença

Este projeto está licenciado sob a licença MIT.
