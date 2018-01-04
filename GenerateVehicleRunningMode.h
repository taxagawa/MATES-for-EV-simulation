/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __GENERATE_VEHICLE_RUNNING_MODE_H__
#define __GENERATE_VEHICLE_RUNNING_MODE_H__

#include "VehicleFamily.h"
#include "TimeManager.h"
#include <map>
#include <string>
#include <vector>

class RoadMap;
class Intersection;
class ODNode;
class Section;
class Vehicle;
class VehicleRunningMode;
class RandomGenerator;

/// 車両の走行モードを扱うクラス
/**
 * @ingroup Running
 */
class GenerateVehicleRunningMode
{
public:

//private:
    GenerateVehicleRunningMode();
    ~GenerateVehicleRunningMode();

    void runningModeGenerate();
    VehicleRunningMode* runningMode();

private:
    VehicleRunningMode* _currentRunningMode;
};

#endif //__GENERATE_VEHICLE_RUNNING_MODE_H__
