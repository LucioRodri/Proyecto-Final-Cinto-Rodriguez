#pragma once
#ifndef estimulo3_h
#define estimulo3_h

#define PIN_START     32
#define PIN_STOP      33

#define OUT_A_PIN_1 18
#define OUT_B_PIN_1 19
#define DAC_PIN_1   25

#define OUT_A_PIN_2 21
#define OUT_B_PIN_2 22
#define DAC_PIN_2   26

#include <Arduino.h>

// --- STRUCTS ---
typedef enum
{
    ENV_IDLE,
    ENV_SUBIDA,
    ENV_MESETA,
    ENV_BAJADA,
    ENV_PAUSA
} envelope_state_t;

typedef struct
{
    float amplitud_v;

    uint32_t frecuencia_hz;      // frecuencia de los pulsos pequeños
    uint32_t t_pulso_us;         // duración de cada pulso pequeño
    uint32_t ipi_us;             // intervalo entre A y B

    uint32_t t_subida_us;        // duración de la subida de la envolvente
    uint32_t t_meseta_us;        // tiempo a amplitud máxima
    uint32_t t_bajada_us;        // duración de la bajada de la envolvente
    uint32_t t_pausa_us;         // descanso entre pulsos grandes

    float superposicion_pct;
} parametros_estimulo_t;

typedef enum
{
    STIM_IDLE = 0,

    STIM_WAIT_START, //este estado va a ser algo especial para la pierna 2

    STIM_PULSO_A,

    STIM_IPI,

    STIM_PULSO_B,

    STIM_REST

} stim_state_t;


typedef struct
{
    // Parámetros generales de la pierna
    stim_state_t state;
    uint32_t state_us; // Tiempo transcurrido dentro del estado actual
    uint32_t last_tick; // Referencia temporal para calcular dt
    uint32_t rest_us;
    float dac_actual; // Valor actual del DAC en voltios

    // Pines correspondientes a esta pierna
    uint8_t pinA;
    uint8_t pinB;
    uint8_t pinDAC;

    // Para adicionar la envolvente
    envelope_state_t envolvente;
    uint32_t envolvente_us; // cuanto tiempo llevo en el estado de la onda grande
    float amplitud_actual_v; //aca se va a ir guardando el valor de la rampa/meseta, el otro DAC es el que voy usando para el calculo nomas

} pierna_t;

typedef enum
{
    ESTADO_IDLE,
    ESTADO_RUNNING,
    ESTADO_EMERGENCIA

} estado_equipo_t;

extern volatile estado_equipo_t equipo_estado;

// Funciones para iniciar el programa / setear parametros

void estimulo_init();

void estimulo_set_params(const parametros_estimulo_t *p);

void estimulo_start();

void estimulo_stop();

// Funciones que actualizan/cambian cosas

void estimulo_update(); //esta es la que llama el main para ir chequeando las piernas

static void actualizar_pierna(pierna_t *p); //esta es la heavy, donde esta la MDE

static void dac_write_voltage(pierna_t *p, float v); //funcion que actualiza el DAC y hace la conversion digital/analogico

static void actualizar_envolvente(pierna_t *p, uint32_t dt); //esta es la MDE de la envolvente

// Funciones para prueba en debugg

uint estimulo_leer_encendido(void);

void estimulo_leer_serial(void);

void habilitar_encendido(void); 

void estimulo_status(void);

static void mostrar_estado_pierna(pierna_t *p); //esta por si quiero ver que tiene guardado cada pierna

// Para freezear

void esperar_tecla();

#endif