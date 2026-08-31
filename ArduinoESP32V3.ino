#include <Arduino.h>
#include "estimulo3.h"

uint estado;

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("=================================");
    Serial.println("   PRUEBA DOS PIERNAS");
    Serial.println("=================================");

    // Inicializamos el sistema
    estimulo_init();

    // Parámetros de prueba
    parametros_estimulo_t parametros;

    // Pulsos chicos
    parametros.amplitud_v    = 3.3f;
    parametros.frecuencia_hz = 100.0f;
    parametros.t_pulso_us   = 500;
    parametros.ipi_us       = 100;

    // Envolvente del pulso grande
    parametros.t_subida_us = 2000000;   // 2 s
    parametros.t_meseta_us = 3000000;   // 3 s
    parametros.t_bajada_us = 2000000;   // 2 s
    parametros.t_pausa_us = 5000000;    // 5 s

    // Cargamos los parámetros
    estimulo_set_params(&parametros);

    Serial.println("Parametros configurados.");
    Serial.println();
}


void loop()
{
 estimulo_leer_serial();
 estimulo_update();
}