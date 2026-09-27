#include "icm20948.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"     
#include "pico/util/queue.h"  
#include "pico/cyw43_arch.h"

#include "ff.h"
#include "sd_card.h"
#include "f_util.h"
#include "hw_config.h"

typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    uint32_t t_us;   // timestamp (lower 32 bits is fine for short runs)
} Sample;

static queue_t sample_q;
static FATFS fs;

static void core1_reader(void)
{
    absolute_time_t target = get_absolute_time();
    while (1) {
        IMU_ST_SENSOR_DATA stGyroRawData, stAccelRawData;

        target = delayed_by_us(target, 2000);

        uint32_t t_now = (uint32_t)time_us_64();

        imuDataAccGyrGet(&stGyroRawData, &stAccelRawData);

        Sample s = { stAccelRawData.s16X, stAccelRawData.s16Y, stAccelRawData.s16Z,
                    stGyroRawData.s16X, stGyroRawData.s16Y, stGyroRawData.s16Z,
                    t_now };
        queue_add_blocking(&sample_q, &s);

        sleep_until(target);
    }
}

void indicate_error(void) {
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(50);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(50);
    }
}

int main(void)
{
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Failed to initialise cyw43\n");
        return 1;
    }

    #define BUFFER_SAMPLES 256

    Sample sd_buffer[BUFFER_SAMPLES]; 
    uint16_t sample_count = 0;        
    UINT bytes_written;                
    FIL file;    
    FRESULT fr;
    bool led_state = false;

    // Initialize SD card driver
    if (!sd_init_driver()) {
        printf("Error: sd_init_driver() failed\n");
        indicate_error();
    }

    sd_card_t *pSD = sd_get_by_num(0);
    if (!pSD) {
        printf("Error: No SD card configuration found\n");
        indicate_error();
    }

    const char *drive = sd_get_drive_prefix(pSD);
    if (!drive) {
        drive = "0:";
    }

    fr = f_mount(&fs, drive, 1);
    if (fr != FR_OK) {
        printf("Error mounting SD card: %d (%s)\n", fr, FRESULT_str(fr));
        indicate_error();
    }

    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/imu_data.bin", drive);

    fr = f_open(&file, filepath, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("Error opening file %s: %d (%s)\n", filepath, fr, FRESULT_str(fr));
        indicate_error();
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    IMU_EN_SENSOR_TYPE type;
    imuInit(&type);

    if (IMU_EN_SENSOR_TYPE_ICM20948 == type) {
        printf("Motion sensor is ICM-20948 (multicore)\n");
    } else {
        printf("Motion sensor NULL\n");
        indicate_error();
    }

    // Queue can hold up to N samples; adjust for your bandwidth
    queue_init(&sample_q, sizeof(Sample), 512);

    // Launch core1 reader
    multicore_launch_core1(core1_reader);

    uint32_t t_prev = (uint32_t)time_us_64();

    while (1) {
        Sample s;
        queue_remove_blocking(&sample_q, &s);

        uint32_t t_now = (uint32_t)time_us_64();
        uint32_t dt_us = (t_now - t_prev);
        t_prev = t_now;
        float hz = (dt_us > 0) ? (1000000.0f / (float)dt_us) : 0.0f;

        sd_buffer[sample_count] = s;
        sample_count += 1;

        if (sample_count == BUFFER_SAMPLES) { 
            fr = f_write(&file, sd_buffer, sizeof(sd_buffer), &bytes_written);
            if (fr != FR_OK) {
                printf("Error writing to SD: %d\n", fr);
            }
            f_sync(&file);

            led_state = !led_state;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);

            sample_count = 0;

            printf("ACC: X=%d Y=%d Z=%d | GYRO: X=%d Y=%d Z=%d | RX Rate: %.1f Hz\r\n",
                s.ax, s.ay, s.az, s.gx, s.gy, s.gz, hz);
        }
    }

    return 0;
}