#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <AFMotor.h>
#include <Keypad.h>
#include <max6675.h>


#define pin01 06
#define pin02 07
#define redPin  24   // Pino digital conectado ao terminal R do LED RGB
#define greenPin  26 // Pino digital conectado ao terminal G do LED RGB
#define bluePin  28  // Pino digital conectado ao terminal B do LED RGB
#define buzzerPin  42 // Pino digital conectado ao buzzer


//Funcoes 
void SetVelocidade( int veloc );


// Definição dos estados da máquina
enum State {
  ST_START,       // Estado inicial, aguardando para iniciar a dosagem
  ST_INICIO,       // Estado inicial, aguardando para iniciar a dosagem
  ST_DOSATV,       // Estado de dosagem ativa
  ST_DOSREV,       // Estado de dosagem reversa
};

// Define a intensidade luminosa de cada cor (varia de 0 a 255)
int redIntensity = 255;   // 0 (apagado) a 255 (máximo brilho)
int greenIntensity = 128; // 0 (apagado) a 255 (máximo brilho)
int blueIntensity = 64;   // 0 (apagado) a 255 (máximo brilho)

int pinSO  = 36; // Atribui o pino 8 como SO
int pinCS  = 38; // Atribui o pino 9 como CS
int pinSCK = 40; // Atribui o pino 10 como SCK

float volumeEmMl = 3.5;
float fluxoEmMlPorMinuto = 43.29; //velocidade 100

// Cria uma instância da biblioteca MAX6675.
MAX6675 termoclanek(pinSCK, pinCS, pinSO);
float teplotaC =0; //Temperatura de termopar

int melody01[] = {
    659, 125, // E5, 1/8
    659, 125, // E5, 1/8
    0, 125,   // Silêncio, 1/8
    659, 125, // E5, 1/8
    0, 125,   // Silêncio, 1/8
    523, 125, // C5, 1/8
    659, 125, // E5, 1/8
    0, 125,   // Silêncio, 1/8
    784, 125, // G5, 1/8
    0, 375,   // Silêncio, 3/8
    392, 125, // G4, 1/8
    0, 375    // Silêncio, 3/8
  };

  
// Melodia "melodyICQ" para reproduzir som do ICQ
int melodyICQ[] = {
  1047, 125, // C6, 1/8
  1175, 125, // D6#, 1/8
  1319, 125, // E6#, 1/8
  1175, 125, // D6#, 1/8
  1047, 125, // C6, 1/8
  880, 125,  // A5, 1/8
  784, 125,  // G5, 1/8
  0, 125,    // Silêncio, 1/8
};

// Melodia "melodyStartrek" para reproduzir o tema de Star Trek
int melodyStartrek[] = {
  988, 4, 1175, 8, 1319, 8, 1175, 8,
  988, 4, 1175, 8, 1319, 8, 1175, 8,
  988, 4, 1175, 8, 1319, 8, 1175, 8,
  1319, 4, 1480, 8, 1568, 8, 1480, 8,
  1319, 4, 1175, 8, 988, 8, 880, 8,
  784, 2,
  // Adicione mais notas conforme necessário
};

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display


const byte numRows = 5; // número de linhas do teclado
const byte numCols = 4; // número de colunas do teclado

char keyMap[numRows][numCols] = {
  {'F', 'G', '#', '*'},
  {'1', '2', '3', '^'},
  {'4', '5', '6', 'v'},
  {'7', '8', '9', 'e'},
  {'<', '0', '>', 'n'}    
};

byte rowPins[numRows] = {25, 27, 29, 31}; // Conecte os pinos das linhas do teclado a estes pinos do Arduino
byte colPins[numCols] = {33, 35, 37, 39}; // Conecte os pinos das colunas do teclado a estes pinos do Arduino

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, numRows, numCols);

char key; //Tecla digitada
char buffer[255];

int velocidade = 255;

AF_DCMotor Motor(1);
unsigned long dosingStartTime; // Variável para armazenar o tempo de início da dosagem
unsigned long dosingTime = 5000; // Tempo de dosagem em milissegundos (5 segundos, por exemplo)
// Variável para controlar o estado atual da máquina
State currentState = ST_START;



void SetVelocidade( int veloc )
{
  velocidade = veloc;
  Motor.setSpeed(velocidade);
  Serial.print("Velocidade:");
  Serial.println(velocidade);
}

// Função para calcular o volume de dosagem com base na velocidade da bomba
float calcularVolume(float velocidade)
{
  // Como exemplo, usaremos uma relação linear simples, onde dosingVolume = 43.29 para velocidade = 100.
  fluxoEmMlPorMinuto = 0.4329*velocidade;
  return fluxoEmMlPorMinuto;
}

// Função para calcular o tempo de dosagem com base no volume e no fluxo (opcional)
unsigned long calcularTempoDeDosagem(float volumeEmMl, float fluxoEmMlPorMinuto)
{
  calcularVolume(velocidade);
  return (volumeEmMl / fluxoEmMlPorMinuto) * 60 * 1000; // Tempo fixo em milissegundos para este exemplo (5 segundos)
  //return 10000;
}

void Start_Termopar()
{
  // Lê a temperatura to termopar em ºC
  teplotaC = termoclanek.readCelsius();
}

void Start_Buzzer()
{
  Serial.println("Iniciando Buzzer");
  pinMode(buzzerPin, OUTPUT);
}

void Start_Serial()
{
  Serial.begin(9600);
  Serial.println("Serial Inicializada");
}

void Start_Rele(){
  // put your setup code here, to run once:
  Serial.println("Iniciando Rele");
  pinMode(pin01,OUTPUT);
  digitalWrite(pin01,LOW);
  pinMode(pin02,OUTPUT);
  digitalWrite(pin02,LOW);

}

void Start_LDC()
{
  Serial.println("Iniciando LDC");
  //lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  //lcd.setCursor(3,0);
  Serial.println("Finalizado");
  
}

void PrintXY(int x, int y, char *Texto)
{
  lcd.setCursor(x,y);
  lcd.print(Texto);
}

void CLS(){
  lcd.clear();
}

void Start_Motor(){
  //Motor.setSpeed(velocidade);
  SetVelocidade(velocidade);
  Parar();
}



void WindowsStart()
{
   // Calcula o número de notas (melodia[] tem frequência e duração)
  int numNotes = sizeof(melody01) / sizeof(melody01[0]) / 2;

  // Reproduz o som do Windows iniciando
  for (int i = 0; i < numNotes * 2; i = i + 2) {
    int noteDuration = melody01[i + 1];
    if (melody01[i] == 0) { // Se a frequência for 0, é um silêncio
      noTone(buzzerPin); // Desliga o buzzer (silêncio)
    } else {
      tone(buzzerPin, melody01[i], noteDuration);
    }
    int pauseBetweenNotes = noteDuration * 1.30; // Pausa entre as notas (1.30 vezes a duração da nota)
    delay(pauseBetweenNotes);
  }
}


// Função para tocar a melodia "melodyICQ"
void ICQErro() {
  // Calcula o número de notas (melodyICQ[] tem frequência e duração)
  int numNotes = sizeof(melodyICQ) / sizeof(melodyICQ[0]) / 2;

  // Reproduz o som do ICQ
  for (int i = 0; i < numNotes * 2; i = i + 2) {
    int noteDuration = melodyICQ[i + 1];
    if (melodyICQ[i] == 0) { // Se a frequência for 0, é um silêncio
      noTone(buzzerPin); // Desliga o buzzer (silêncio)
    } else {
      tone(buzzerPin, melodyICQ[i], noteDuration);
    }
    int pauseBetweenNotes = noteDuration * 1.30; // Pausa entre as notas (1.30 vezes a duração da nota)
    delay(pauseBetweenNotes);
  }

  // Pausa entre as sequências de sons
  delay(1000);
}
// Função para tocar a melodia "melodyStartrek"
void Startrek_theme() {
  // Calcula o número de notas (melodyStartrek[] tem frequência e duração)
  int numNotes = sizeof(melodyStartrek) / sizeof(melodyStartrek[0]) / 2;

  // Reproduz o tema de Star Trek
  for (int i = 0; i < numNotes * 2; i = i + 2) {
    int noteDuration = melodyStartrek[i + 1];
    if (melodyStartrek[i] == 0) { // Se a frequência for 0, é um silêncio
      noTone(buzzerPin); // Desliga o buzzer (silêncio)
    } else {
      tone(buzzerPin, melodyStartrek[i], noteDuration);
    }
    int pauseBetweenNotes = noteDuration * 1.30; // Pausa entre as notas (1.30 vezes a duração da nota)
    delay(pauseBetweenNotes);
    noTone(buzzerPin); // Desliga o buzzer após a duração da nota
    delay(50); // Pequena pausa entre as notas
  }

  // Pausa entre as sequências de sons
  delay(1000);
}

void Inicia_Variaveis(){
  Serial.println("Iniciando Variaveis");
  currentState = ST_START;
  memset(buffer,'/0',sizeof(buffer));
  calcularVolume(velocidade); /*Calcula volume*/
  PrintXY(0,0,"        Dosador");
  PrintXY(0,1,"       Versao 1.0");
  WindowsStart();
}

void Start_Led()
{
  Serial.println("Iniciando Led");
  // Configura os pinos como saídas
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void setup() {
  Start_Serial();
  Start_Buzzer();
  Startrek_theme();
  Start_LDC();
  Start_Rele();  
  Start_Motor();
  Start_Led();
  Inicia_Variaveis();
  Start_Termopar();
  Serial.println("Fim de Setup");
}


void Succao(){
  dosingTime = calcularTempoDeDosagem(volumeEmMl, fluxoEmMlPorMinuto);
  //FORWARD
  Motor.run(BACKWARD); //delante
  dosingStartTime = millis();
  currentState = ST_DOSATV;
}

void Parar(){
  Motor.run(RELEASE);
  currentState = ST_INICIO;
  ICQErro();
}

void Despejar(){
  dosingTime = calcularTempoDeDosagem(volumeEmMl, fluxoEmMlPorMinuto);
  //BACKWARD
  Motor.run(FORWARD); //paro
  dosingStartTime = millis();
  currentState = ST_INICIO;
}


void Analisa_DosAtv()
{

  // Verifica se o tempo de dosagem foi alcançado
  if (millis() - dosingStartTime >= dosingTime)
  {
        Parar(); // Para a dosagem
        currentState = ST_INICIO;
  }
}

void Wellcome()
{
  PrintXY(0,0,"   DOSADOR V.1    ");
  PrintXY(0,1,"F1-DOSE | F2-SETUP");
  Serial.println("Bem vindo ao sistema");
  currentState = ST_INICIO;  
}

void Controla_Led()
{
   // Controla o LED RGB definindo a intensidade de cada cor usando PWM
  analogWrite(redPin, redIntensity);
  analogWrite(greenPin, greenIntensity);
  analogWrite(bluePin, blueIntensity);
}

void Le_Teclado()
{
  key = keypad.getKey();
}

void Le_Termopar()
{
  // Lê a temperatura to termopar em ºC
  teplotaC = termoclanek.readCelsius();
  char strtmp[20];
  memset(strtmp,'\0',sizeof(strtmp));
  dtostrf(teplotaC,7,2,strtmp);
  strcat(strtmp,"C");
  PrintXY(0,2,strtmp);
  
}

void Leituras()
{
  Le_Termopar();
  Le_Teclado();
}

void Analisar()
{
  Leituras();
  switch (currentState)
  { 
    case ST_START:
      Wellcome();
      Parar(); // Inicia a dosagem  
      break;
    case ST_INICIO:
      // Seu código para determinar quando iniciar a dosagem
             
      break;

    case ST_DOSATV:
      // Seu código para determinar quando parar a dosagem
      Analisa_DosAtv();
      break;

    case ST_DOSREV:
      Analisa_DosAtv();
      break;
  }
}




void loop()
{    
  Analisar();
}
