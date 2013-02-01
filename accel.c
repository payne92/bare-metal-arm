
// accel.c -- Accelerometer support
//
//  Copyright (c) 2012-2013 Andrew Payne <andy@payne.org>
//

// Based on demo example from Freescale

#include <freedom.h>
#include "common.h"

#define MMA8451_I2C_ADDRESS (0x1d<<1)
#define I2C_READ 1
#define I2C_WRITE 0

#define I2C0_B  I2C0_BASE_PTR

// ---------------------------------------------------------------------------
// I2C bus control functions
//
inline void i2c_set_tx(I2C_MemMapPtr p)     {p->C1 |= I2C_C1_TX_MASK;}
inline void i2c_set_rx(I2C_MemMapPtr p)     {p->C1 &= ~I2C_C1_TX_MASK;}
inline void i2c_set_slave(I2C_MemMapPtr p)  {p->C1 &= ~I2C_C1_MST_MASK;}
inline void i2c_set_master(I2C_MemMapPtr p) {p->C1 |=  I2C_C1_MST_MASK;}
inline void i2c_give_nack(I2C_MemMapPtr p)  {p->C1 |= I2C_C1_TXAK_MASK;}
inline void i2c_give_ack(I2C_MemMapPtr p)   {p->C1 &= ~I2C_C1_TXAK_MASK;}
inline void i2c_repeated_start(I2C_MemMapPtr p){p->C1     |= I2C_C1_RSTA_MASK;}
inline uint8_t i2c_read(I2C_MemMapPtr p)    {return p->D;}

inline void i2c_start(I2C_MemMapPtr p)
{
    i2c_set_master(p);
    i2c_set_tx(p);
}

inline void i2c_stop(I2C_MemMapPtr p)
{
    i2c_set_slave(p);
    i2c_set_rx(p);
}

inline void i2c_wait(I2C_MemMapPtr p)
{
    // Spin wait for the interrupt flag to be set
    while((p->S & I2C_S_IICIF_MASK) == 0)
        ;

    p->S |= I2C_S_IICIF_MASK;           // Clear flag
}

inline int i2c_write(I2C_MemMapPtr p, uint8_t data)
{
    // Send data, wait, and return ACK status
    p->D = data;
    i2c_wait(p);
    return ((p->S & I2C_S_RXAK_MASK) == 0);
}

inline void i2c_init(I2C_MemMapPtr p)
{
    // Enable clocks
    SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;      
    SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;
    
    // Configure GPIO for I2C
    PORTE_PCR24 = PORT_PCR_MUX(5);
    PORTE_PCR25 = PORT_PCR_MUX(5);
    
    p->F  = 0x14;                   // Baudrate settings:  ICR=0x14, MULT=0
    p->C1 = I2C_C1_IICEN_MASK;      // Enable:  IICEN=1
}

// ---------------------------------------------------------------------------
// MMA8451 control functions
//

// this delay is very important, it may cause w-r operation failure.
static void pause(void)
{
    int n;
    for(n=0; n<40; n++)
        asm("nop");
}

uint8_t mma8451_read(uint8_t addr)
{
    pause();
    i2c_start(I2C0_B);
    i2c_write(I2C0_B, MMA8451_I2C_ADDRESS | I2C_WRITE);
    i2c_write(I2C0_B, addr);
    i2c_repeated_start(I2C0_B);
    i2c_write(I2C0_B, MMA8451_I2C_ADDRESS | I2C_READ);
    i2c_set_rx(I2C0_B);
    i2c_give_nack(I2C0_B);
    i2c_read(I2C0_B);
    i2c_wait(I2C0_B);
    i2c_stop(I2C0_B);
    return i2c_read(I2C0_B);
}

void mma8451_write(uint8_t addr, uint8_t data)
{
    pause();
    i2c_start(I2C0_B);
    i2c_write(I2C0_B, MMA8451_I2C_ADDRESS | I2C_WRITE);
    i2c_write(I2C0_B, addr);
    i2c_write(I2C0_B, data);
    i2c_stop(I2C0_B);
}

#define CTRL_REG1 (0x2a)
void accel_init(void)
{
    uint8_t tmp;

    i2c_init(I2C0_B);
    tmp = mma8451_read(CTRL_REG1);
    mma8451_write(CTRL_REG1, tmp | 0x01);       // ACTIVE = 1
}

// Read a signed 14-bit value from (reg, reg+1)
int16_t _read_reg14(int reg)
{
    return (int16_t)((mma8451_read(reg) << 8) | mma8451_read(reg+1)) >> 2;
}

// Read acceleration values for each axis
int16_t accel_x(void) {return _read_reg14(0x01);}
int16_t accel_y(void) {return _read_reg14(0x03);}
int16_t accel_z(void) {return _read_reg14(0x05);}








