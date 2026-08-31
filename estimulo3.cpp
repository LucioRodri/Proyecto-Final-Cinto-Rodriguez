#include "estimulo3.h"
#define OUT_A_PIN_1 18
#define OUT_B_PIN_1 19
#define DAC_PIN_1   25

#define OUT_A_PIN_2 21
#define OUT_B_PIN_2 22
#define DAC_PIN_2   26

volatile estado_equipo_t equipo_estado = ESTADO_IDLE;

// Variables globales--------------------------------------------------
static parametros_estimulo_t g_stim;

static pierna_t pierna1;
static pierna_t pierna2;

//----------------------------------------------------------------------

static void dac_write_voltage(pierna_t *p, float v) //funcion para escribir un valor de voltaje en el DAC, con un rango de 0 a 3.3V, y convertirlo a un valor de 0 a 255 para escribirlo en el DAC
{
    if (v < 0)
        v = 0;

    if (v > 3.3)
        v = 3.3;

    uint8_t dac_value = (uint8_t)((v / 3.3f) * 255.0f);

    dacWrite(p->pinDAC, dac_value);

    p->dac_actual = v;
}

//------------------------------------------------------------
// Aca empieza el inicio y seteo de parametros y pines
void estimulo_init()
{
    // PIERNA 1
    pierna1.pinA = OUT_A_PIN_1;
    pierna1.pinB = OUT_B_PIN_1;
    pierna1.pinDAC = DAC_PIN_1;

    pinMode(pierna1.pinA, OUTPUT);
    pinMode(pierna1.pinB, OUTPUT);

    digitalWrite(pierna1.pinA, LOW);
    digitalWrite(pierna1.pinB, LOW);

    dac_write_voltage(&pierna1, 0);

    // PIERNA 2
    pierna2.pinA = OUT_A_PIN_2;
    pierna2.pinB = OUT_B_PIN_2;
    pierna2.pinDAC = DAC_PIN_2;

    pinMode(pierna2.pinA, OUTPUT);
    pinMode(pierna2.pinB, OUTPUT);

    digitalWrite(pierna2.pinA, LOW);
    digitalWrite(pierna2.pinB, LOW);

    dac_write_voltage(&pierna2, 0);

    // PINES DE CONTROL
    pinMode(PIN_START, INPUT_PULLUP);
    pinMode(PIN_STOP, INPUT_PULLUP);

    // Nueva máquina de envolvente
    pierna1.envolvente = ENV_PAUSA;
    pierna2.envolvente = ENV_PAUSA;

    pierna1.envolvente_us = 0;
    pierna2.envolvente_us = 0;

    pierna1.amplitud_actual_v = 0;
    pierna2.amplitud_actual_v = 0;
}

void estimulo_set_params(const parametros_estimulo_t *p)
{
    g_stim = *p;
    uint32_t periodo_us = 1000000UL / g_stim.frecuencia_hz;
    pierna1.rest_us = periodo_us - g_stim.t_pulso_us - g_stim.ipi_us - g_stim.t_pulso_us; //el periodo en total va a ser 2 veces el tiempo de pulso (el positivo y negativo), el IPI y el resto
    pierna2.rest_us = pierna1.rest_us;
}

void habilitar_encendido(void) //esto simplemente para poder habilitar el pin 19
{
digitalWrite(PIN_START, HIGH);
} 

//--------------------------------------------------------------------------------
//Aca empiezan los comando para empezar o detener el estimulo, asociados a pin 18 y 19

void estimulo_start()
{
    // PIERNA 1
    pierna1.state = STIM_PULSO_A;
    pierna1.state_us = 0;

    pierna1.envolvente = ENV_SUBIDA;
    pierna1.envolvente_us = 0;

    pierna1.amplitud_actual_v = 0;

    pierna1.last_tick = micros();

    // PIERNA 2
    pierna2.state = STIM_WAIT_START;
    pierna2.state_us = 0;

    // La segunda pierna todavía está esperando
    // su desfase de 2*t_pulso + IPI.
    pierna2.envolvente = ENV_IDLE;
    pierna2.envolvente_us = 0;

    pierna2.amplitud_actual_v = 0;

    pierna2.last_tick = micros();


    Serial.println("Estimulación iniciada");
}

void estimulo_stop()
{
    // Detener máquina de pulsos pequeños
    pierna1.state = STIM_IDLE;
    pierna2.state = STIM_IDLE;

    // Detener máquina de envolvente
    pierna1.envolvente = ENV_IDLE;
    pierna2.envolvente = ENV_IDLE;

    // Reiniciar tiempos
    pierna1.state_us = 0;
    pierna2.state_us = 0;

    pierna1.envolvente_us = 0;
    pierna2.envolvente_us = 0;

    // Amplitudes a cero
    pierna1.amplitud_actual_v = 0;
    pierna2.amplitud_actual_v = 0;

    // Apagar salidas
    digitalWrite(pierna1.pinA, LOW);
    digitalWrite(pierna1.pinB, LOW);

    digitalWrite(pierna2.pinA, LOW);
    digitalWrite(pierna2.pinB, LOW);

    dac_write_voltage(&pierna1, 0);
    dac_write_voltage(&pierna2, 0);

    Serial.println("Estimulación detenida");
}

uint estimulo_leer_encendido()
{
    if(digitalRead(PIN_START)==HIGH) //el pin de start es el 19, esto es para que se prenda
    {
        Serial.println("Estimulacion encendida");
        return 1;
    }

    if(digitalRead(PIN_STOP)==HIGH || digitalRead(PIN_START)==LOW) //esto si se activa el boton de apagar o si el de prendido esta no pulsado
    {
        Serial.println("Estimulacion apagada");
        estimulo_stop();
        return 0;
    }
    else
    {
        Serial.println("Error, apagando estimulacion");
        estimulo_stop();
        return 0;
    }

}

//--------------------------------------------------
//Esto es simplemente para probar en el codigo

void estimulo_leer_serial()
{
    if(Serial.available())
    {
        char c = Serial.read();

        switch(c)
        {
            case 's':

                estimulo_start();
                break;

            case 'x':

                estimulo_stop();
                break;
                
            case 'i':

                estimulo_status();
                break;

            default:

                Serial.println("Comando invalido");
                break;
                
        }
    }
}

static void mostrar_estado_pierna(pierna_t *p)
{
    Serial.println();

    Serial.println("----- PIERNA -----");

    Serial.print("Estado: ");

    switch (p->state)
    {
        case STIM_PULSO_A:
            Serial.println("STIM_PULSO_A");
            break;

        case STIM_PULSO_B:
            Serial.println("STIM_PULSO_B");
            break;

        case STIM_IPI:
            Serial.println("STIM_IPI");
            break;

        case STIM_REST:
            Serial.println("STIM_REST");
            break;

        case STIM_IDLE:
            Serial.println("STIM_IDLE");
            break;

        default:
            Serial.println("ESTADO DESCONOCIDO");
            break;
    }

    Serial.print("OUT A: ");
    Serial.println(digitalRead(p->pinA));

    Serial.print("OUT B: ");
    Serial.println(digitalRead(p->pinB));

    Serial.print("DAC (V): ");
    Serial.println(p->dac_actual);

    Serial.print("Tiempo estado (us): ");
    Serial.println(p->state_us);

    Serial.print("Descanso (us): ");
    Serial.println(p->rest_us);

    Serial.println("------------------");
}

void esperar_tecla()
{
    Serial.println();
    Serial.println("Presione ENTER para continuar...");

    while(!Serial.available())
    {
        delay(10);
    }

    while(Serial.available())
    {
        Serial.read();
    }

    Serial.println("Continuando...");
}

void estimulo_status()
{
    Serial.println();
    Serial.println("========== ESTADO DEL EQUIPO ==========");

    Serial.print("Amplitud: ");
    Serial.println(g_stim.amplitud_v);

    Serial.print("Frecuencia: ");
    Serial.println(g_stim.frecuencia_hz);

    Serial.print("Pulso (us): ");
    Serial.println(g_stim.t_pulso_us);

    Serial.print("IPI (us): ");
    Serial.println(g_stim.ipi_us);

    Serial.println();

    mostrar_estado_pierna(&pierna1);

    Serial.println("=======================================");
}

//----------------------------------------------------------------------
// Aca la funcion MDE que actualiza la envolvente
//comentario, esta funcion no llama directamente a la que sobrescribe el valor del dac (dac_writevoltage), sino que solo calcula el valor que debe tener
//esto lo hice con la intención de que la funcion que cambia el valor del dac, siga siendo llamada solo en actualizar pierna, para que no haya lio
//ya que la pierna 2 empieza después

static void actualizar_envolvente(pierna_t *p, uint32_t dt)
{
    p->envolvente_us += dt;

    switch (p->envolvente)
    {
        case ENV_IDLE:
        {
            p->amplitud_actual_v = 0;
            p->envolvente_us = 0;
            break;
        }

        case ENV_SUBIDA:
        {
            if (p->envolvente_us >= g_stim.t_subida_us)
            {
                // Terminó la subida
                p->envolvente_us = 0;
                p->envolvente = ENV_MESETA;

                p->amplitud_actual_v = g_stim.amplitud_v; //como paso a meseta clavo el valor en lo que setea el usuario
            }
            else
            {
                // Rampa lineal de 0 V hasta amplitud_v
                if (g_stim.t_subida_us > 0)
                {
                    p->amplitud_actual_v = g_stim.amplitud_v * ((float)p->envolvente_us/(float)g_stim.t_subida_us);
                }
                else
                {
                    p->amplitud_actual_v = g_stim.amplitud_v;
                }
            }

            break;
        }

        case ENV_MESETA:
        {
            p->amplitud_actual_v = g_stim.amplitud_v;

            if (p->envolvente_us >= g_stim.t_meseta_us)
            {
                p->envolvente_us = 0;
                p->envolvente = ENV_BAJADA;
            }

            break;
        }

        case ENV_BAJADA:
        {
            if (p->envolvente_us >= g_stim.t_bajada_us)
            {
                // Terminó la bajada
                p->envolvente_us = 0;
                p->envolvente = ENV_PAUSA;

                p->amplitud_actual_v = 0;
            }
            else
            {
                // Rampa lineal desde amplitud_v hasta 0 V
                if (g_stim.t_bajada_us > 0)
                {
                    p->amplitud_actual_v = g_stim.amplitud_v *(1.0f - ((float)p->envolvente_us/(float)g_stim.t_bajada_us));
                }
                else
                {
                    p->amplitud_actual_v = 0;
                }
            }

            break;
        }

        case ENV_PAUSA:
        {
            p->amplitud_actual_v = 0;

            if (p->envolvente_us >= g_stim.t_pausa_us)
            {
                p->envolvente_us = 0;
                p->envolvente = ENV_SUBIDA;
                p->amplitud_actual_v = 0;

                 // Cada nuevo pulso grande comienza con A
                p->state = STIM_PULSO_A;
                p->state_us = 0;
            }

            break;
        }
    }
}

//----------------------------------------------------------------------
// Aca la funcion main que va actualizando el estimulo

static void actualizar_pierna(pierna_t *p)
{
	//ahora primero voy chequeando los tiempos, y después actualizo el estado, para asegurarme de que el estímulo se mantenga durante el tiempo correcto, aunque haya algún retraso en la ejecución del código
    uint32_t ahora = micros();
    uint32_t dt = ahora - p->last_tick;

    if (dt == 0)
    {
        return;
    }

    p->last_tick = ahora;
    p->state_us += dt;

// ------------- Parte de la envolvente -------------

    actualizar_envolvente(p, dt); //esto para ver en que parte de la envolvente estoy

    if (p->envolvente == ENV_PAUSA || p->envolvente == ENV_PAUSA) //si la envolvente está en pausa, no quiero que corra la máquina de estados
    {
    digitalWrite(p->pinA, LOW);
    digitalWrite(p->pinB, LOW);
    dac_write_voltage(p, 0);
    return;
    }

// ------------- Termina envolvente -------------

    switch (p->state)
    {
    case STIM_WAIT_START:
    {

      Serial.println("===== Pasamos a Espera de inicio =====");
    // La pierna 2 está esperando su momento de inicio

    digitalWrite(p->pinA, LOW); //me aseguro de que todo esté en 0
    digitalWrite(p->pinB, LOW);
    dac_write_voltage(p, 0);

    uint32_t tiempo_inicio_segunda = 2UL * g_stim.t_pulso_us + g_stim.ipi_us;

    if (p->state_us >= tiempo_inicio_segunda)
    {
        // Arranca la máquina de pulsos pequeños
        p->state = STIM_PULSO_A;
        p->state_us = 0;

        // Arranca también la envolvente del pulso grande
        p->envolvente = ENV_SUBIDA;
        p->envolvente_us = 0;
        p->amplitud_actual_v = 0;
    }
    break;
    }
    
    case STIM_PULSO_A:
    {
        //---------------------------------------------- Para ver debug nomas
       // Serial.println("===== Pasamos a Estimulo A =====");
        //----------------------------------------------

		if (p->envolvente != ENV_PAUSA) //me aseguro que no haya pulsos durante la pausa
        {
        digitalWrite(p->pinA, HIGH);
        digitalWrite(p->pinB, LOW);
        dac_write_voltage(p, p->amplitud_actual_v);
        }

        else //si entro acá es que estoy en la pausa, apago todo
        {
            digitalWrite(p->pinA, LOW);
            digitalWrite(p->pinB, LOW);
            dac_write_voltage(p, 0); //que el dac no mande nada mientras estoy en IPI
        }

        if (p->state_us >= g_stim.t_pulso_us)
        {
            digitalWrite(p->pinA, LOW);
            dac_write_voltage(p, 0);
            p->state = STIM_IPI;
            p->state_us = 0;
        }
            break;
    }

    case STIM_IPI:
    {
        //---------------------------------------------- Para ver debug nomas
        //Serial.println("===== Pasamos a IPI =====");
        //----------------------------------------------
        
        digitalWrite(p->pinA, LOW);
        digitalWrite(p->pinB, LOW);

        dac_write_voltage(p, 0);

        if (p->state_us >= g_stim.ipi_us) //si ya pasó el tiempo de IPI, pasamos al pulso B
        {
            p->state = STIM_PULSO_B;
            p->state_us = 0;
        }
            break;
    }

    case STIM_PULSO_B:
    {
        //---------------------------------------------- Para ver debug nomas
        //Serial.println("===== Pasamos a Estimulo b =====");
        //----------------------------------------------

        if (p->envolvente != ENV_PAUSA)
        {
        digitalWrite(p->pinA, LOW);
        digitalWrite(p->pinB, HIGH);
        dac_write_voltage(p, p->amplitud_actual_v);
        }

        else
        {
        digitalWrite(p->pinA, LOW);
        digitalWrite(p->pinB, LOW);
        dac_write_voltage(p, 0);
        }

        if (p->state_us >= g_stim.t_pulso_us)
            {
                digitalWrite(p->pinB, LOW);
                dac_write_voltage(p, 0);
                p->state = STIM_REST;
                p->state_us = 0;
            }
            break;
    }


    case STIM_REST:
    {
        //---------------------------------------------- Para ver debug nomas
        //Serial.println("===== Pasamos a Descanso =====");
        //----------------------------------------------

        digitalWrite(p->pinA, LOW);
        digitalWrite(p->pinB, LOW);
        dac_write_voltage(p, 0);

        if (p->state_us >= p->rest_us)
        {
            p->state = STIM_PULSO_A;
            p->state_us = 0;
        }
        break;
    }
    default:
    {
    // ------------------------------------------
    // IDLE
    // ------------------------------------------

        digitalWrite(p->pinA, LOW);
        digitalWrite(p->pinB, LOW);
        dac_write_voltage(p, 0);

        p->state = STIM_IDLE;

        break;
    }
    }
}

//esta es la funcion adaptada a dos piernas que va a llamar a la de arriba
void estimulo_update()
{
    actualizar_pierna(&pierna1);
    actualizar_pierna(&pierna2);
}