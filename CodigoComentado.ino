// === BIBLIOTECAS ===
// Importa bibliotecas essenciais para sensores, display, relógio e memória
#include <Wire.h>                // Comunicação I2C usada para LCD e RTC
#include <LiquidCrystal_I2C.h>   // Controle do display LCD via protocolo I2C
#include <DHT.h>                 // Sensor de temperatura e umidade DHT11
#include <RTClib.h>              // RTC DS1307 para manter data e hora atualizadas
#include <EEPROM.h>              // Memória EEPROM para armazenar dados persistentes

// === DEFINIÇÕES ===
// Define os pinos e tipos de dispositivos usados
#define DHTPIN 4                 // Pino digital para sensor DHT11
#define DHTTYPE DHT11            // Tipo do sensor DHT
#define BOTAO 2                  // Pino digital do botão para controle

DHT dht(DHTPIN, DHTTYPE);        // Cria objeto para o sensor DHT
RTC_DS1307 rtc;                  // Cria objeto para o relógio RTC
LiquidCrystal_I2C lcd(0x27, 16, 2); // Cria objeto para o LCD I2C (endereço 0x27, 16 colunas, 2 linhas)

// === PINOS ===
// Definição dos pinos dos sensores e atuadores
const int ldrPin = A0;           // Pino analógico para sensor LDR (luz)
const int ledVerde = 13;         // LED verde para indicar estado OK
const int ledAmarelo = 12;       // LED amarelo para alerta
const int ledVermelho = 11;      // LED vermelho para estado crítico
const int buzzer = 7;            // Buzzer para alertas sonoros

// === VARIÁVEIS DE TEMPO ===
// Controle de temporização para leituras e debounce de botão
unsigned long ultimoMillis = 0;          // Armazena o último tempo de leitura
unsigned long intervaloLeitura = 6000;   // Intervalo entre leituras (6 segundos)

unsigned long tempoBotaoPressionado = 0; // Guarda tempo de início do pressionamento do botão
bool botaoAnterior = HIGH;                // Estado anterior do botão (para detectar bordas)
bool modoAuditoria = false;               // Flag que indica se está no modo auditoria
int indiceLogAtual = 0;                   // Índice do log exibido no modo auditoria
bool longPressHandled = false;            // Controle para detectar pressionamento longo do botão

// EEPROM
int enderecoEEPROM = 0;                   // Endereço da próxima gravação na EEPROM
const int TAMANHO_REGISTRO = 10;          // Tamanho fixo dos registros na EEPROM (bytes)
int numLogsEscritos = 0;                  // Contador dos logs salvos na EEPROM

// FLAGS para armazenar últimos valores críticos para evitar gravações repetidas
float ultimoTempCrit = NAN;               // Última temperatura crítica registrada
float ultimaUmidCrit = NAN;                // Última umidade crítica registrada
int ultimaLuzCrit = -1;                    // Última luz crítica registrada

// Definição dos caracteres personalizados para o símbolo do infinito no LCD
byte name1x5[] = { B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000 };
byte name0x6[] = { B00000, B00111, B01000, B10000, B10000, B10000, B01000, B00111 };
byte name0x7[] = { B00000, B00000, B10001, B01010, B00100, B01010, B10001, B00000 };
byte name0x8[] = { B00000, B11100, B00010, B00001, B00001, B00001, B00010, B11100 };

// Função para animar o logo do infinito no LCD, movendo os caracteres da direita para esquerda
void animaLogo() {
  for (int pos = 16; pos >= 0; pos--) {
    lcd.clear();
    lcd.setCursor(pos, 1); lcd.write(byte(0));   // parte inferior esquerda do símbolo
    lcd.setCursor(pos + 1, 0); lcd.write(byte(1)); // topo esquerdo
    lcd.setCursor(pos + 2, 0); lcd.write(byte(2)); // topo meio
    lcd.setCursor(pos + 3, 0); lcd.write(byte(3)); // topo direito
    delay(200);
  }
}

// === SETUP ===
// Configuração inicial do sistema, sensores, LCD, pinos e mensagens de boas-vindas
void setup() {
  Serial.begin(9600);           // Inicializa comunicação serial para debug
  dht.begin();                  // Inicializa sensor DHT
  rtc.begin();                  // Inicializa relógio RTC

  // Define pinos dos LEDs e buzzer como saída
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(buzzer, OUTPUT);

  pinMode(BOTAO, INPUT_PULLUP);  // Botão configurado com resistor interno de pull-up

  lcd.init();                   // Inicializa LCD
  lcd.backlight();             // Liga luz de fundo do LCD

  // Mensagens iniciais na tela para boas-vindas
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("BEM-VINDO A");
  delay(1500);

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("INFINITY");
  delay(1500);

  // Cria caracteres personalizados para desenhar símbolo do infinito (∞)
  lcd.createChar(0, name1x5);
  lcd.createChar(1, name0x6);
  lcd.createChar(2, name0x7);
  lcd.createChar(3, name0x8);

  // Desenha o símbolo do infinito no LCD
  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.write(byte(0));
  lcd.setCursor(6, 0);
  lcd.write(byte(1));
  lcd.setCursor(7, 0);
  lcd.write(byte(2));
  lcd.setCursor(8, 0);
  lcd.write(byte(3));
  delay(2000);

  // Animação do logo
  animaLogo();

  // Sequência de mensagens finais da inicialização
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("MONITORAMENTO");
  delay(1500);

  lcd.setCursor(1, 0);
  lcd.print("SISTEMA ATIVO");
  delay(2000);
  lcd.clear();

  // (Comentado) Código para verificar logs existentes na EEPROM e contar quantos já foram escritos
  // Serve para retomar do último ponto gravado ao reiniciar
  /*
  Serial.println("DEBUG: Verificando EEPROM no setup...");
  for (int i = 0; i < EEPROM.length(); i += TAMANHO_REGISTRO) {
    byte marker = EEPROM.read(i + 9);
    Serial.print("DEBUG: Endereco "); Serial.print(i);
    Serial.print(", Byte 9 (marcador): 0x"); Serial.print(marker, HEX);
    if (marker == 0x01) {
      numLogsEscritos++;
      enderecoEEPROM = i + TAMANHO_REGISTRO;
      Serial.println(" (Valido)");
    } else {
      Serial.println(" (Invalido/Vazio). Parando contagem.");
      break;
    }
  }
  Serial.print("Logs encontrados na EEPROM: ");
  Serial.println(numLogsEscritos);
  */
}

// === LOOP PRINCIPAL ===
// Loop que roda continuamente: lê sensores, verifica botão, salva logs, exibe no LCD e controla LEDs/buzzer
void loop() {
  bool estadoBotao = digitalRead(BOTAO); // Lê estado atual do botão
  unsigned long agora = millis();         // Tempo atual em milissegundos

  // === DETECÇÃO DE BOTÃO ===
  // Detecta quando o botão começa a ser pressionado (bordas)
  if (estadoBotao == LOW && botaoAnterior == HIGH) {
    tempoBotaoPressionado = agora; // Marca o tempo do início do pressionamento
  }

  // Detecta pressionamento longo (>3s) para alternar o modo auditoria
  if (estadoBotao == LOW && (agora - tempoBotaoPressionado > 3000)) {
    if (!longPressHandled) {          // Garante que a alternância ocorra só uma vez por pressionamento longo
      modoAuditoria = !modoAuditoria; // Alterna modo auditoria on/off
      lcd.clear();
      if (modoAuditoria) {
        lcd.setCursor(0,0); lcd.print("MODO AUDITORIA");
        indiceLogAtual = 0;           // Reinicia índice para navegação nos logs
      } else {
        lcd.setCursor(1,0); lcd.print("SISTEMA ATIVO");
        // Reseta os últimos valores críticos ao voltar para modo monitoramento
        ultimoTempCrit = NAN;
        ultimaUmidCrit = NAN;
        ultimaLuzCrit = -1;
      }
      delay(1000);                    // Mostra mensagem por 1 segundo e evita bouncing
      lcd.clear();
      longPressHandled = true;        // Marca que já tratou esse long press
    }
  } else if (estadoBotao == HIGH && botaoAnterior == LOW) {
    // Quando botão é liberado, reseta flag para detectar novo pressionamento longo depois
    longPressHandled = false;
  }

  // Se estiver no modo auditoria, executa loop específico para mostrar logs, e não faz leituras normais
  if (modoAuditoria) {
    modoAuditoriaLoop(estadoBotao);
    botaoAnterior = estadoBotao; // Atualiza estado do botão para próxima verificação
    return;                     // Sai para evitar leituras e atualizações normais no modo auditoria
  }

  // === LEITURA PERIÓDICA DOS SENSORES ===
  if (agora - ultimoMillis >= intervaloLeitura) {
    ultimoMillis = agora;                // Atualiza último tempo de leitura

    float temperatura = dht.readTemperature(); // Lê temperatura do DHT
    float umidade = dht.readHumidity();         // Lê umidade do DHT
    int leituraLDR = analogRead(ldrPin);        // Lê valor analógico do LDR

    // Mapeia leitura do LDR para percentual de luz
    // Ajusta para inversão do sensor e range esperado no ambiente
    int luzPercent = constrain(map(leituraLDR, 1002, 250, 0, 100), 0, 100);

    // Verifica se leituras do DHT são válidas
    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Erro ao ler DHT!");
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("ERRO SENSOR DHT!");
      setLeds(3);                    // Acende LED vermelho e buzzer para erro
      return;                       // Pula essa leitura para evitar usar dados inválidos
    }

    // Define faixas de temperatura: ideal, alerta e crítico
    bool tempIdeal = (temperatura >= 10 && temperatura <= 16);
    bool tempAlerta = ((temperatura >= 7 && temperatura < 10) || (temperatura > 16 && temperatura <= 19));
    bool tempCrit = !(tempIdeal || tempAlerta);

    // Faixas para umidade (exemplo, pode ajustar conforme aplicação)
    bool umidIdeal = (umidade >= 70 && umidade <= 85);
    bool umidAlerta = ((umidade >= 60 && umidade < 70) || (umidade > 85 && umidade <= 90));
    bool umidCrit = !(umidIdeal || umidAlerta);

    // Faixas para luz (só exemplo, ajustar conforme uso)
    bool luzIdeal = (luzPercent >= 30 && luzPercent <= 70);
    bool luzAlerta = ((luzPercent >= 20 && luzPercent < 30) || (luzPercent > 70 && luzPercent <= 80));
    bool luzCrit = !(luzIdeal || luzAlerta);

    // Decide cor do LED e acionamento do buzzer conforme pior estado entre temperatura, umidade e luz
    if (tempCrit || umidCrit || luzCrit) {
      setLeds(3);        // Vermelho + buzzer
    } else if (tempAlerta || umidAlerta || luzAlerta) {
      setLeds(2);        // Amarelo (alerta), sem buzzer
    } else {
      setLeds(1);        // Verde (normal), sem buzzer
    }

    // Exibe dados no LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(temperatura,1);
    lcd.print("C ");

    lcd.print("U:");
    lcd.print(umidade,0);
    lcd.print("%");

    lcd.setCursor(0,1);
    lcd.print("Luz:");
    lcd.print(luzPercent);
    lcd.print("% ");

    // === GRAVAÇÃO DE LOGS NA EEPROM ===
    // Só grava se algum parâmetro estiver em estado crítico (para economizar espaço e gravação)
    if ((tempCrit && !isEqualFloat(temperatura, ultimoTempCrit)) ||
        (umidCrit && !isEqualFloat(umidade, ultimaUmidCrit)) ||
        (luzCrit && luzPercent != ultimaLuzCrit)) {

      // Atualiza últimos valores críticos para comparação futura
      if (tempCrit) ultimoTempCrit = temperatura;
      if (umidCrit) ultimaUmidCrit = umidade;
      if (luzCrit) ultimaLuzCrit = luzPercent;

      // Salva o log na EEPROM com timestamp e dados
      salvarLogEEPROM(temperatura, umidade, luzPercent);

      Serial.println("Log crítico salvo na EEPROM.");
    }

  }

  botaoAnterior = estadoBotao; // Atualiza estado do botão para próxima verificação
}

// === FUNÇÕES AUXILIARES ===

// Função para exibir os LEDs conforme código:
// 1 - Verde ligado
// 2 - Amarelo ligado
// 3 - Vermelho ligado + buzzer
// 0 - Apaga tudo
void setLeds(int estado) {
  switch (estado) {
    case 1: // Verde
      digitalWrite(ledVerde, HIGH);
      digitalWrite(ledAmarelo, LOW);
      digitalWrite(ledVermelho, LOW);
      digitalWrite(buzzer, LOW);
      break;
    case 2: // Amarelo
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledAmarelo, HIGH);
      digitalWrite(ledVermelho, LOW);
      digitalWrite(buzzer, LOW);
      break;
    case 3: // Vermelho + buzzer
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledAmarelo, LOW);
      digitalWrite(ledVermelho, HIGH);
      digitalWrite(buzzer, HIGH);
      break;
    default: // Tudo apagado
      digitalWrite(ledVerde, LOW);
      digitalWrite(ledAmarelo, LOW);
      digitalWrite(ledVermelho, LOW);
      digitalWrite(buzzer, LOW);
  }
}

// Função para salvar um registro crítico na EEPROM
// Guarda temperatura, umidade, luz e timestamp do RTC
void salvarLogEEPROM(float temp, float umid, int luz) {
  if (!rtc.isrunning()) {
    Serial.println("RTC não está rodando, não salva log!");
    return;
  }

  DateTime agora = rtc.now();

  // Verifica se EEPROM tem espaço (considerando tamanho fixo do registro)
  if (enderecoEEPROM + TAMANHO_REGISTRO > EEPROM.length()) {
    Serial.println("EEPROM cheia! Apagando tudo...");
    // Apaga toda EEPROM (opcional: só zera os bytes)
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0xFF);
    }
    enderecoEEPROM = 0;
    numLogsEscritos = 0;
  }

  // Grava dados no formato:
  // Byte 0: temperatura em graus C (inteiro)
  // Byte 1: umidade %
  // Byte 2: luz %
  // Byte 3-6: ano, mês, dia, hora, minuto, segundo (ou outro formato)
  // Byte 9: marcador 0x01 para indicar registro válido

  EEPROM.write(enderecoEEPROM, (byte)temp);
  EEPROM.write(enderecoEEPROM + 1, (byte)umid);
  EEPROM.write(enderecoEEPROM + 2, (byte)luz);
  EEPROM.write(enderecoEEPROM + 3, agora.year() - 2000); // ano a partir de 2000
  EEPROM.write(enderecoEEPROM + 4, agora.month());
  EEPROM.write(enderecoEEPROM + 5, agora.day());
  EEPROM.write(enderecoEEPROM + 6, agora.hour());
  EEPROM.write(enderecoEEPROM + 7, agora.minute());
  EEPROM.write(enderecoEEPROM + 8, agora.second());
  EEPROM.write(enderecoEEPROM + 9, 0x01); // marcador de registro válido

  enderecoEEPROM += TAMANHO_REGISTRO;
  numLogsEscritos++;
}

// Função para comparar floats com margem de erro pequena, evitando gravações repetidas
bool isEqualFloat(float a, float b) {
  if (isnan(a) && isnan(b)) return true;
  if (isnan(a) || isnan(b)) return false;
  return fabs(a - b) < 0.1;
}

// Função para o loop de modo auditoria: navega pelos logs na EEPROM com o botão
void modoAuditoriaLoop(bool estadoBotao) {
  static bool botaoAnteriorAud = HIGH;
  static unsigned long debounceTempo = 0;
  const unsigned long debounceDelay = 200;

  unsigned long agora = millis();

  // Debounce simples do botão
  if (estadoBotao != botaoAnteriorAud) {
    debounceTempo = agora;
  }
  if ((agora - debounceTempo) > debounceDelay) {
    if (botaoAnteriorAud == HIGH && estadoBotao == LOW) {
      // Botão pressionado no modo auditoria: avança para próximo log
      if (numLogsEscritos == 0) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("EEPROM VAZIA");
        return;
      }

      indiceLogAtual++;
      if (indiceLogAtual >= numLogsEscritos) {
        indiceLogAtual = 0; // volta ao primeiro log
      }

      exibirLogEEPROM(indiceLogAtual);
    }
  }
  botaoAnteriorAud = estadoBotao;
}

// Função que lê e exibe um log salvo na EEPROM no LCD
void exibirLogEEPROM(int indice) {
  int endereco = indice * TAMANHO_REGISTRO;

  byte temp = EEPROM.read(endereco);
  byte umid = EEPROM.read(endereco + 1);
  byte luz = EEPROM.read(endereco + 2);
  byte ano = EEPROM.read(endereco + 3);
  byte mes = EEPROM.read(endereco + 4);
  byte dia = EEPROM.read(endereco + 5);
  byte hora = EEPROM.read(endereco + 6);
  byte minuto = EEPROM.read(endereco + 7);
  byte segundo = EEPROM.read(endereco + 8);
  byte marcador = EEPROM.read(endereco + 9);

  if (marcador != 0x01) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("LOG INVALIDO");
    return;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(temp);
  lcd.print("C U:");
  lcd.print(umid);
  lcd.print("%");

  lcd.setCursor(0,1);
  lcd.print("L:");
  lcd.print(luz);
  lcd.print("% ");

  lcd.print(hora);
  lcd.print(":");
  if (minuto < 10) lcd.print("0");
  lcd.print(minuto);

  lcd.print(" ");

  lcd.print(dia);
  lcd.print("/");
  lcd.print(mes);
  lcd.print("/");
  lcd.print(ano + 2000);
}
