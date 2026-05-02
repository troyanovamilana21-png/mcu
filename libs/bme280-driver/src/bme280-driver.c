#include "bme280-driver.h"
#include "bme280-regs.h"
#include <stdint.h>
#include <stdio.h>

typedef struct
{
    bme280_i2c_read i2c_read;
    bme280_i2c_write i2c_write;
} bme280_ctx_t;

static struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib;

static bme280_ctx_t bme280_ctx = {};
static int32_t t_fine;

static void bme280_read_calibration_data(void)
{
    uint8_t buffer[24] = {0};
    
    bme280_read_regs(0x88, buffer, 24);
    
    bme280_calib.dig_T1 = (uint16_t)(buffer[0] | (buffer[1] << 8));
    bme280_calib.dig_T2 = (int16_t)(buffer[2] | (buffer[3] << 8));
    bme280_calib.dig_T3 = (int16_t)(buffer[4] | (buffer[5] << 8));
    
    bme280_calib.dig_P1 = (uint16_t)(buffer[6] | (buffer[7] << 8));
    bme280_calib.dig_P2 = (int16_t)(buffer[8] | (buffer[9] << 8));
    bme280_calib.dig_P3 = (int16_t)(buffer[10] | (buffer[11] << 8));
    bme280_calib.dig_P4 = (int16_t)(buffer[12] | (buffer[13] << 8));
    bme280_calib.dig_P5 = (int16_t)(buffer[14] | (buffer[15] << 8));
    bme280_calib.dig_P6 = (int16_t)(buffer[16] | (buffer[17] << 8));
    bme280_calib.dig_P7 = (int16_t)(buffer[18] | (buffer[19] << 8));
    bme280_calib.dig_P8 = (int16_t)(buffer[20] | (buffer[21] << 8));
    bme280_calib.dig_P9 = (int16_t)(buffer[22] | (buffer[23] << 8));
    
    uint8_t h_buffer[7] = {0};
    bme280_read_regs(0xA1, &bme280_calib.dig_H1, 1);
    bme280_read_regs(0xE1, h_buffer, 7);
    
    bme280_calib.dig_H2 = (int16_t)(h_buffer[0] | (h_buffer[1] << 8));
    bme280_calib.dig_H3 = h_buffer[2];
    bme280_calib.dig_H4 = (int16_t)((h_buffer[3] << 4) | (h_buffer[4] & 0x0F));
    bme280_calib.dig_H5 = (int16_t)((h_buffer[4] >> 4) | (h_buffer[5] << 4));
    bme280_calib.dig_H6 = (int8_t)h_buffer[6];
    
    printf("BME280 calibration data loaded\n");
}

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write)
{
    bme280_ctx.i2c_read = i2c_read;
    bme280_ctx.i2c_write = i2c_write;
    
    uint8_t id_reg_buf[1] = {0};
    bme280_read_regs(BME280_REG_id, id_reg_buf, sizeof(id_reg_buf));
    
    if (id_reg_buf[0] == 0x60)
    {
        printf("BME280 detected: chip ID = 0x60\n");
    }
    else
    {
        printf("ERROR: BME280 not found! ID = 0x%02X (expected 0x60)\n", id_reg_buf[0]);
        return;
    }
    
    
    bme280_read_calibration_data();
    
    uint8_t ctrl_hum_reg_value = 0;
    ctrl_hum_reg_value |= (0b001 << 0);
    bme280_write_reg(BME280_REG_ctrl_hum, ctrl_hum_reg_value);
    
    uint8_t config_reg_value = 0;
    config_reg_value |= (0b0 << 0);
    config_reg_value |= (0b000 << 2);
    config_reg_value |= (0b001 << 5);
    bme280_write_reg(BME280_REG_config, config_reg_value);
    
    uint8_t ctrl_meas_reg_value = 0;
    ctrl_meas_reg_value |= (0b11 << 0);
    ctrl_meas_reg_value |= (0b001 << 2);
    ctrl_meas_reg_value |= (0b001 << 5);
    bme280_write_reg(BME280_REG_ctrl_meas, ctrl_meas_reg_value);
    
    printf("BME280 initialized and configured\n");
}

void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length)
{
    uint8_t data[1] = {start_reg_address};
    bme280_ctx.i2c_write(data, sizeof(data));
    bme280_ctx.i2c_read(buffer, length);
}

void bme280_write_reg(uint8_t reg_address, uint8_t value)
{
    uint8_t data[2] = {reg_address, value};
    bme280_ctx.i2c_write(data, sizeof(data));
}

uint32_t bme280_read_temp_raw_20bit(void)
{
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_temp_msb, read, 3);
    uint32_t value = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
    return value;
}

uint16_t bme280_read_temp_raw(void)
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_temp_lsb, read, 2);
    return ((uint16_t)read[1] << 8) | read[0];
}

uint16_t bme280_read_press_raw(void)
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_press_lsb, read, 2);
    return ((uint16_t)read[1] << 8) | read[0];
}

uint16_t bme280_read_hum_raw(void)
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_hum_lsb, read, 2);
    return ((uint16_t)read[1] << 8) | read[0];
}

float bme280_read_temp_celsius(void)
{
    uint32_t adc_T = bme280_read_temp_raw_20bit();
    
    int32_t var1, var2;
    var1 = (int32_t)((adc_T >> 3) - ((int32_t)bme280_calib.dig_T1 << 1));
    var1 = (var1 * (int32_t)bme280_calib.dig_T2) >> 11;
    
    var2 = (int32_t)((adc_T >> 4) - (int32_t)bme280_calib.dig_T1);
    var2 = ((var2 * var2) >> 12) * (int32_t)bme280_calib.dig_T3;
    var2 = var2 >> 14;
    
    t_fine = var1 + var2;
    
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}


float bme280_read_press_pascal(void)
{
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_press_msb, read, 3);
    uint32_t adc_P = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
    
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bme280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bme280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bme280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bme280_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)bme280_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bme280_calib.dig_P1) >> 33;
    
    if (var1 == 0) return 0;
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bme280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bme280_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bme280_calib.dig_P7) << 4);
    
    return (float)p / 256.0f / 100.0f;
}

float bme280_read_hum_percent(void)
{
    
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_hum_msb, read, 2);
    uint16_t adc_H = ((uint16_t)read[0] << 8) | read[1];
    
    int32_t var1;
    var1 = (t_fine - ((int32_t)76800));
    
    var1 = (((((adc_H << 14) - (((int32_t)bme280_calib.dig_H4) << 20) -
              (((int32_t)bme280_calib.dig_H5) * var1)) + ((int32_t)16384)) >> 15) *
              (((((((var1 * (int32_t)bme280_calib.dig_H6) >> 10) *
              (((var1 * (int32_t)bme280_calib.dig_H3) >> 11) + ((int32_t)32768))) >> 10) +
              ((int32_t)2097152)) * (int32_t)bme280_calib.dig_H2 + 8192) >> 14));
    
    var1 = (var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * (int32_t)bme280_calib.dig_H1) >> 4));
    var1 = (var1 < 0) ? 0 : var1;
    var1 = (var1 > 419430400) ? 419430400 : var1;
    
    float h = (var1 >> 12) / 1024.0f;
    return (h > 100.0f) ? 100.0f : h;
}
