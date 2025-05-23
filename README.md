# 💡 Sistema de Monitoramento de Ambiente - Projeto Infinity

Este projeto foi desenvolvido com o objetivo de monitorar de forma eficiente três variáveis ambientais críticas: temperatura, umidade e luminosidade. Utilizando sensores integrados a uma plataforma Arduino, o sistema fornece alertas visuais (LEDs), sonoros (buzzer) e registra automaticamente os eventos críticos na memória EEPROM para posterior auditoria. Ideal para aplicações como vinherias, laboratórios, estufas ou adegas que demandam controle ambiental preciso.

---

## 📦 Funcionalidades Principais

- Monitoramento em tempo real de temperatura, umidade e luz ambiente.
- Exibição no display LCD I2C 16x2 com interface intuitiva.
- Sistema de alerta visual (LEDs) e sonoro (buzzer) para indicar o estado atual.
- Registro de dados críticos com data e hora na EEPROM.
- **Modo Auditoria:** navegue pelos registros históricos de eventos críticos salvos na EEPROM.

---

## ✅ Dependências

Antes de compilar e carregar o projeto, certifique-se de instalar as seguintes bibliotecas na IDE do Arduino:

- `Wire.h`
- `LiquidCrystal_I2C.h`
- `DHT.h`
- `RTClib.h`
- `EEPROM.h`

Todas estão disponíveis via Gerenciador de Bibliotecas da IDE do Arduino.

---

## 🧰 Componentes Utilizados

- Arduino Uno (ou similar)
- Sensor DHT11 (temperatura e umidade)
- Sensor LDR (luminosidade)
- RTC DS1307 (relógio de tempo real)
- Display LCD I2C 16x2
- LEDs (verde, amarelo e vermelho)
- Buzzer
- Botão
- Resistores, jumpers e protoboard

---

## 💻 Montagem e Execução

### Passo a Passo:

1. Instale a [IDE do Arduino](https://www.arduino.cc/en/software).
2. Vá em **Sketch > Include Library > Manage Libraries...** e instale as bibliotecas listadas acima.
3. Monte o circuito conforme os pinos especificados:

```
LDR               → A0  
Botão             → D2  
LED Verde         → D13  
LED Amarelo       → D12  
LED Vermelho      → D11  
Buzzer            → D7  
DHT11             → D4  
LCD I2C           → SDA/SCL (A4/A5 em Arduino Uno)  
RTC DS1307        → SDA/SCL  
```

4. Abra o arquivo `.ino` na IDE do Arduino.
5. Carregue o código na placa.
6. Execute o sistema.

---

## 🧑‍💻 Execução do Sistema

Durante a inicialização, o display LCD mostra uma sequência de boas-vindas com o nome **"INFINITY"** e o símbolo personalizado de infinito.

Após a inicialização, o sistema começa o monitoramento contínuo, realizando uma leitura a cada 6 segundos.

### Estados e Ações:

| Estado  | Ação no Sistema                      |
|---------|--------------------------------------|
| Ideal   | LED Verde aceso, sem alarme         |
| Alerta  | LED Amarelo aceso                   |
| Crítico | LED Vermelho + Buzzer acionado      |

- Sempre que um valor entrar em estado **crítico**, um log será salvo na EEPROM com **data e hora**.

---

## 🧠 Diferencial: Modo Auditoria

Ao segurar o botão por mais de **3 segundos**, o sistema entra no **modo auditoria**, exibindo os logs críticos salvos na EEPROM:

- Cada **clique curto** no botão avança para o próximo registro.
- O LCD exibe:
  - Primeira linha: `dd/mm hh:mm`
  - Segunda linha: `Lxx% Uxx% Txx%`
- Ao chegar no último log, o sistema volta ao primeiro.
- Para retornar ao modo de monitoramento, **segure o botão por mais 3 segundos**.

---

## 👥 Membros do Grupo

- **Diogo Pelinson** – RM563321  
- **Jessica Tavares** – RM566220  
- **Luara Soares** – RM561266  
- **Miguel Amaro** – RM566200  
- **Pedro Henrique Caires** – RM562344  

---

## 📂 Sobre

Sistema completo de monitoramento ambiental em Arduino com recursos de registro histórico e interface visual com LCD. Ideal para projetos escolares, acadêmicos ou aplicações reais de controle de ambiente.
