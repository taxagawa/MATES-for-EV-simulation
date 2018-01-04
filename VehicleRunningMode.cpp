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
#include <sstream>
#include <fstream>
#include <cassert>
#include <climits>
#include <cmath>

using namespace std;

//======================================================================
VehicleRunningMode::VehicleRunningMode(){}

//======================================================================
VehicleRunningMode::~VehicleRunningMode(){}

//======================================================================
double VehicleRunningMode::velocity(ulint modeTime)
{
    itr = _runningMode.find(modeTime);
    if (itr == _runningMode.end())
    {
        assert(modeTime);
    }

    return (*itr).second;
}

//======================================================================
void VehicleRunningMode::print()
{
    for(itr = _runningMode.begin(); itr != _runningMode.end(); ++itr)
    {
        cout << (*itr).first << ", " << (*itr).second << endl;
    }
}

//======================================================================
ulint VehicleRunningMode::cycle()
{
    ritr = _runningMode.rbegin();
    return (*itr).first;
}

//======================================================================
void VehicleRunningMode::createRunningMode()
{

    string fModeFile;
    GVManager::getVariable("RUNNING_MODE_FILE",
                           &fModeFile);
    ifstream inModeFile(fModeFile.c_str(), ios::in);
    if (inModeFile.good())
    {
        string str;
        int index = 0;
        while (inModeFile.good())
        {
            getline(inModeFile, str);
            AmuStringOperator::getAdjustString(&str);
            if (!str.empty())
            {
                vector<string> tokens;
                AmuStringOperator::getTokens(&tokens, str, ',');
                ulint step = AmuConverter::strtoul(tokens[0]);
                double value = AmuConverter::strtod(tokens[1]);
                _runningMode.insert(map<ulint, double>::value_type(step, value));
            }
        }
    }
    print();
}

//======================================================================
bool VehicleRunningMode::isExist()
{
    if (_runningMode.size() != 0) return true;
    return false;
}
