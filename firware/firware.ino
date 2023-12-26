#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <AFMotor.h>
#include <Keypad.h>
#include <max6675.h>
#include <string.h>
#include <ctype.h>
#include <EEPROM.h>

#define pin01 42
#define pin02 44
#define redPin  24   // Pino digital conectado ao terminal R do LED RGB
#define greenPin  26 // Pino digital conectado ao terminal G do LED RGB
#define bluePin  28  // Pino digital conectado ao terminal B do LED RGB
#define buzzerPin  34 // Pino digital conectado ao buzzer


boolean flgDispChange = false;
int MenuCont = 0;
int MAXCont = 5; //Maximo de menus
int MAXBarra = 255;
int posBarra = 0;
int velocidade = 255;
float volumeEmMl = 3.5;
//float fluxoEmMlPorMinuto = 43.29; //velocidade 100
float fluxoEmMlPorMinuto = 82.25; //velocidade 100
float valordigitado = 0;
float maxTemperatura = 99; //Temperatura maxima permitida
float Temperatura = 60; //Temperatura indicada
bool Aquecimento = true;
//Funcoes 

void PrintfXY(int x, int y, float valor);
float getTemperatura();
float getConteudo(); 
int getVelocidade(); 
bool getAquecimento(); 
void setTemperatura(float temperatura); 
void setConteudo(float Conteudo);
void SetVelocidade( int veloc );
void setAquecimento(bool Conteudo); 
void Rele(bool flgctrl);


// Definição dos estados da máquina
enum State {
  ST_START,       // Estado inicial, aguardando para iniciar a dosagem
  ST_INICIO,       // Estado inicial, aguardando para iniciar a dosagem
  ST_DOSATV,       // Estado de dosagem ativa
  ST_SET_INIC,     // Estado de Setup Inicial 
  ST_SET_VELOC_INIC, //Inicio Velocidade   
  ST_SET_CONTE_INIC, //Inicio Conteudo
  ST_SET_TEMPE_INIC,  //Inicio Temperatura
  ST_SET_AQUEC_INIC  //Inicio Aquecimento  
};

// Define a intensidade luminosa de cada cor (varia de 0 a 255)
int redIntensity = 255;   // 0 (apagado) a 255 (máximo brilho)
int greenIntensity = 128; // 0 (apagado) a 255 (máximo brilho)
int blueIntensity = 64;   // 0 (apagado) a 255 (máximo brilho)

int pinSO  = A9; // Atribui o pino 8 como SO
int pinCS  = A10; // Atribui o pino 9 como CS
int pinSCK = A11; // Atribui o pino 10 como SCK


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

byte rowPins[numRows] = {25, 27, 29, 31,33}; // Conecte os pinos das linhas do teclado a estes pinos do Arduino
byte colPins[numCols] = {41, 39, 37, 35}; // Conecte os pinos das colunas do teclado a estes pinos do Arduino

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, numRows, numCols);

char key; //Tecla digitada
char buffer[255];


AF_DCMotor Motor(1);
unsigned long dosingStartTime; // Variável para armazenar o tempo de início da dosagem
unsigned long dosingTime = 5000; // Tempo de dosagem em milissegundos (5 segundos, por exemplo)
// Variável para controlar o estado atual da máquina
State currentState = ST_START;


float getTemperatura() 
{
  byte floatBytes[4];
  
  for (int i = 0; i < 4; i++) {
    floatBytes[i] = EEPROM.read(i);
  }
  
  float temperatura;
  memcpy(&temperatura, &floatBytes, sizeof(temperatura));

  return temperatura;
}

float getConteudo() 
{
  byte floatBytes[4];
  
  for (int i = 0; i < 4; i++) {
    floatBytes[i+4] = EEPROM.read(i);
  }
  
  float Conteudo;
  memcpy(&Conteudo, &floatBytes, sizeof(Conteudo));

  return Conteudo;
}


int getVelocidade() 
{
  byte floatBytes[2];
  
  for (int i = 0; i < 2; i++) {
    floatBytes[i+8] = EEPROM.read(i);
  }
  
  int Velocidade;
  memcpy(&Velocidade, &floatBytes, sizeof(Velocidade));

  return Velocidade;
}

bool getAquecimento() 
{
  bool aqueci;
  
  aqueci = (EEPROM.read(10)==0?false:true);
   
  return aqueci;
}

void setTemperatura(float temperatura) 
{
  byte* floatBytes = (byte*) &temperatura;

  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, floatBytes[i]);
  }
}

void setConteudo(float Conteudo) 
{
  byte* floatBytes = (byte*) &Conteudo;

  for (int i = 0; i < 4; i++) {
    EEPROM.write(i+4, floatBytes[i]);
  }
}


void SetVelocidade( int veloc )
{
   byte* floatBytes = (byte*) &veloc;

  for (int i = 0; i < 2; i++) {
    EEPROM.write(i+8, floatBytes[i]);
  }
  velocidade = veloc;
  Motor.setSpeed(velocidade);
  Serial.print("Velocidade:");
  Serial.println(velocidade);
}

void setAquecimento(bool Conteudo) 
{ 
    EEPROM.write(10, (Conteudo==true?255:0)); 
}


void ChangeState( State value)
{
  currentState = value;
}

float atualizarNumero(float numeroAtual, char caracter) {
    char str[50];
    int intNumero = numeroAtual*100;
    sprintf(str, "%d", intNumero);  // Convert float to string

    if (caracter == '<') {
        // Se o caractere é '<', remover o último dígito
        int length = strlen(str);
        if (length > 0) {
            // Se o último dígito é o ponto decimal, remover mais um dígito
            if (str[length - 1] == '.') {
                str[length - 2] = '\0';
            } else {
                str[length - 1] = '\0';
            }
        }
        // Se a string ficou vazia, definir o número como zero
        if (str[0] == '\0') {
            numeroAtual = 0.0f;
        } else {
            numeroAtual = atof(str);  // Convert string back to float
        }
    } else if (caracter == '#') {
        numeroAtual = 0.0f;
    } else if (isdigit(caracter)) {
        Serial.println("Entrou aqui!");
        // Se o caractere é um dígito, adicioná-lo ao número atual
        // Adicionar o dígito na parte inteira
        sprintf(str,"%s%c",str,caracter);
        Serial.print("caract:");
        Serial.println(str);
        numeroAtual = atof(str)/100;  // Convert string back to float
        
        Serial.print("numeroAtual:");
        Serial.println(numeroAtual);
    }
    // Caso contrário, ignorar o caractere

    return numeroAtual;
}


// Função para calcular o volume de dosagem com base na velocidade da bomba
float calcularVolume(float velocidade)
{
  // Como exemplo, usaremos uma relação linear simples, onde dosingVolume = 43.29 para velocidade = 100.
  //fluxoEmMlPorMinuto = 0.4329*velocidade;
  return fluxoEmMlPorMinuto;
}

// Função para calcular o tempo de dosagem com base no volume e no fluxo (opcional)
unsigned long calcularTempoDeDosagem(float volumeEmMl, float fluxoEmMlPorMinuto)
{
  calcularVolume(velocidade);
  Serial.print("volumeEmML:");
  Serial.println(volumeEmMl);
  Serial.print("fluxoEmMlPorMinuto:");
  Serial.println(fluxoEmMlPorMinuto);
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

void PrintfXY(int x, int y, float valor)
{
  lcd.setCursor(x,y);
  lcd.print(valor);
}

void CLS(){
  lcd.clear();
}

void Start_Motor(){
  //Motor.setSpeed(velocidade);
  SetVelocidade(velocidade);
  Parar();
}

void ChamaSetup(){
  ChangeState(ST_SET_INIC);
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
void ICQ() {
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

void Inicia_Variaveis(){
  Serial.println("Iniciando Variaveis");
  flgDispChange = false;
  MenuCont = 0;
  //currentState = ST_START;
  ChangeState(ST_START);
  memset(buffer,'\0',sizeof(buffer));
  calcularVolume(velocidade); /*Calcula volume*/
  Temperatura = getTemperatura(); //Pega temperatura da EEPROM
  velocidade = getVelocidade(); //Pega a velocidade da EEPROM
  volumeEmMl = getConteudo(); //Pega o valor do conteudo da EEPROM
  Aquecimento = getAquecimento(); //Pega o valor do conteudo da EEPROM
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
  Serial.println("Sucao");
  dosingStartTime = millis();
  //currentState = ST_DOSATV;
  ChangeState(ST_DOSATV);
  flgDispChange= true;
}

void Parar(){
  Motor.run(RELEASE);
  Serial.println("Parar");
  //currentState = ST_INICIO;
  ChangeState(ST_INICIO);
  flgDispChange= true;
  ICQ();
}

void Despejar(){
  
  dosingTime = calcularTempoDeDosagem(volumeEmMl, fluxoEmMlPorMinuto);
  Serial.println(dosingTime);
  //BACKWARD
  Motor.run(FORWARD); //paro
  Serial.println("Despejar");
  dosingStartTime = millis();
  //currentState = ST_INICIO;
  ChangeState(ST_DOSATV);
  flgDispChange= true;
}


void Analisa_DosAtv()
{
  //Serial.println("Analisa_DosAtv"); 
  // Verifica se o tempo de dosagem foi alcançado
  if ((millis() - dosingStartTime) >= dosingTime)
  {
        Serial.println("Deu tempo");
        Parar(); // Para a dosagem
        //currentState = ST_INICIO;
        ChangeState(ST_INICIO);
        flgDispChange= true;
        
  }
}

void Wellcome()
{
  CLS();
  PrintXY(0,0,"   DOSADOR V.1    ");
  PrintXY(0,1,"                  ");
  PrintXY(0,2,"                  ");
  PrintXY(0,3,"F1-DOSE | F2-SETUP");
  Serial.println("Bem vindo ao sistema");
  //currentState = ST_INICIO;  
  ChangeState(ST_INICIO);
}

void Controla_Led()
{
   // Controla o LED RGB definindo a intensidade de cada cor usando PWM
  analogWrite(redPin, redIntensity);
  analogWrite(greenPin, greenIntensity);
  analogWrite(bluePin, blueIntensity);
}

void Rele(bool flgctrl)
{
  // put your setup code here, to run once:
  
  if(!flgctrl)
  {
    if(digitalRead(pin02)==HIGH)
    {
      
      digitalWrite(pin01,LOW);
      digitalWrite(pin02,LOW);
      
      Serial.println("mudou Baixo");
    }
  } else 
  {
    if(digitalRead(pin02)==LOW)
    {
      digitalWrite(pin01,LOW);
      digitalWrite(pin02,HIGH);
      
      Serial.println("mudou Alto");
    }
  }
}





//*******Inicio de bloco de Setup***
void Mostra_Menu()
{
  //MenuCont
  int pagina = (MenuCont / 2);
  int posicaopag = MenuCont % 2;
  char texto[40];
  switch(pagina) {
    case 0:
      sprintf(texto,"%sVelocidade",(posicaopag==0?"*":" ") );
      PrintXY(0,1,texto);
      sprintf(texto,"%sConteudo",(posicaopag==1?"*":" ") );
      PrintXY(0,2,texto);
      break;
    case 1:
      sprintf(texto,"%sTemperatura",(posicaopag==0?"*":" ") );
      PrintXY(0,1,texto);
      sprintf(texto,"%sAquecimento",(posicaopag==1?"*":" ") );
      PrintXY(0,2,texto);
      break;
    case 2:
      sprintf(texto,"%sIntegracao",(posicaopag==0?"*":" ") );
      PrintXY(0,1,texto);
      sprintf(texto,"%sInfo",(posicaopag==1?"*":" ") );
      PrintXY(0,2,texto);
      break;    
    case 3:
      sprintf(texto,"%s             ",(posicaopag==0?"*":" ") );
      PrintXY(0,1,texto);
      sprintf(texto,"%s             ",(posicaopag==1?"*":" ") );
      PrintXY(0,2,texto);
      break;          
  }  
}

//Mostra velocidade
void Mostra_Velocidade()
{
    //flgDispChange=false;
    //Serial.println("Mostra_Velocidade");
    PrintXY(0,1,"Digite ^ ou v");
    Serial.print("POSBARRA:");
    Serial.println(posBarra);
    char barra[20]; /*Barra de demonstracao*/ 
    memset(barra,'\0',20); /*Iniciando barra*/  
    int total = ((posBarra*100/255)*20)/100;
    
    Serial.print("Total:");
    Serial.println(total);
    for(int cont=0;cont<(int)total;cont++)
    {
     strcat(barra,"#");
    }
    Serial.print("Barra:");
    Serial.println(barra);
    PrintXY(0,2,barra);  
}

//Mostra Conteudo
void Mostra_Conteudo()
{
    PrintXY(0,1,"Digite o valor, # < ");
    PrintfXY(0,2,valordigitado);  
}

//Mostra Conteudo
void Mostra_Temperatura()
{
    PrintXY(0,1,"Digite o valor, # < ");
    PrintfXY(0,2,valordigitado);  
}

void Mostra_Aquecimento()
{
    PrintXY(0,1,"Setas para mudar, # < ");
    PrintXY(0,2,(valordigitado==0?"Desligado:":"Ativo"));  
}

void Setup_Inicio()
{
  if (flgDispChange)
  {
    CLS();
    PrintXY(0,0,"     ### SETUP ### ");
    Mostra_Menu();
  
  
    PrintXY(0,3,"Setas | ENT selec  ");
    flgDispChange=false;
  }  
}

void Setup_Veloc_Inic()
{
  
  if(flgDispChange)
  {    
    CLS();
    flgDispChange = false;
    PrintXY(0,0,"# SETUP VELOCIDADE #");
    Mostra_Velocidade();
    
    PrintXY(0,3,"SETAS | ENT/ESC ");
    //Serial.println("Setup_Veloc_Inic()");
  }  

}

void Setup_CONTE_Inic()
{
  
  if(flgDispChange)
  {    
    CLS();
    flgDispChange = false;
    PrintXY(0,0,"# SETUP CONTEUDO #");
    Mostra_Conteudo();
    
    PrintXY(0,3,"DIGITE | ENT/ESC ");
  }  
}

void Setup_TEMPE_Inic()
{
  
  if(flgDispChange)
  {    
    CLS();
    flgDispChange = false;
    PrintXY(0,0,"# SETUP TEMPERATURA #");
    Mostra_Temperatura();
    PrintXY(0,3,"DIGITE | ENT/ESC ");
  }  

}

void Setup_AQUEC_Inic()
{
  
  if(flgDispChange)
  {    
    CLS();
    flgDispChange = false;
    PrintXY(0,0,"# SETUP AQUECIMENTO #");
    Mostra_Aquecimento();
    PrintXY(0,3,"SETA | ENT/ESC ");
  }  

}


void MudaSetup()
{
  int pagina = (MenuCont / 2);
  int posicaopag = MenuCont % 2;
  
  Serial.println("MudaSetup()");
  //Muda para setup de velocidade
  if ((pagina==0)&&(posicaopag==0)) {
    Serial.println("Mudou para ST_SET_VELOC_INIC");
    flgDispChange= true;
    //Atribui valor de velocidade
    
    ChangeState(ST_SET_VELOC_INIC);
    posBarra = velocidade;
    Serial.println("Fim de MudaSetup()");
  }
  if ((pagina==0)&&(posicaopag==1)) {
    Serial.println("Mudou para ST_SET_CONTE_INIC");
    flgDispChange= true;
    //Atribui valor de velocidade
    
    ChangeState(ST_SET_CONTE_INIC);
    valordigitado = volumeEmMl;
    Serial.println("Fim de MudaSetup()");
  //ST_SET_CONTE_INIC
  }
  if ((pagina==1)&&(posicaopag==0)) {
    Serial.println("Mudou para ST_SET_TEMPE_INIC");
    flgDispChange= true;
    //Atribui valor de temperatura
    
    ChangeState(ST_SET_TEMPE_INIC);
    valordigitado = Temperatura;
    Serial.println("Fim de MudaSetup()");
  //ST_SET_TEMPE_INIC
  }
  if ((pagina==1)&&(posicaopag==1)) {
    Serial.println("Mudou para ST_SET_AQUEC_INIC");
    flgDispChange= true;
    //Atribui valor de temperatura
    
    ChangeState(ST_SET_AQUEC_INIC);
    valordigitado = Aquecimento==true?255:0;
    Serial.println("Fim de MudaSetup()");
    //ST_SET_AQUEC_INIC
  }  
}

//*******Fim de bloco de setup***

void Le_Teclado()
{
  key = keypad.getKey();
  if(key){
    Serial.print("Teclado");
    Serial.println(key);
  }
}

void Le_Termopar()
{
  // Lê a temperatura to termopar em ºC
  teplotaC = termoclanek.readCelsius();
  char strtmp[20];
  memset(strtmp,'\0',sizeof(strtmp));
  
  //Mostra temperatura em tela principal
  if(currentState==ST_INICIO)
  {
    dtostrf(teplotaC,7,2,strtmp);
    strcat(strtmp,"C");
    PrintXY(0,2,strtmp);
  }
  if(Aquecimento)
  {
    //Serial.print("Temperatura:");
    //Serial.println(Temperatura);
    //Serial.print("Atual:");
    //Serial.println(teplotaC);
    if(teplotaC<Temperatura)
    {      
      //Serial.println("Liga sensor");
      Rele(true);
    } else
    {
      //Serial.println("Desliga sensor");
      Rele(false);
    }
    
  }
}

void Leituras()
{
  Le_Termopar();
  Le_Teclado();
  
}

void Analisa_teclas()
{
  //F1
  if(key=='F'){
    if(currentState==ST_INICIO)
    {
      if (Temperatura<=teplotaC)
      {
        Despejar();
        Serial.println("Despejar");
        //flgDispChange= true;
      }
      else
      {
        Serial.println("Temperatura nao alcançada");
        Serial.println(Temperatura);
        Serial.println(teplotaC);
      
      }
    }
    
  }
  if(key=='G'){
    if(currentState==ST_INICIO){
      ChangeState(ST_SET_INIC);
      flgDispChange= true;
      MenuCont = 0;
    }    
  }
  //ESC
  if(key=='e')
  {
    Wellcome();
  }
  //Seta p baixo
  if(key=='v')
  {
    if(currentState==ST_SET_INIC)
    {
      flgDispChange= true;
      if (MenuCont<MAXCont)
      {
        MenuCont = MenuCont +1;
      }
       else
       {
        MenuCont = 0;
       }     
    }
    //Controle de velocidade
    if(currentState==ST_SET_VELOC_INIC)
    {
      flgDispChange= true;
      if (posBarra>0)
      {
        posBarra = posBarra -1;
        
      }
       else
       {
        posBarra = MAXBarra;
       }
       Serial.println(posBarra);     
    }  
    //Controle de Aquecimento
    if(currentState==ST_SET_AQUEC_INIC)
    {
      flgDispChange= true;
      valordigitado = !valordigitado;
      Serial.println(valordigitado);     
    }          
  }
    //Seta p cima
  if(key=='^')
  {
    if(currentState==ST_SET_INIC)
    {
      flgDispChange= true;
      if (MenuCont>0)
      {
        MenuCont = MenuCont -1;
      }
      else
      {
        MenuCont = MAXCont;
      }
      Serial.println(posBarra);
      
    }
    //Controle de velocidade
    if(currentState==ST_SET_VELOC_INIC)
    {
      flgDispChange= true;
      if(posBarra<MAXBarra)
      {
        posBarra= posBarra+1;
      }
      else
      {
        posBarra= 0;
      }
      
    }
    //Controle de Aquecimento
    if(currentState==ST_SET_AQUEC_INIC)
    {
      flgDispChange= true;
      valordigitado = !valordigitado;
      Serial.println(valordigitado);     
    }              
  }
  if(key=='>')
  {
    //Controle de velocidade
    if(currentState==ST_SET_VELOC_INIC)
    {
      flgDispChange= true;
      if(posBarra<MAXBarra)
      {
        posBarra= posBarra+1;
      }
      else
      {
        posBarra= 0;
      }
    }
    //Controle de Aquecimento
    if(currentState==ST_SET_AQUEC_INIC)
    {
      flgDispChange= true;
      valordigitado = !valordigitado;
      Serial.println(valordigitado);     
    }              
  }
  if(key=='<')
  {
    if(currentState==ST_SET_VELOC_INIC)  //Temos problema
    {
      flgDispChange= true;
      valordigitado = atualizarNumero(valordigitado,key);
    }
    if(currentState==ST_SET_VELOC_INIC)
    {
      flgDispChange= true;
      if (posBarra>0)
      {
        posBarra = posBarra -1;
        
      }
       else
      {
        posBarra = MAXBarra;
      }
      Serial.println(posBarra);     
    }    
    //Controle de Aquecimento
    if(currentState==ST_SET_AQUEC_INIC)
    {
      flgDispChange= true;
      valordigitado = !valordigitado;
      Serial.println(valordigitado);     
    }              
  }
  //Seta Enter
  if(key=='#')
  {
    //O Controle derivador do setup
    
    if(currentState==ST_SET_CONTE_INIC)
    {
      Serial.println("#");
      flgDispChange= true;
      //valordigitado = atualizarNumero(valordigitado,key);
      valordigitado = 0;
    } 
       
    //O Controle derivador do setup
    if(currentState==ST_SET_TEMPE_INIC)
    {
      Serial.println("#");
      flgDispChange= true;
      valordigitado = atualizarNumero(valordigitado,key);
    }    
    
   
    //O Controle derivador do setup
    if(currentState==ST_SET_AQUEC_INIC)
    {
      Serial.println("#");
      flgDispChange= true;
      //Aquecimento = ==0?false:true;
      valordigitado = Aquecimento==0?false:true;
      setAquecimento(Aquecimento);
    }       
  }
  //Seta Enter
  if(key=='n')
  {
    Serial.println(currentState);
    
    if(currentState==ST_SET_INIC)
    {
      ICQ();
      MudaSetup();
    } else
    {
      //O Controle derivador do setup
      if(currentState==ST_SET_VELOC_INIC)
      {
        ///ST_SET_INIC  
        flgDispChange= true;
        ICQ();
        //velocidade=posBarra;
        SetVelocidade(posBarra);
        ChangeState(ST_SET_INIC);   
      }    
      //O Controle derivador do setup
      if(currentState==ST_SET_CONTE_INIC)
      {
        flgDispChange= true;
        ICQ();
        volumeEmMl = valordigitado;
        setConteudo(volumeEmMl); //Grava em EEPROM
        ChangeState(ST_SET_INIC);           
      }
      //O Controle derivador do setup
      if(currentState==ST_SET_TEMPE_INIC)
      {
        flgDispChange= true;
        ICQ();
        if (valordigitado > maxTemperatura)
        { 
          Temperatura = maxTemperatura; 
          setTemperatura(Temperatura);   
               
          PrintXY(0,2,"Temp MAX Excedida");
        } else {        
          Temperatura = valordigitado;
          setTemperatura(Temperatura);   
          ChangeState(ST_SET_INIC); 
        }                  
      }
      //O Controle derivador do setup
      if(currentState==ST_SET_AQUEC_INIC)
      {
        flgDispChange= true;
        ICQ();
        
        Aquecimento = valordigitado==0?false:true;
        setAquecimento(Aquecimento);
        ChangeState(ST_SET_INIC); 
      }
      
    }
  }
  //Digitou valores
  if(isdigit(key))
  {
    Serial.println("Numeros");
    if(currentState==ST_SET_CONTE_INIC)
    {      
      flgDispChange= true;
      valordigitado = atualizarNumero(valordigitado,key);
      Serial.print("valordigitado:");
      Serial.println(valordigitado);
    }
    if(currentState==ST_SET_TEMPE_INIC)
    {      
      flgDispChange= true;
      valordigitado = atualizarNumero(valordigitado,key);
      Serial.print("valordigitado:");
      Serial.println(valordigitado);
    }    
  }

  key = ' '; //Zera key
}

void Analisar()
{
  Leituras();
  Analisa_teclas();
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

    case ST_SET_INIC:
      //Opcoes de Inicio de setup
      Setup_Inicio();
      break;
    case ST_SET_VELOC_INIC:
      Setup_Veloc_Inic();
      break;
    case ST_SET_CONTE_INIC:
      Setup_CONTE_Inic();
      break;   
    case ST_SET_TEMPE_INIC:
      Setup_TEMPE_Inic();
      break;    
    case ST_SET_AQUEC_INIC:
      Setup_AQUEC_Inic();
      break;           
  }
  
}

void loop()
{    
  Analisar();
}
