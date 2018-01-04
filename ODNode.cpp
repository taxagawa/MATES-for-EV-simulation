/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "ODNode.h"
#include "LaneBundle.h"
#include "Intersection.h"
#include "Section.h"
#include "Lane.h"
#include "RoadMap.h"
#include "Vehicle.h"
#include "VehicleEV.h"
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
ODNode::ODNode(const string& id,
               const string& type,
               RoadMap* parent)
    : Intersection(id, type, parent)
{
    _lastGenTime = 0;
    _nodeGvd.clear();
    _isWaitingToPushVehicle = false;
    _isOutputGenerateVehicleData = false;
    _rnd = Random::randomGenerator();
}

//======================================================================
ODNode::~ODNode()
{
    // ODNode特有の変数をここでdelete
    for (int i=0;
         i<static_cast<signed int>(_waitingVehicles.size());
         i++)
    {
        delete _waitingVehicles[i];
    }
    _waitingVehicles.clear();
    Random::releaseRandomGenerator(_rnd);
}

//======================================================================
bool ODNode::hasWaitingVehicles() const
{
    if (!_waitingVehicles.empty())
    {
        return true;
    }
    else
    {
        return false;
    }
}

//======================================================================
void ODNode::addWaitingVehicle(Vehicle* vehicle)
{
    assert(vehicle);

#ifdef _OPENMP
#pragma omp critical (addWaitingVehicle)
    {
#endif //_OPENMP

    _waitingVehicles.push_back(vehicle);

#ifdef _OPENMP
    }
#endif //_OPENMP
}

//======================================================================
void ODNode::addWaitingVehicleFront(Vehicle* vehicle)
{
    assert(vehicle);

#ifdef _OPENMP
#pragma omp critical (addWaitingVehicle)
    {
#endif //_OPENMP

        _waitingVehicles.push_front(vehicle);

#ifdef _OPENMP
    }
#endif //_OPENMP
}

//======================================================================
void ODNode::pushVehicleToReal(RoadMap* roadMap)
{
    assert(!(_waitingVehicles.empty()));

    Section* section = _incSections[0];
    vector<Lane*> lanes = section->lanesFrom(this);

    // 他車両の車線変更先となっている車線を取得する
    // 発生点付近で車線変更が行われている場合は車両は発生できない
    vector<Lane*> shiftTargetLanes;
    list<Vehicle*>* notifyVehicles = section->watchedVehicles();

    list<Vehicle*>::iterator itv;
    for (itv = notifyVehicles->begin();
         itv != notifyVehicles->end();
         itv++)
    {
        VehicleLaneShifter anotherLaneShifter = (*itv)->laneShifter();
        if (anotherLaneShifter.isActive() &&
            anotherLaneShifter.lengthTo() < 25)
        {
            shiftTargetLanes.push_back(const_cast<Lane*>(anotherLaneShifter.laneTo()));
        }
    }

    // 十分な空きのあるレーンを取得
    deque<Lane*> possibleLanes;
    for (int i=0;
         i<static_cast<signed int>(lanes.size());
         i++)
    {

        vector<Lane*>::iterator itl = find(shiftTargetLanes.begin(),
                                           shiftTargetLanes.end(),
                                           lanes[i]);
        if (itl!=shiftTargetLanes.end()) continue;

        if (lanes[i]->tailAgent()==NULL)
        {
            possibleLanes.push_back(lanes[i]);
        }
#ifdef GENERATE_VELOCITY_0
        else if (lanes[i]->tailAgent()->length()
                 - lanes[i]->tailAgent()->bodyLength()/2
                 - _waitingVehicles.front()->bodyLength()
                 > 1.0)
        {
            possibleLanes.push_back(lanes[i]);
        }
#else // GENERATE_VELOCITY_0 not defined
        else if (lanes[i]->tailAgent()->length()
                 - lanes[i]->tailAgent()->bodyLength()/2
                 - _waitingVehicles.front()->bodyLength()
                 > 1.38+lanes[i]->speedLimit()/60.0/60.0*740)
        {
            possibleLanes.push_back(lanes[i]);
        }
#endif // GENERATE_VELOCITY_0
    }

    vector<Vehicle*> skipVehicles;
    if (!possibleLanes.empty())
    {
        // 空きレーンがある場合に限り車両を発生させる
        vector<int> order = Random::randomOrder(_rnd, possibleLanes.size());

        while (!_waitingVehicles.empty() && !order.empty())
        {
            Vehicle* tmpVehicle = _waitingVehicles.front();
            _waitingVehicles.pop_front();

            Lane* generateLane = NULL;
            generateLane = possibleLanes[order.back()];
            order.pop_back();

            if (generateLane == NULL)
            {
                skipVehicles.push_back(tmpVehicle);
            }
            else
            {

                // 経路選択は車両登場時に実行する
                tmpVehicle->reroute(section, this);

                // by uchida 2016/5/30
                // CS経由選択が必要なら
                if (tmpVehicle->threshold() && tmpVehicle->type() >= 80)
                {
                    // by uchida 2017/2/8
                    // _isChargeを利用して充電走行開始地点を取得する
                    tmpVehicle->onChargeflag();

                    // 2
                    //tmpVehicle->CSreroute(section, this,
                    //    tmpVehicle->_searchCSCost(roadMap, section, this));

                    // 3
                    tmpVehicle->CSreroute(section, this,
                        tmpVehicle->_searchCSSumCost(roadMap, section, this));
                }

                // 登場時刻の記録
                tmpVehicle->setStartTime(TimeManager::time());

                // 道路上に登場
                tmpVehicle->addToSection(roadMap, section, generateLane,
                                         tmpVehicle->bodyLength()/2);
                const_cast<Route*>(tmpVehicle->route())->setLastPassedIntersection(this);

                bool result = ObjManager::addVehicleToReal(tmpVehicle);
                assert(result);

                // 作成した車両属性情報を出力する
                /*
                 * addVehicleToRealの後でなければIDが決まらないため，
                 * ここで出力する
                 */
                VehicleIO::instance().writeVehicleStaticData(tmpVehicle);

                if (GVManager::getFlag("FLAG_OUTPUT_GEN_COUNTER"))
                {
                    GeneratedVehicleData gvd;
                    gvd.vehicle = tmpVehicle;
                    gvd.lane = generateLane;
                    gvd.headway = TimeManager::time()-_lastInflowTime;
                    gvd.genTime = tmpVehicle->genTime();
                    gvd.genInterval = gvd.genTime-_lastGenTime;
                    _nodeGvd.push_back(gvd);
                }
                _lastInflowTime = TimeManager::time();
                _lastGenTime    = tmpVehicle->genTime();
            }
        }
    }

    for (unsigned int i = 0; i < skipVehicles.size(); i++)
    {
        _waitingVehicles.push_front(skipVehicles[i]);
    }

    // 車両発生情報の出力
    if (GVManager::getFlag("FLAG_OUTPUT_GEN_COUNTER")
        && _isOutputGenerateVehicleData
        && !_nodeGvd.empty())
    {
        GenerateVehicleIO::writeGeneratedVehicleData(this, &_nodeGvd);
        _nodeGvd.clear();
    }
}

//======================================================================
bool ODNode::isWaitingToPushVehicle() const
{
    return _isWaitingToPushVehicle;
}

//======================================================================
void ODNode::setWaitingToPushVehicle(bool flag)
{
#ifdef _OPENMP
#pragma omp critical (setWaitingToPushVehicle)
    {
#endif //_OPENMP
        _isWaitingToPushVehicle = flag;
#ifdef _OPENMP
    }
#endif //_OPENMP
}

//======================================================================
void ODNode::deleteAgent()
{
    ITRMAPLAN itl;

    for (itl=_lanes.begin(); itl!=_lanes.end(); itl++)
    {
        vector<RoadOccupant*>* agentsOfLane = (*itl).second->agents();
        if (!agentsOfLane->empty())
        {
            (*itl).second->setUsed();
        }
        for (unsigned int i=0; i<agentsOfLane->size(); i++)
        {
            if (dynamic_cast<Vehicle*>((*agentsOfLane)[i]))
            {

                // debug by uchida 2017/6/27
                // if (dynamic_cast<Vehicle*>((*agentsOfLane)[i])->type() >= 80)
                // {
                //     cout << "initial_SoC " << dynamic_cast<VehicleEV*>((*agentsOfLane)[i])->initialSoC()
                //          << " terminal_SoC " << dynamic_cast<Vehicle*>((*agentsOfLane)[i])->SOC()
                //          << " tripLength " << dynamic_cast<Vehicle*>((*agentsOfLane)[i])->tripLength()
                //          << " id " << dynamic_cast<Vehicle*>((*agentsOfLane)[i])->id() << endl;
                // }

                if (GVManager::getFlag("FLAG_OUTPUT_TRIP_INFO"))
                {
                    // 消去する前にトリップ長の出力
                    VehicleIO::instance().writeVehicleDistanceData
                        (dynamic_cast<Vehicle*>((*agentsOfLane)[i]));
                }

                // by uchida 2017/2/27
                //これまでの経路を取得する
                if (GVManager::getFlag("FLAG_OUTPUT_SCORE"))
                {
                    if (dynamic_cast<Vehicle*>((*agentsOfLane)[i])->type() >= 80)
                    {
                        dynamic_cast<Vehicle*>((*agentsOfLane)[i])->getSubNodes_del();
                        dynamic_cast<Vehicle*>((*agentsOfLane)[i])->scoringSubNodes();
                    }
                }

                ObjManager::deleteVehicle(
                    dynamic_cast<Vehicle*>((*agentsOfLane)[i]));
            }
        }
    }
}

//======================================================================
void ODNode::setOutputGenerateVehicleDataFlag(bool flag)
{
    _isOutputGenerateVehicleData = flag;
}

//======================================================================
void ODNode::clearGVD(GeneratedVehicleData* gvd)
{
    gvd->vehicle     = NULL;
    gvd->lane        = NULL;
    gvd->headway     = 0;
    gvd->genTime     = 0;
    gvd->genInterval = 0;
}
