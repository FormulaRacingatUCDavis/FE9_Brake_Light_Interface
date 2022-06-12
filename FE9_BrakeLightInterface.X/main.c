/*
 * FE9 Brake Light Interface
 * 
 * @Description
 *  Controls brake light and buzzer
 *   - mechanical braking
 *   - regen braking
 *   - HV to DRIVE transition
 */

#include "mcc_generated_files/mcc.h"

typedef enum {
    VEHICLE_STATE = 0x0c0,
    BSPD_FLAGS = 0x0c1,
    DRIVER_SWITCHES = 0x0d0,
    TORQUE_REQUEST_COMMAND = 0x766,
    BRAKE_COMMAND = 0x767,
    BMS_STATUS_MSG = 0x380,
    PEI_CURRENT = 0x387,
    BMS_VOLTAGES = 0x388,
    BMS_TEMPERATURES = 0x389,
    MC_ESTOP = 0x366,
    MC_DEBUG = 0x466,
    MC_PDO_SEND = 0x566,
    MC_PDO_ACK = 0x666
} CAN_ID;

typedef enum {
    LV,
    PRECHARGING,
    HV_ENABLED,
    DRIVE,
    FAULT
} state_t;

typedef enum {
    NONE,
    DRIVE_REQUEST_FROM_LV,
    CONSERVATIVE_TIMER_MAXED,
    BRAKE_NOT_PRESSED,
    HV_DISABLED_WHILE_DRIVING,
    SENSOR_DISCREPANCY,
    BRAKE_IMPLAUSIBLE,
    ESTOP
} error_t;

// Initial FSM state
volatile state_t state = LV;
volatile state_t prev_state = LV;
volatile error_t error = NONE;

/* Buzzer timer ISR; just turn off buzzer after 2s
 * 
 * EV.10.5.2: Sounded continuously for minimum 1 second
 * and a maximum of 3 seconds [we use 2 seconds]
 */
void buzzer_timer_ISR() {
    TMR0_StopTimer();
    Buzzer_SetLow();
}

/*
                         Main application
 */
void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();
    
    TMR0_SetInterruptHandler(buzzer_timer_ISR);
    
    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global Interrupts
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();
    
    uint8_t mech_braking = 0; 
    uint8_t regen_braking = 0; 
    
    uCAN_MSG rxMsg;
    while (1) {
        if (CAN_receive(&rxMsg)) {
            if (rxMsg.frame.id == TORQUE_REQUEST_COMMAND) {
                /********** BUZZER **********/
                prev_state = state;
                state = rxMsg.frame.data4;
                // transition from HV_ENABLED to DRIVE
                if (prev_state == HV_ENABLED && state == DRIVE) {
                    Buzzer_SetHigh();
                    TMR0_StartTimer();
                }
                /********** MECH BRAKING **********/
                if (rxMsg.frame.data5) {
                    mech_braking = 1; 
                } else {
                    mech_braking = 0;
                }
            } else if (rxMsg.frame.id == PEI_CURRENT) {
                /********** REGEN BRAKING **********/
                if ((int8_t)(rxMsg.frame.data0) < -8) {
                    regen_braking = 1; 
                } else {
                    regen_braking = 0; 
                }
            }
        }
        
        if(mech_braking || regen_braking) {
            BrakeLight_SetHigh(); 
        } else {
            BrakeLight_SetLow(); 
        }
        
    }
    
}
/**
 End of File
*/