#include <soporte_placa.h>

#define LUZ_ON 0
#define LUZ_OFF 1
#define PULSADOR_ACTIVO 0
#define PULSADOR_NORMAL 1

int main(void){
    SP_init();  //Inicializador
    SP_Pin_setModo(SP_PULSADOR,SP_PIN_ENTRADA_PULLUP); //pulsador
    SP_Pin_setModo(SP_LED,SP_PIN_SALIDA);   //LED
   
    SP_Pin_write(SP_LED,LUZ_OFF);


    for (;;){
        while(SP_Pin_read(SP_PULSADOR) != PULSADOR_ACTIVO);
        SP_Pin_write(SP_LED,LUZ_ON);
        SP_delay(30000);
        SP_Pin_write(SP_LED,LUZ_OFF);
 
    }
    return 0;
}