/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "AmuConverter.h"
#include "NCSNode.h"
#include "LaneBundle.h"
#include "Intersection.h"
#include "ODNode.h"
#include "Section.h"
#include "Lane.h"
#include "RoadMap.h"
#include "Vehicle.h"
#include "Route.h"
#include "ObjManager.h"
#include "GenerateVehicleIO.h"
#include "VehicleIO.h"
#include "VehicleLaneShifter.h"
#include "GVManager.h"
#include "Random.h"
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <algorithm>
#include <cassert>

using namespace std;

//======================================================================
NCSNode::NCSNode(const string& id,
                const string& type,
                RoadMap* parent)
    : ODNode(id, type, parent)
{
    // by uchida 2016/5/23
    _isCS = true;
    _integratedCharge = 0.0;

//    _lastGenTime = 0;
//    _nodeGvd.clear();
//    _isWaitingToPushVehicle = false;
//    _isOutputGenerateVehicleData = false;
//    _rnd = Random::randomGenerator();
}

//======================================================================
NCSNode::~NCSNode()
{
//    // ODNode特有の変数をここでdelete
//    for (int i=0;
//         i<static_cast<signed int>(_waitingVehicles.size());
//         i++)
//    {
//        delete _waitingVehicles[i];
//    }
//    _waitingVehicles.clear();
//    Random::releaseRandomGenerator(_rnd);
}

// by uchida 2016/5/23
////======================================================================
void NCSNode::sumCharge(double chargingValue)
{
    _instantaneousCharge += chargingValue;
}

////======================================================================
void NCSNode::initCharge()
{
    _instantaneousCharge = 0.0;
}

// by takusagawa 2018/3/26
////======================================================================
double NCSNode::instantaneousCharge()
{
    return _instantaneousCharge;
}

// by uchida 2017/11/24
////======================================================================
void NCSNode::integrated(double instantaneousCharge)
{
    _integratedCharge += instantaneousCharge;
}

////======================================================================
double NCSNode::integratedCharge()
{
    return _integratedCharge;
}

////======================================================================
void NCSNode::initIntegrated()
{
    _integratedCharge = 0.0;
}

////======================================================================
void NCSNode::addEV(Vehicle* vehicle)
{
    waitingLine.push_back(vehicle);
    vehicle->setWaiting(true);
    // 追加直後に充電すべきEVの指定
    vector<Vehicle*>::iterator itr = waitingLine.begin();
    for (int i = 0; i < _capacity; i++)
    {
        if (itr == waitingLine.end())
        {
            break;
        }
        else
        {
            (*itr)->setonCharging(true);
            itr++;
        }
    }
}

////======================================================================
void NCSNode::removeEV()
{
    vector<Vehicle*>::iterator itr = waitingLine.begin();
    (*itr)->setWaiting(false);

    // debug by uchida 2017/6/23
    //cout << "~ " << TimeManager::time() / 1000 << " [s] "
    //    << (*itr)->id() << " : restart from " << _id << endl;


    // if文の追加 by takusagawa 2018/3/26
    if (waitingLine.size() != 0)
    {
      waitingLine.erase(itr);
    }


    // 削除直後にも充電すべきEVの指定
    if (waitingLine.size() != 0)
    {
        itr = waitingLine.begin();
        for (int i = 0; i < _capacity; i++)
        {
            if (itr == waitingLine.end())
            {
                break;
            }
            else
            {
                (*itr)->setonCharging(true);
                itr++;
            }
        }
    }
}

////======================================================================
void NCSNode::setCapacity(int capacity)
{
    _capacity = capacity;
}

////======================================================================
void NCSNode::setOutPower(double outPower)
{
    _outPower = outPower;
}

////======================================================================
double NCSNode::outPower()
{
    return _outPower;
}
