//*************************************************************************/
// Universidad del Valle de Guatemala
// BE3029 - Electrónica Digital 2
// Ana López - 241204
// 12 de octubre de 2026
// Proyecto 2
//*************************************************************************/

//*************************************************************************/
// Librerías
//*************************************************************************/

#include <Arduino.h>
#include <stdio.h>
#include <stdint.h>
#include <ESP32SPISlave.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include "Wire.h" // Librería para I2C
#include <LiquidCrystal.h> // Librería para LCD


//*************************************************************************/
// Definiciones
//*************************************************************************/

// LEDS
#define led_Amarillo 15
#define led_Verde 2
#define led_Rojo 4

#define pinPotenciometro 34

// Solamente permitirá una transacción SPI pendiente
#define QUEUE_SIZE 1

#define tamMsg 3 // Tamaño del mensaje: Led, byte alto del tiempo, byte bajo del tiempo 

#define I2C_DEV_ADDRESS 0x25 // Dirección del esclavo I2C
#define I2C_CMD_MEDIR 'M' // Comando de "solicitar medición"
#define periodoMuestroMs 10 // Cada cuánto se lee el ADC del potenciómetro (en milisegundos)

// Pines para LCD
#define lcd_rs 13
#define lcd_en 12
#define lcd_d4 25
#define lcd_d5 26
#define lcd_d6 27
#define lcd_d7 33

#define periodoLCDms 200 // Cada cuánto se actualiza la pantalla LCD (en milisegundos)

//*************************************************************************/
// Variables globales
//*************************************************************************/

// Instancia del esclavo SPI
ESP32SPISlave slave;

// Control de los LEDs de opción 1
const uint8_t led_Pines[3] = {led_Amarillo, led_Rojo, led_Verde};
const char led_Inicial[3] = {'A', 'R', 'V'};
uint32_t ledFin[3] = {0, 0, 0}; // Instante en que debe de apagarse cada LED
bool ledActivo[3] = {false, false, false};
char ultimoLed = '-';

// Buffers del SPI de opción 1 (alineados a 4 bytes para DMA)
WORD_ALIGNED_ATTR uint8_t spi_tx[4];
WORD_ALIGNED_ATTR uint8_t spi_rx[4];

// Variables para opción 2
const float alpha = 0.1;   // Factor de suavizado (0-1)
float adcRawEMA_Bits = 0; // Variable para almacenar el valor del ADC
float adcRawEMA_MV = 0; // Variable para almacenar el valor del ADC en milivoltios
float adcRawBits = 0; // Variable para almacenar el valor del ADC
float adcRawMV = 0; // Variable para almacenar el valor del ADC en milivoltios

// Variable para almacenar última medición del potenciómetro para enviar al maestro I2C
volatile uint8_t I2Cvalor8 = 0; // Valor del potenciómetro en bits (8 bits) de 0 a 255
volatile uint16_t I2CvalorMV = 0; // Valor del potenciómetro en milivoltios (16 bits) de 0 a 3300 mV

// Comunicación entre los callbacks del I2C y el loop principal
volatile uint8_t I2Ccomando = 0; // Comando recibido del maestro I2C
volatile bool I2CNuevoComando = false; // Indica si hay un nuevo comando recibido del maestro I2C
volatile bool I2CMedicionEnviada = false; // Indica si la medición del potenciómetro está lista para ser enviada
volatile uint8_t I2CenviadoBits = 0; // Valor del potenciómetro en bits (8 bits) enviado al maestro I2C
volatile uint16_t I2CenviadoMV = 0; // Valor del potenciómetro en milivoltios (16 bits) enviado al maestro I2C

uint32_t ultimoMuestreo = 0; // Último instante en que se leyó el potenciómetro

// Objeto para controlar la pantalla LCD
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

uint32_t ultimoLCD = 0; // Último instante en que se actualizó la pantalla LCD

//*************************************************************************/
// Prototipos de funciones
//*************************************************************************/

void opcionUno();
void opcionDos();
uint32_t adc_potenciometro_bits();
float adc_potenciometro_mv();

// Funciones para la comunicación I2C
void onReceieve(int len);
void onRequest();

// Funciones para la pantalla LCD
void inicializarLCD();
void actualizarLCD();

//*************************************************************************/
// ISRs Rutinas de Interrupción
//*************************************************************************/

//*************************************************************************/
// Función principal
//*************************************************************************/
void setup()
{

  //-----------------------Setup para opción 1-----------------------/

  Serial.begin(115200);

  for (int8_t i = 0; i < 3; i++)
  {
    pinMode(led_Pines[i], OUTPUT);
    digitalWrite(led_Pines[i], LOW);
  }

  memset(spi_tx, 0, tamMsg);
  memset(spi_rx, 0, tamMsg);

  // Configura el modo de comunicación SPI
  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(QUEUE_SIZE);
  // HSPI: CS: 15, CLK: 14, MOSI: 13, MISO: 12
  // VSPI: CS: 5, CLK: 18, MOSI: 23, MISO: 19

  slave.begin(VSPI);
  // Se imprimen los pines utilizados
  Serial.print("MOSI Pin: ");
  Serial.println(MOSI);

  Serial.print("MISO Pin: ");
  Serial.println(MISO);

  Serial.print("SCK Pin: ");
  Serial.println(SCK);

  Serial.print("CS Pin: ");
  Serial.println(SS);

  Serial.println("Esclavo SPI listo para recibir datos del maestro.");

  slave.queue(spi_tx, spi_rx, tamMsg);
  slave.trigger();

  // -----------------------Setup para opción 2-----------------------/
  
  adcRawBits = analogRead(pinPotenciometro); // Inicializa el ADC para el pin del potenciómetro en bits
  adcRawMV  = analogReadMilliVolts(pinPotenciometro); // Inicializa el ADC para el pin del potenciómetro en milivoltios
  adcRawEMA_Bits = adcRawBits; // Inicializa el valor EMA en bits
  adcRawEMA_MV = adcRawMV; // Inicializa el valor EMA

  //-----------------------Setup para comunicación I2C-----------------------/
  Wire.onReceive(onReceieve);
  Wire.onRequest(onRequest);

  Wire.begin((uint8_t)I2C_DEV_ADDRESS); // Inicializa el bus I2C como esclavo con la dirección especificada
  Serial.println("Esclavo I2C listo para recibir datos del maestro.");

  //-----------------------Setup para pantalla LCD-----------------------/
  inicializarLCD();

}

//*************************************************************************/
// Loop
//*************************************************************************/
void loop()
{
  opcionUno(); // SPI + LEDs
  opcionDos(); // ADC + EMA
  actualizarLCD(); // Actualiza la pantalla LCD con los valores del potenciómetro y el LED activo

  // Mensajes de depuración
  if (I2CNuevoComando)
  {
    I2CNuevoComando = false; // Se procesa el comando y se reinicia la bandera
    Serial.printf("Comando recibido del maestro I2C: %c\n", I2Ccomando);
  }

  if (I2CMedicionEnviada)
  {
      I2CMedicionEnviada = false; // Se procesa la medición y se reinicia la bandera
      Serial.printf("Medición del potenciómetro enviada al maestro I2C: %u (8 bits), %u mV\n", 
        (unsigned)I2CenviadoBits, (unsigned)I2CenviadoMV);
  }
}

//*************************************************************************/
// Otras funciones
//*************************************************************************/
void opcionUno()
{
  if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE))
  {

    std::vector<size_t> recibidosVec = slave.numBytesReceivedAll();
    size_t recibidos = recibidosVec.empty() ? 0 : recibidosVec[0];

    Serial.printf("recibidos = %u, bytes: %02X %02X %02X\n", (unsigned)recibidos, spi_rx[0], spi_rx[1], spi_rx[2]);

    if (recibidos >= tamMsg)
    {
      uint8_t led = spi_rx[0];
      uint16_t tiempo = ((uint16_t)spi_rx[1] << 8) | spi_rx[2];

      if (led >= 1 && led <= 3 && tiempo > 0)
      {
        uint8_t ledIndex = led - 1;
        digitalWrite(led_Pines[ledIndex], HIGH);
        ledActivo[ledIndex] = true;
        ledFin[ledIndex] = millis() + tiempo;
        ultimoLed = led_Inicial[ledIndex];

        Serial.printf("LED %c encendido por %d ms\n", ultimoLed, tiempo);
      }
    }

    // Preparar la siguiente transacción SPI
    memset(spi_rx, 0, tamMsg);
    slave.queue(spi_tx, spi_rx, tamMsg);
    slave.trigger();
  }

  // Apagar los LEDs que hayan cumplido su tiempo
  uint32_t tiempoActual = millis();
  for (int8_t i = 0; i < 3; i++)
  {
    if (ledActivo[i] && (int32_t)(tiempoActual - ledFin[i]) >= 0)
    {
      digitalWrite(led_Pines[i], LOW);
      ledActivo[i] = false;
      Serial.printf("LED %c apagado\n", led_Inicial[i]);
    }
  }
}

void opcionDos()
{
  uint32_t tiempoActual = millis();

  // Solo mide el potenciómetro cada periodoMuestroMs milisegundos
  if(tiempoActual - ultimoMuestreo < periodoMuestroMs) {
    return;
  }

  ultimoMuestreo = tiempoActual;

  I2Cvalor8 = (uint8_t)adc_potenciometro_bits(); // Actualiza el valor del potenciómetro en bits
  I2CvalorMV = (uint16_t)adc_potenciometro_mv(); // Actualiza el valor del potenciómetro en milivoltios

}

uint32_t adc_potenciometro_bits () {

  adcRawBits = analogRead(pinPotenciometro);
  adcRawEMA_Bits = alpha * adcRawBits + (1.0 - alpha) * adcRawEMA_Bits;
  return map(adcRawEMA_Bits, 0, 4095, 0, 255); // Mapeo a 8 bits
}

float adc_potenciometro_mv () {

  adcRawMV = analogReadMilliVolts(pinPotenciometro);
  adcRawEMA_MV = alpha * adcRawMV + (1.0 - alpha) * adcRawEMA_MV;
  return adcRawEMA_MV;
}

void onRequest() {
  // La respuesta ya fue preparada en onReceive, así que solo se envía al maestro I2C
  I2CMedicionEnviada = true; // Indica que la medición del potenciómetro está lista para ser enviada
}

void onReceieve(int len) {
  while (Wire.available()) {
    I2Ccomando = Wire.read(); // Leer el comando del maestro I2C
  } 

  if (I2Ccomando == I2C_CMD_MEDIR)
  {
    // Se toma la medición actual y se deja lista en la FIFO antes de que el maestro I2C la solicite
    uint16_t mv = I2CvalorMV; // Valor del potenciómetro en milivoltios
    uint8_t bits = I2Cvalor8; // Valor del potenciómetro en bits (8 bits)

    uint8_t tx[3];
    tx[0] = bits; // Valor del potenciómetro en bits (8 bits)
    tx[1] = (mv >> 8) & 0xFF; // Byte alto del valor en milivoltios
    tx[2] = mv & 0xFF; // Byte bajo del valor en milivoltios

    Wire.slaveWrite(tx, 3); // Carga la respuesta en la FIFO para que el maestro I2C la lea

    I2CenviadoBits = bits; // Guardar el valor enviado en bits
    I2CenviadoMV = mv; // Guardar el valor enviado en miliv
  }
  
  I2CNuevoComando = true; // Indica que hay un nuevo comando recibido del maestro I2C

}

void inicializarLCD() {
  lcd.begin(16, 2); // Inicializa la pantalla LCD con 16 columnas y 2 filas

  lcd.setCursor(0, 1); // Posiciona el cursor en la primera columna de la segunda fila
  lcd.print("Pot1: Pot1: LED:");
  
  Serial.println("Pantalla LCD inicializada.");
}

void actualizarLCD() {
  uint32_t tiempoActual = millis();

  // Solo actualiza la pantalla LCD cada periodoLCDms milisegundos
  if((tiempoActual - ultimoLCD) < periodoLCDms) {
    return;
  }

  ultimoLCD = tiempoActual;

  // Se copian los datos compartidos por I2C para trabajar con valores estables
  uint16_t mv = I2CvalorMV; // Valor del potenciómetro en milivoltios
  uint8_t bits = I2Cvalor8; // Valor del potenciómetro en bits (8 bits)

  // Se arma la fila 2 de la pantalla LCD
  char fila2[17]; // 16 caracteres + '\0'
  snprintf(fila2, sizeof(fila2), "%u.%02uV %-5u %c", 
  (unsigned)(mv / 1000), 
  (unsigned)((mv % 1000) / 10), 
  (unsigned)bits, 
  ultimoLed);

  lcd.setCursor(0, 0);
  lcd.print(fila2);

}
