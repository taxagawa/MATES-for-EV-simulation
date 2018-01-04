/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "VehicleRunningMode.h"
#include "RoadMap.h"
#include "Intersection.h"
#include "ODNode.h"
#include "Section.h"
#include "Lane.h"
#include "Vehicle.h"
#include "Random.h"
#include "ObjManager.h"
#include "GVManager.h"
#include "VehicleFamilyManager.h"
#include "VehicleFamilyIO.h"
#include "VehicleFamily.h"
#include "AmuConverter.h"
#include "AmuStringOperator.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <climits>
#include <cmath>

using namespace std;

//======================================================================
GenerateVehicleRunningMode::GenerateVehicleRunningMode(){}

//======================================================================
GenerateVehicleRunningMode::~GenerateVehicleRunningMode(){}

//======================================================================
void GenerateVehicleRunningMode::runningModeGenerate()
{
    _currentRunningMode = new VehicleRunningMode();

    _currentRunningMode->createRunningMode();
}

//======================================================================
VehicleRunningMode* GenerateVehicleRunningMode::runningMode()
{
    VehicleRunningMode* tmp = _currentRunningMode;
    _currentRunningMode = 0;
    return tmp;
}
