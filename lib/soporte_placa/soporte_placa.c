#include <soporte_placa.h>
#include <stm32l0xx.h> 
#include <stdint.h>

/*------------ IMPLEMENTACIÓN -------------*/

/**
 * @brief Rutina de servicio de interrupción de timer SysTick
 * 
 */
void SysTick_Handler(void);

/* Inicialización general */

void SP_init(void){
    
    SystemCoreClockUpdate();
    
    uint32_t const frecuencia_hertz = SystemCoreClock;
    uint32_t const cuentas_por_milisgundo = frecuencia_hertz/1000;

    
    SysTick_Config(cuentas_por_milisgundo); 
}



/*------------ TEMPORIZACIÓN -------------*/

/**
 * @brief Variable actualizada una vez por milisegundo en el handler
 * de interrupción del timer del sistema (SysTick)
 * 
 */
static uint32_t volatile ticks;

void SP_delay(uint32_t tiempo){
    uint32_t const ticks_inicial = ticks;
    uint32_t tiempo_transcurrido = ticks - ticks_inicial;
    while(tiempo_transcurrido < tiempo){
       
        __WFI();
        tiempo_transcurrido = ticks - ticks_inicial;
    }

}


void SysTick_Handler(void){
    ++ticks;
}

/*------------ GPIO -------------*/

//Definimos una estructura Pin
typedef struct Pin{
    GPIO_TypeDef * puerto;
    int nrPin;
    uint32_t mascaraIOPEN;
}Pin;




static Pin const pines[SP_NUM_PINES] = {
    [SP_PA6]={.puerto=GPIOA,.nrPin=6,  .mascaraIOPEN=RCC_IOPENR_GPIOAEN}, 
    [SP_PA4]={.puerto=GPIOA,.nrPin=4,.mascaraIOPEN=RCC_IOPENR_GPIOAEN},
    
};



/**
 * @brief Obtiene un puntero a Pin a partir de su handle
 * 
 * @param hPin Handle
 * @return Pin const* Puntero al objeto Pin (solo lectura) 
 */
static Pin const * pinDeHandle(SP_HPin hPin){
    return &pines[hPin];
}

typedef enum Velocidad{VEL_BAJA,VEL_MEDIA,VEL_ALTA,VEL_MAX} Velocidad;



/**
 * @brief Configura el modo de salida 
 * 
 * @param pin 
 * @param vel 
 * @param openDrain 
 * @param funcionAlterna 
 */
static void modo_salida(Pin const *pin, Velocidad vel, bool openDrain, bool funcionAlterna){
    

    int const offset_1 = (pin->nrPin % 8)* 2;
    uint32_t const mascara_1 = 0x3; // 2 bits 


//Configura salida como : open drain o push pull-------------------------
if(openDrain){
    //Configura como open drain
    pin->puerto->OTYPER = 1 << pin->nrPin;

}else {
    //configura como push-pull
    pin->puerto->OTYPER = 0 << pin->nrPin;
              
}


//Configura la salida como : Funcion Alternativa o de proposito general --------------------------
if(funcionAlterna){
    //configurado como funcion alternativa
    pin->puerto->MODER = ( pin->puerto->MODER & ~(mascara_1 << offset_1))
                        | ((0b10 & mascara_1)<<offset_1);
}else {
    //Configura el pin como GPIO
    pin->puerto->MODER = ( pin->puerto->MODER & ~(mascara_1 << offset_1))
                        | ((0b01 & mascara_1) << offset_1);
}


//Configura la Velocidad  -------------------------------------------------
if(vel==VEL_BAJA){
    //Velocidad Baja
    pin->puerto->OSPEEDR = ( pin->puerto->OSPEEDR & ~(mascara_1 << offset_1))
                        | ((0b00 & mascara_1)<<offset_1);

}else{

    if(vel==VEL_MEDIA){
        //Velocidad Media
        pin->puerto->OSPEEDR = ( pin->puerto->OSPEEDR & ~(mascara_1 << offset_1))
                                | ((0b01 & mascara_1) << offset_1);

    } else {
            if(vel==VEL_ALTA){
            //Velocidad alta
            pin->puerto->OSPEEDR = ( pin->puerto->OSPEEDR & ~(mascara_1 << offset_1))
                                     | ((0b10 & mascara_1) << offset_1);

            } else {
                    //Velocidad Max
                    pin->puerto->OSPEEDR = ( pin->puerto->OSPEEDR & ~(mascara_1 << offset_1))
                                          | ((0b11 & mascara_1) << offset_1);

            }

    }


}

    // config puerto->MODER
    // config puerto->OTYPER
    // config puerto->OSPEEDR
    
}

typedef enum ModoPull{M_FLOTANTE,M_PULL_UP,M_PULL_DOWN} ModoPull;



/**
 * @brief Configura el Modo de Entrada
 * 
 * @param pin 
 * @param modoPull 
 */
static void modo_entrada(Pin const *pin,ModoPull modoPull){

    int const offset_2 = (pin->nrPin %8)*2;
    uint32_t const mascara_2 = 0x3 ; // 2 bits

    //ENTRADA
    pin->puerto->MODER = ( pin->puerto->MODER & ~(mascara_2 << offset_2))
                        | ((0b00 & mascara_2)<<offset_2);

    //configura entrada en modo : FLOTANTE , PULL UP , PULL DOWN

    if(modoPull==M_FLOTANTE){
        //Modo flotante
        pin->puerto->PUPDR = ( pin->puerto->PUPDR & ~(mascara_2 << offset_2))
                            | ((0b00 & mascara_2) << offset_2);


    }else {
            if(modoPull==M_PULL_UP){
                        // Modo pull up
                        pin->puerto->PUPDR = ( pin->puerto->PUPDR & ~(mascara_2 << offset_2))
                                             | ((0b01 & mascara_2)<<offset_2);

                        //pin->puerto->BSRR = 1 << pin->nrPin;
            }else {
                    //modo pull down
                    pin->puerto->PUPDR = ( pin->puerto->PUPDR & ~(mascara_2 << offset_2))
                                         | ((0b10 & mascara_2) << offset_2);
                    
                 //   pin->puerto->BRR = 1 << pin->nrPin;
            }

    }


    // config puerto->MODER
    // config puerto->PUPDR
    // config ODR (con BSRR o BRR)
//        pin->puerto->BSRR = 1 << pin->nrPin; // NO necesita deshabilitar interrupciones (Pone en 1)
//        pin->puerto->BRR = 1 << pin->nrPin; 
}



void SP_Pin_setModo(SP_HPin hPin,SP_Pin_Modo modo){
    if(hPin >= SP_NUM_PINES) return; 
    Pin const *pin = pinDeHandle(hPin); //Recuperamos el puntero
    __disable_irq();// Deshabita las interrupciones
    RCC->IOPENR |= pin->mascaraIOPEN;

    switch (modo)
    {
    case SP_PIN_ENTRADA:
        modo_entrada(pin,M_FLOTANTE);
    break;case SP_PIN_ENTRADA_PULLUP:
        // configurar
        modo_entrada(pin,M_PULL_UP);
    break;case SP_PIN_ENTRADA_PULLDN:
        // configurar
        modo_entrada(pin,M_PULL_DOWN);
    break;case SP_PIN_SALIDA:
        // configurar
        modo_salida(pin,VEL_BAJA,false,false);
    break;case SP_PIN_SALIDA_OPEN_DRAIN:
        // configurar
        modo_salida(pin,VEL_BAJA,true,false);
    break;default:
    break;
    }
    __enable_irq();
}



bool SP_Pin_read(SP_HPin hPin){
   
    Pin const *pin = pinDeHandle(hPin);// Recuperamos el puntero
    
    return  (pin->puerto->IDR & (1 << pin->nrPin)); 
}

void SP_Pin_write(SP_HPin hPin, bool valor){
   Pin const *pin = pinDeHandle(hPin);// Recuperamos el puntero

    if(valor){
        pin->puerto->BSRR =(1 << pin->nrPin); 
    }else{
        pin->puerto->BRR = (1 << pin->nrPin); 
    }
   
}

