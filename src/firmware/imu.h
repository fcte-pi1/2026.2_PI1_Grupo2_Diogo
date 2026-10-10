#pragma once

bool initIMU();
void updateIMU(float dt);
float getYawAngle();
float getYawRate();
void resetYawAngle();