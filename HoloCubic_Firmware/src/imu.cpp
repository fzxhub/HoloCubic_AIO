#include "imu.h"
#include "common.h"


IMU::IMU()
{
    action_info.isValid = true;
    action_info.active = UNKNOWN;
    action_info.long_time = true;
}

void IMU::init()
{
    Wire.begin(IMU_I2C_SDA, IMU_I2C_SCL);
    Wire.setClock(400000);
    unsigned long timeout = 5000;
    unsigned long preMillis = millis();
    mpu = MPU6050(0x68);
    while (!mpu.testConnection() && !doDelayMillisTime(timeout, &preMillis, 0))
        ;
    mpu.initialize();
    // supply your own gyro offsets here, scaled for min sensitivity
    // mpu.setXGyroOffset(220);
    // mpu.setYGyroOffset(76);
    // mpu.setZGyroOffset(-85);
    // mpu.setXAccelOffset(-1788);
    // mpu.setYAccelOffset(1788);
    // mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // 7次循环自动校正
    mpu.CalibrateAccel(7);
    mpu.CalibrateGyro(7);
    mpu.PrintActiveOffsets();
}

Imu_Action *IMU::update(int interval)
{
    mpu.getMotion6(&(action_info.ax), &(action_info.ay), &(action_info.az),
                   &(action_info.gx), &(action_info.gy), &(action_info.gz));

    if (millis() - last_update_time > interval)
    {
        if (action_info.ay > 4000 && !action_info.isValid)
        {
            encoder_diff--;
            action_info.isValid = 1;
            action_info.active = TURN_LEFT;
        }
        else if (action_info.ay < -4000)
        {
            encoder_diff++;
            action_info.isValid = 1;
            action_info.active = TURN_RIGHT;
        }
        else
        {
            action_info.isValid = 0;
        }

        if (action_info.ax > 5000 && !action_info.isValid)
        {
            delay(300);
            mpu.getMotion6(&(action_info.ax), &(action_info.ay), &(action_info.az),
                           &(action_info.gx), &(action_info.gy), &(action_info.gz));
            if (action_info.ax > 5000)
            {
                action_info.isValid = 1;
                action_info.active = GO_FORWORD;
                encoder_state = LV_INDEV_STATE_PR;
            }
        }
        else if (action_info.ax < -5000 && !action_info.isValid)
        {
            delay(300);
            mpu.getMotion6(&(action_info.ax), &(action_info.ay), &(action_info.az),
                           &(action_info.gx), &(action_info.gy), &(action_info.gz));
            if (action_info.ax < -5000)
            {
                action_info.isValid = 1;
                action_info.active = RETURN;
                encoder_state = LV_INDEV_STATE_REL;
            }
        }
        else
        {
            action_info.isValid = 0;
        }
        last_update_time = millis();
    }
    return &action_info;
}
