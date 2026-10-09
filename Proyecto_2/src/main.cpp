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

#define tamMsg 3 // Tamaño del mensaje: Led, tiempo en alto, tiempo en bajo

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
float adcRawEMA = 0; // EMA para el potenciometro
const float alpha = 0.1;   // Factor de suavizado (0-1)
float adcRawEMA_Bits = 0; // Variable para almacenar el valor del ADC
float adcRawEMA_MV = 0; // Variable para almacenar el valor del ADC en milivoltios
float adcRawBits = 0; // Variable para almacenar el valor del ADC
float adcRawMV = 0; // Variable para almacenar el valor del ADC en miliv

//*************************************************************************/
// Prototipos de funciones
//*************************************************************************/

void opcionUno();
void opcionDos();
uint32_t adc_potenciometro_bits();
float adc_potenciometro_mv();

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
  adcRawEMA_MV  = analogReadMilliVolts(pinPotenciometro); // Inicializa el ADC para el pin del potenciómetro en milivoltios
  adcRawEMA_Bits = adcRawBits; // Inicializa el valor EMA en bits
  adcRawEMA_MV = adcRawEMA_MV; // Inicializa el valor EMA

}

//*************************************************************************/
// Loop
//*************************************************************************/
void loop()
{
  opcionUno();
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
  adc_potenciometro_bits();
  adc_potenciometro_mv();
}

uint32_t adc_potenciometro_bits () {

  adcRawBits = analogRead(pinPotenciometro);
  adcRawEMA_Bits = alpha * adcRawBits + (1.0 - alpha) * adcRawEMA_Bits;

  int adcPot_8bits = map(adcRawEMA_Bits, 0, 4095, 0, 255); // Mapeo a 8 bits

  return adcPot_8bits;

  Serial.print("Valor del potenciómetro (8 bits): ");
  Serial.println(adcPot_8bits);

}

float adc_potenciometro_mv () {

  adcRawMV = analogReadMilliVolts(pinPotenciometro);
  adcRawEMA_MV = alpha * adcRawMV + (1.0 - alpha) * adcRawEMA_MV;

  return adcRawEMA_MV;

  Serial.print("Valor del potenciómetro (mV): ");
  Serial.println(adcRawEMA_MV);

}