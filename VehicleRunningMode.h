/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __VEHICLE_RUNNING_MODE_H__
#define __VEHICLE_RUNNING_MODE_H__

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
class RandomGenerator;

/// 車両の走行モードを扱うクラス
/**
 * @ingroup Running
 */
class VehicleRunningMode
{
public:
    /// 速度テーブルを作成する
    void createRunningMode();

    /// 速度を返す
    double velocity(ulint modeTime);

    // 出力
    // by uhida
    void print();

    // 1サイクルの秒数[msec]を返す
    ulint cycle();

    // 走行モードファイルの存在確認
    bool isExist();

    //runningMode();

//private:
    VehicleRunningMode();
    ~VehicleRunningMode();

    std::map<ulint, double> _runningMode;
    std::map<ulint, double>::iterator itr;
    std::map<ulint, double>::reverse_iterator ritr;
};

#endif //__VEHICLE_RUNNING_MODE_H__
