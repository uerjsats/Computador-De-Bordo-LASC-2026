
#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

bool mpuInit();
void mpuUpdate(float &gyroX,  float &gyroY,  float &gyroZ,  float &accelX,  float &accelY,  float &accelZ);

#endif
