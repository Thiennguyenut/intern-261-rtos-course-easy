/* Week 2 / Ex1 — reads MPU6050 over I2C1 every SAMPLE_PERIOD_MS, pushes a
 * raw_sample_t into queue_raw.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

#define MPU6050_I2C_ADDR        ((uint16_t)(0x68U << 1))
#define MPU6050_REG_WHO_AM_I    (0x75U)
#define MPU6050_REG_PWR_MGMT_1  (0x6BU)
#define MPU6050_REG_ACCEL_XOUT_H (0x3BU)

extern I2C_HandleTypeDef hi2c1;

static volatile uint32_t sensor_i2c_errors = 0U;
static volatile uint32_t sensor_raw_dropped = 0U;

void sensor_task(void *argument)
{
    (void)argument;
    uint32_t seq = 0;

    /* TODO 1: HAL_I2C init / verify MPU6050 WHO_AM_I. Do this once here, not
     *         inside the loop. */
    uint8_t who_am_i = 0U;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, MPU6050_REG_WHO_AM_I,
    I2C_MEMADD_SIZE_8BIT, &who_am_i, 1U, 100U
    );

    configASSERT(status == HAL_OK);

    configASSERT(who_am_i == 0x68U || who_am_i == 0x70U);

    status = HAL_I2C_Mem_Write(&hi2c1, MPU6050_I2C_ADDR, MPU6050_REG_PWR_MGMT_1,
    I2C_MEMADD_SIZE_8BIT, (uint8_t[]){0x00U}, 1U, 100U
    );

    configASSERT(status == HAL_OK); 

    vTaskDelay(pdMS_TO_TICKS(100U));

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        raw_sample_t s;
        s.seq = seq++;

        /* TODO 2: HAL_I2C_Mem_Read the accel registers into s.ax/ay/az. */
        uint8_t accel_bytes[6];

        status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, MPU6050_REG_ACCEL_XOUT_H,
        I2C_MEMADD_SIZE_8BIT, accel_bytes,
        sizeof(accel_bytes), 5U
        );

        if (status == HAL_OK) {
            s.ax = (int16_t)(((uint16_t)accel_bytes[0] << 8) | accel_bytes[1]);
            s.ay = (int16_t)(((uint16_t)accel_bytes[2] << 8) | accel_bytes[3]);
            s.az = (int16_t)(((uint16_t)accel_bytes[4] << 8) | accel_bytes[5]);

        /* TODO 3: xQueueSend(queue_raw, &s, <timeout>) — pick and justify the
         *         timeout in PREDICTION.md question 3/4. What happens to this
         *         sample if the queue is full and the timeout expires? */

        BaseType_t send_result = xQueueSend(queue_raw, &s, 0U);

            if (send_result != pdPASS) {
            ++sensor_raw_dropped;
            }
        } 
        else {
            ++sensor_i2c_errors;
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
