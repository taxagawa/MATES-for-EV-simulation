/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "VehicleRunningMode.h"
#include "GenerateVehicleController.h"
#include "RoadMap.h"
#include "Intersection.h"
#include "ODNode.h"
#include "Section.h"
#include "Lane.h"
#include "Vehicle.h"
#include "VehicleEV.h"
#include "Random.h"
#include "TimeManager.h"
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

// 車両を確率的でなく等間隔で発生させるは以下のマクロを定義する
// #define GENERATE_VEHICLE_EQUAL_INTERVAL

//======================================================================
GenerateVehicleController::GenerateVehicleController()
{
    _generateVehicleQueue.clear();
    _generateVehiclePriorQueue.clear();
    _rnd = Random::randomGenerator();
}

//======================================================================
GenerateVehicleController::~GenerateVehicleController()
{
    Random::releaseRandomGenerator(_rnd);
}

//======================================================================
GenerateVehicleController& GenerateVehicleController::instance()
{
    static GenerateVehicleController instance;
    return instance;
}

//======================================================================
void GenerateVehicleController::setRoadMap(RoadMap* roadMap)
{
    _roadMap = roadMap;
}

//======================================================================
void GenerateVehicleController::setRunningMode(VehicleRunningMode* runningMode)
{
    _runningMode = runningMode;
}

//======================================================================
bool GenerateVehicleController::getReadyGeneration()
{
    //------------------------------------------------------------------
    // ODノードのレベル分け
    _classifyODNodes();

    //------------------------------------------------------------------
    // ODノード間の最大距離を格納
    maxOD = maxOdDistance();

    //------------------------------------------------------------------
    // 標準的な交通量(単路ごとの台/時) : 基本交通容量の10%
    /*
     * "レーンごと"ではない
     */
    _defaultTrafficVolume[0]
        = static_cast<int>(
            GVManager::getNumeric("DEFAULT_TRAFFIC_VOLUME_WIDE"));
    _defaultTrafficVolume[1]
        = static_cast<int>(
            GVManager::getNumeric("DEFAULT_TRAFFIC_VOLUME_NORMAL"));
    _defaultTrafficVolume[2]
        = static_cast<int>(
            GVManager::getNumeric("DEFAULT_TRAFFIC_VOLUME_NARROW"));

    //------------------------------------------------------------------
    // 車種情報の設定
    // デフォルトで20(普通車), 50(大型車)を作成する
    double length, width, height;
    double weight = 0.0;
    double accel, decel;
    double r, g, b;

    length = GVManager::getNumeric("VEHICLE_LENGTH_PASSENGER");
    width  = GVManager::getNumeric("VEHICLE_WIDTH_PASSENGER");
    height = GVManager::getNumeric("VEHICLE_HEIGHT_PASSENGER");
    accel  = GVManager::getNumeric("MAX_ACCELERATION_PASSENGER");
    decel  = GVManager::getNumeric("MAX_DECELERATION_PASSENGER");
    r = 1.0;
    g = 0.0;
    b = 0.0;
    VFAttribute passenger(VehicleFamily::passenger(),
                          length, width, height, weight,
                          accel, decel, r, g, b);
    VehicleFamilyManager::addVehicleFamily(passenger);

    length = GVManager::getNumeric("VEHICLE_LENGTH_TRUCK");
    width  = GVManager::getNumeric("VEHICLE_WIDTH_TRUCK");
    height = GVManager::getNumeric("VEHICLE_HEIGHT_TRUCK");
    accel  = GVManager::getNumeric("MAX_ACCELERATION_TRUCK");
    decel  = GVManager::getNumeric("MAX_DECELERATION_TRUCK");
    r = 0.3;
    g = 0.7;
    b = 1.0;
    VFAttribute truck(VehicleFamily::truck(),
                      length, width, height, weight,
                      accel, decel, r, g, b);
    VehicleFamilyManager::addVehicleFamily(truck);

    // EVの車種情報の設定
    // デフォルトで80（EV普通車）を作成する
    if (GVManager::getFlag("FLAG_GEN_EVs"))
    {
        length = GVManager::getNumeric("VEHICLE_LENGTH_EV_PASSENGER");
        width  = GVManager::getNumeric("VEHICLE_WIDTH_EV_PASSENGER");
        height = GVManager::getNumeric("VEHICLE_HEIGHT_EV_PASSENGER");
        accel  = GVManager::getNumeric("MAX_ACCELERATION_EV_PASSENGER");
        decel  = GVManager::getNumeric("MAX_DECELERATION_EV_PASSENGER");
        r = 0.0;
        g = 1.0;
        b = 0.0;
        VFAttribute EV_passenger(VehicleFamily::EV_passenger(),
                                 length, width, height, weight,
                                 accel, decel, r, g, b);
        VehicleFamilyManager::addVehicleFamily(EV_passenger);

        length = GVManager::getNumeric("VEHICLE_LENGTH_EV_PASSENGER");
        width  = GVManager::getNumeric("VEHICLE_WIDTH_EV_PASSENGER");
        height = GVManager::getNumeric("VEHICLE_HEIGHT_EV_PASSENGER");
        accel  = GVManager::getNumeric("MAX_ACCELERATION_EV_PASSENGER");
        decel  = GVManager::getNumeric("MAX_DECELERATION_EV_PASSENGER");
        r = 0.3;
        g = 0.0;
        b = 0.0;
        VFAttribute EV_ghost(VehicleFamily::EV_passenger(),
                             length, width, height, weight,
                             accel, decel, r, g, b);
        VehicleFamilyManager::addVehicleFamily(EV_ghost);
    }
    // -eオプションを指定しなかった場合は普通車と同一諸元の車両が発生する
    // ただし車種typeは80のまま
    else
    {
        length = GVManager::getNumeric("VEHICLE_LENGTH_PASSENGER");
        width  = GVManager::getNumeric("VEHICLE_WIDTH_PASSENGER");
        height = GVManager::getNumeric("VEHICLE_HEIGHT_PASSENGER");
        accel  = GVManager::getNumeric("MAX_ACCELERATION_PASSENGER");
        decel  = GVManager::getNumeric("MAX_DECELERATION_PASSENGER");
        r = 1.0;
        g = 0.0;
        b = 0.0;
        VFAttribute passenger(VehicleFamily::EV_passenger(),
                              length, width, height, weight,
                              accel, decel, r, g, b);
        VehicleFamilyManager::addVehicleFamily(passenger);
    }

    // 車両情報をファイルから読み込む
    VehicleFamilyIO::getReadyVehicleFamily();
    VehicleFamilyIO::print();

    //------------------------------------------------------------------
    // 車両発生定義ファイルの読み込み
    if (GVManager::getFlag("FLAG_INPUT_VEHICLE"))
    {
        string fGenerateTable, fDefaultGenerateTable, fFixedGenerateTable;
        GVManager::getVariable("GENERATE_TABLE",
                               &fGenerateTable);
        GVManager::getVariable("DEFAULT_GENERATE_TABLE",
                               &fDefaultGenerateTable);
        GVManager::getVariable("FIXED_GENERATE_TABLE",
                               &fFixedGenerateTable);

        if (!fGenerateTable.empty())
        {
            _table.init(fGenerateTable);
        }
        if (!fDefaultGenerateTable.empty())
        {
            _defaultTable.init(fDefaultGenerateTable);
        }
        if (!fFixedGenerateTable.empty())
        {
            _fixedTable.initFixedTable(fFixedGenerateTable);
        }
    }

    // 車両発生が定義されていない交差点/時間帯のテーブルを作成する
    if (GVManager::getFlag("FLAG_GEN_RAND_VEHICLE"))
    {
        _createRandomTable();
    }

    //------------------------------------------------------------------
    // 経路探索用のパラメータの設定
    _readRouteParameter();

    return true;
}

//======================================================================
void GenerateVehicleController::generateVehicle()
{
    // 現在時刻に適用されるGTCellを有効化する
    /*
     * 未activeなGTCellをactivateするため，
     * 各GTCellがactivateの対象になるのは1回のみ
     */
    _activatePresentGTCells();

    // 車両を確実に発生させる（バス等）
    _generateVehicleFromPriorQueue();

    // 車両を確率的に発生させる
    _generateVehicleFromQueue();

    /*
     * pushVehicleToRealが実行された順にIDが付与されるため，
     * 以下はシングルスレッドで実行しなければならない
     */
    vector<ODNode*> tmpODNodes;
    for (unsigned int i=0; i<_waitingODNodes.size(); i++)
    {
        _waitingODNodes[i]->pushVehicleToReal(_roadMap);
        if (_waitingODNodes[i]->hasWaitingVehicles())
        {
            // まだ登場しきれていないwaitingVehiclesが存在する場合
            // 次のタイムステップでpushVeicleを試みる
            tmpODNodes.push_back(_waitingODNodes[i]);
        }
        else
        {
            // 保持する全てのwaitingVehicleを登場させた場合，
            // 次にaddWaitingVehicleされるまで何もしない
            _waitingODNodes[i]->setWaitingToPushVehicle(false);
        }
    }
    _waitingODNodes.swap(tmpODNodes);
}


//======================================================================
void GenerateVehicleController::generateVehicleManual(
    const std::string& startId,
    const std::string& goalId,
    std::vector<std::string> stopPoints,
    VehicleType vehicleType,
    std::vector<double> params)
{
    ODNode* start
        = dynamic_cast<ODNode*>(_roadMap->intersection(startId));
    if (start==NULL)
    {
        cout << "start:" << startId << " is not a OD node." << endl;
        return;
    }

    ODNode* goal = NULL;
    if (goalId!="******")
    {
        goal = dynamic_cast<ODNode*>(_roadMap->intersection(goalId));
    }
    if (goal==NULL)
    {
        goal = _decideGoalRandomly(start);
    }
    if (goal==NULL)
    {
        cout << "cannot decide goal." << endl;
        return;
    }

    assert(start && goal);

    OD od;
    od.setValue(start->id(), goal->id(), stopPoints);
    Vehicle* newVehicle
        = _createVehicle(start,
                         goal,
                         start,
                         start->nextSection(0),
                         &od,
                         vehicleType,
                         params);

    // startの_waitingVehiclesの先頭に加える
    start->addWaitingVehicleFront(newVehicle);

#ifdef _OPENMP
#pragma omp critical (isWaitingToPushVehicle)
    {
#endif //_OPENMP
        if (!(start->isWaitingToPushVehicle()))
        {
            _waitingODNodes.push_back(start);
            start->setWaitingToPushVehicle(true);
        }
#ifdef _OPENMP
    }
#endif
    newVehicle->route()->print(cout);
}

//======================================================================
void GenerateVehicleController::generateVehicleGhostEV(Vehicle* vehicle)
{
    Vehicle* newVehicle = _createGhostEV(vehicle);

    // ID付与を付与する
    bool result = ObjManager::addVehicleToReal(newVehicle);
    assert(result);


}

//======================================================================
void GenerateVehicleController::_createRandomTable()
{
    // vector<ODNode*> odNodes = _roadMap->odNodes();
    vector<ODNode*> startNodes;

    for (unsigned int i=0; i<3; i++)
    {
        startNodes.insert(startNodes.end(),
                          _startLevel[i].begin(),
                          _startLevel[i].end());
    }

    for (unsigned int i=0; i<startNodes.size(); i++)
    {
        if (_odNodeStartLevel(startNodes[i])<0)
        {
            // ODノードからの流出点がない = Destinationにしかなり得ない
            continue;
        }
        double volume
            = _defaultTrafficVolume[_odNodeStartLevel(startNodes[i])]
            * GVManager::getNumeric("RANDOM_OD_FACTOR");

        vector<const GTCell*> validGTCells;
        _table.getValidGTCells(startNodes[i]->id(), &validGTCells);
        _defaultTable.getValidGTCells(startNodes[i]->id(), &validGTCells);
        // _fixedTableで指定されていてもランダム発生させる

        if (validGTCells.empty())
        {
            // 有効なGTCellが見つからなかった場合
            /*
             * デフォルトの交通量を適用する
             */
            /**
             * @todo シミュレーションの最大時間をGVManagerで管理すべき
             */
            _randomTable.createGTCell(0, 86400000, volume,
                                      startNodes[i]->id(), "******");
        }
        else
        {
            // 有効なGTCellが見つかった場合
            /*
             * 指定時間外はデフォルト交通量を適用
             */
            /**
             * @todo 厳密に区間の計算をする必要があるか
             */
            ulint minStart = ULONG_MAX;
            ulint maxEnd = 0;
            for (unsigned int j=0; j < validGTCells.size(); j++)
            {
                if (validGTCells[j]->begin() < minStart)
                {
                    minStart = validGTCells[j]->begin();
                }
                if (validGTCells[j]->end() > maxEnd)
                {
                    maxEnd = validGTCells[j]->end();
                }
            }
            if (minStart > 0)
            {
                _randomTable.createGTCell(0, minStart, volume,
                                          startNodes[i]->id(), "******");
            }
            if (maxEnd < 86400000)
            {
                _randomTable.createGTCell(maxEnd, 86400000, volume,
                                          startNodes[i]->id(), "******");
            }
        }
    }
}

//======================================================================
void GenerateVehicleController::_activatePresentGTCells()
{
    // 現在時刻で有効なセルを抽出する
    vector<const GTCell*> activeGTCells;
    _table.extractActiveGTCells(&activeGTCells);
    _defaultTable.extractActiveGTCells(&activeGTCells);
    _randomTable.extractActiveGTCells(&activeGTCells);

    // 抽出したセル全てについて車両発生時刻を求める
    for (unsigned int i=0; i<activeGTCells.size(); i++)
    {
        _addNextGenerateTime(activeGTCells[i]->begin(),
                             activeGTCells[i]);
    }

    // 現在時刻で有効なセルを抽出する
    vector<const GTCell*> activeFixedGTCells;
    if ((GVManager::getFlag("DEBUG_FLAG_GEN_FIXED_VEHICLE_ALL_AT_ONCE")))
    {
        _fixedTable.extractActiveGTCellsAllAtOnce(&activeFixedGTCells);
    }
    else
    {
        _fixedTable.extractActiveGTCells(&activeFixedGTCells);
    }

    // 抽出したセル全てについて車両発生時刻を求める
    for (unsigned int i=0; i<activeFixedGTCells.size(); i++)
    {
        _addFixedGenerateTime(activeFixedGTCells[i]);
    }
}

//======================================================================
bool GenerateVehicleController::_addNextGenerateTime(
    ulint startTime,
    const GTCell* cell)
{
    // 交通量0の場合は何もしない
    if (cell->volume()<1.0e-3)
    {
        return false;
    }

    // 平均時間間隔[msec]
    /*
     * 交通量から算出される
     */
    double meanInterval = (60.0*60.0*1000.0)/(cell->volume());

    // 時間間隔
    ulint interval;

#ifdef GENERATE_VEHICLE_EQUAL_INTERVAL //...............................

    if (cell->generatedVolume()==0)
    {
        // このcellによって初めて車両が生成される場合
        /*
         * 交通量が1[veh./hour]の場合は30minで最初の1台発生させる
         */
        interval = ceil(meanInterval/2.0);
    }
    else
    {
        // このcellから既に車両が生成されている場合
        /*
         * 交通量が1[veh./hour]の場合は60min間隔で発生させる
         */
        interval = ceil(meanInterval);
        interval += TimeManager::unit()-interval%TimeManager::unit();
    }

#else // GENERATE_VEHICLE_EQUAL_INTERVAL not defined ...................

    interval = ceil(-meanInterval*log(1-Random::uniform(_rnd)));
    interval += TimeManager::unit()-interval%TimeManager::unit();

#endif // GENERATE_VEHICLE_EQUAL_INTERVAL ..............................

    ulint nextTime = startTime + interval;

    if (nextTime <= cell->end())
    {

#ifdef _OPENMP
#pragma omp critical (addNextGenerateTime)
        {
#endif //_OPENMP

            _generateVehicleQueue.insert(
                pair<unsigned long, const GTCell*>(nextTime, cell));
            const_cast<GTCell*>(cell)->incrementGeneratedVolume();

#ifdef _OPENMP
        }
#endif //_OPENMP

        return true;
    }
    else
    {
        return false;
    }
}

//======================================================================
bool GenerateVehicleController::_addFixedGenerateTime(const GTCell* cell)
{

#ifdef _OPENMP
#pragma omp critical (addFixedGenerateTime)
    {
#endif //_OPENMP

        _generateVehiclePriorQueue.insert(
            pair<unsigned long, const GTCell*>(cell->begin(),
                                               cell));

#ifdef _OPENMP
    }
#endif //_OPENMP
    return true;
}

//======================================================================
void GenerateVehicleController::_generateVehicleFromQueue()
{
    /// @attention 本関数はシングルスレッドで実行すること
    /*
     * 車両生成順が固定できないと乱数の再現性が損なわれるため
     */

    // 現在のタイムステップで車両を発生させるGTCell
    vector<const GTCell*> activeCells;

    while (true)
    {
        if (_generateVehicleQueue.empty())
        {
            break;
        }
        if ((*(_generateVehicleQueue.begin())).first
            > TimeManager::time())
        {
            break;
        }
        activeCells.push_back((*(_generateVehicleQueue.begin())).second);
        _generateVehicleQueue.erase(_generateVehicleQueue.begin());
    }

    if (activeCells.empty())
    {
        return;
    }

    for (unsigned int i=0; i<activeCells.size(); i++)
    {
        // 出発地の取得
        string startId = activeCells[i]->start();
        ODNode* start
            = dynamic_cast<ODNode*>(_roadMap->intersection(startId));
        assert(start);

        // 目的地の取得
        string goalId  = activeCells[i]->goal();
        ODNode* goal = NULL;
        if (goalId == "******")
        {
            goal = _decideGoalRandomly(start);
        }
        else
        {
            goal
                = dynamic_cast<ODNode*>(_roadMap->intersection(startId));
        }
        assert(goal);

        // 経由地の取得
        OD od = *(activeCells[i]->od());
        if (goalId == "******")
        {
            // 目的地の再設定
            const OD* pOD = activeCells[i]->od();
            od.setValue(pOD->start(), goal->id(), *(pOD->stopPoints()));
        }

        // 車種の取得
        VehicleType type = activeCells[i]->vehicleType();

        // 車両の生成
        Vehicle* newVehicle
            = _createVehicle(start, goal, start, start->nextSection(0),
                             &od, type);

        // 出発地のwaitingVehiclesに追加
        start->addWaitingVehicle(newVehicle);

        if (!(start->isWaitingToPushVehicle()))
        {
            _waitingODNodes.push_back(start);
            start->setWaitingToPushVehicle(true);
        }
        // 次の発生時刻を予約
        _addNextGenerateTime(TimeManager::time(), activeCells[i]);
    }
}

//======================================================================
void GenerateVehicleController::_generateVehicleFromPriorQueue()
{
    /// @attention 本関数はシングルスレッドで実行すること
    /*
     * 車両生成順が固定できないと乱数の再現性が損なわれるため
     */

    // 現在のタイムステップで車両を発生させるGTCell
    vector<const GTCell*> activeCells;

    while(true)
    {
        if (_generateVehiclePriorQueue.empty())
        {
            break;
        }
        if ((*(_generateVehiclePriorQueue.begin())).first
            > TimeManager::time())
        {
            if (!(GVManager::getFlag("DEBUG_FLAG_GEN_FIXED_VEHICLE_ALL_AT_ONCE")))
            {
                break;
            }
        }
        activeCells.push_back((*(_generateVehiclePriorQueue.begin())).second);
        _generateVehiclePriorQueue.erase(_generateVehiclePriorQueue.begin());
    }

    if (activeCells.empty())
    {
        return;
    }
    for (unsigned int i=0; i<activeCells.size(); i++)
    {
        // 出発地の取得
        string startId = activeCells[i]->start();
        ODNode* start
            = dynamic_cast<ODNode*>(_roadMap->intersection(startId));
        assert(start);

        // 目的地の取得
        string goalId  = activeCells[i]->goal();
        ODNode* goal = NULL;
        if (goalId == "******")
        {
            goal = _decideGoalRandomly(start);
        }
        else
        {
            goal
                = dynamic_cast<ODNode*>(_roadMap->intersection(goalId));
        }
        assert(goal);

        // 経由地の取得
        OD od = *(activeCells[i]->od());
        if (goalId == "******")
        {
            // 目的地の再設定
            const OD* pOD = activeCells[i]->od();
            od.setValue(pOD->start(), goal->id(), *(pOD->stopPoints()));
        }

        // 車種の取得
        VehicleType type = activeCells[i]->vehicleType();

        // 車両の生成
        Vehicle* newVehicle
            = _createVehicle(start, goal, start, start->nextSection(0),
                             &od, type);

        // startの_waitingVehiclesの先頭に加える
        start->addWaitingVehicleFront(newVehicle);

        if (!(start->isWaitingToPushVehicle()))
        {
            _waitingODNodes.push_back(start);
            start->setWaitingToPushVehicle(true);
        }
    }
}

//======================================================================
Vehicle* GenerateVehicleController::_createVehicle(
    ODNode* start,
    ODNode* goal,
    Intersection* past,
    Section* section,
    OD* od,
    VehicleType vehicleType)
{
    assert(start!=NULL && goal!=NULL);

    // 車両の生成
    Vehicle* tmpVehicle = ObjManager::createVehicle(vehicleType);

    //車両属性の設定
    tmpVehicle->setType(vehicleType);
    _setVehicleStatus(tmpVehicle);

    // 経路選択用のパラメータの設定
    tmpVehicle->router()->setTrip(od, _roadMap);
    tmpVehicle->router()
        ->setParam(_vehicleRoutingParams
                   [Random::uniform(_rnd,
                                    0, _vehicleRoutingParams.size())]);

    // 走行モードの設定
    // by uchida
    tmpVehicle->setRunningMode(_runningMode);

    // by takusagawa 2018/01/05
    // OD距離を設定
    tmpVehicle->setOdDistance();

    if (VehicleFamily::isEV_Passenger(vehicleType))
    {
      dynamic_cast<VehicleEV*>(tmpVehicle)->setInitSoC();
      // by takusagawa 2018/9/25
      // 一定割合で待ち時間情報を取得可能なEVを発生させる
      double rnd = 0.0;
      rnd = Random::uniform(_rnd);
      //cout << "rnd: " << rnd << endl;

      if (rnd <= CAN_RECEIVE_WAITING_INFO_RATE)
      {
          dynamic_cast<VehicleEV*>(tmpVehicle)->setReceiveWaitingInfo();
      }
    }

    // 経路選択は関数の外に出す
    // tmpVehicle->reroute(section, past);

    return tmpVehicle;
}

//======================================================================
Vehicle* GenerateVehicleController::_createVehicle(
    ODNode* start,
    ODNode* goal,
    Intersection* past,
    Section* section,
    OD* od,
    VehicleType vehicleType,
    std::vector<double> params)
{
    assert(start!=NULL && goal!=NULL);

    // 車両の生成
    Vehicle* tmpVehicle = ObjManager::createVehicle(vehicleType);

    // 車両属性の設定
    tmpVehicle->setType(vehicleType);
    _setVehicleStatus(tmpVehicle);

    // 経路選択用のパラメータの設定
    vector<double> p;
    tmpVehicle->router()->setTrip(od, _roadMap);
    for (int i=0; i<VEHICLE_ROUTING_PARAMETER_SIZE; i++)
    {
        p.push_back(_vehicleRoutingParams[0][i]);
    }
    for (unsigned int i=0; i<params.size()&&i<p.size(); i++)
    {
        p[i] = params[i];
    }
    tmpVehicle->router()->setParam(p);

    // 経路選択
    // tmpVehicle->reroute(section, past);

    return tmpVehicle;
}

//======================================================================
Vehicle* GenerateVehicleController::_createGhostEV(Vehicle* vehicle)
{
    // 車両の生成
    Vehicle* tmpVehicle = ObjManager::createVehicle(VehicleFamily::EV_ghost());

    // 車両属性の設定
    tmpVehicle->setType(VehicleFamily::EV_ghost());
    _setVehicleStatus(tmpVehicle);

    // 経路選択用のパラメータの設定
    tmpVehicle->router()->setParam(vehicle->router()->param());

    OD* od;
    od = vehicle->router()->od();
    tmpVehicle->router()->setTrip(od, _roadMap);

    // 経路選択
    // tmpVehicle->reroute(section, past);

    return tmpVehicle;
}

//======================================================================
void GenerateVehicleController::_setVehicleStatus(Vehicle* vehicle)
{
    double length, width, height;
    double accel, decel;
    double r, g, b;

    // 車種に対応付けられた属性を取得する
    /*
     * ファイルから読み込んだ属性を適用するように変更
     * 2014/6/8 by H.Fujii
     */
    VFAttribute* vfa
        = VehicleFamilyManager::vehicleFamilyAttribute(vehicle->type());
    if (!vfa)
    {
        if (VehicleFamily::isTruck(vehicle->type()))
        {
            vfa = VehicleFamilyManager::vehicleFamilyAttribute
                (VehicleFamily::truck());
        }
        // EV追加
        else if (VehicleFamily::isEV_Passenger(vehicle->type()))
        {
            vfa = VehicleFamilyManager::vehicleFamilyAttribute
                (VehicleFamily::EV_passenger());
        }
        else
        {
            vfa = VehicleFamilyManager::vehicleFamilyAttribute
                (VehicleFamily::passenger());
        }
    }
    vfa->getSize(&length, &width, &height);
    vfa->getPerformance(&accel, &decel);
    vfa->getBodyColor(&r, &g, &b);

    vehicle->setBodySize(length, width, height);
    vehicle->setPerformance(accel, decel);
    vehicle->setBodyColor(r, g, b);
}

//======================================================================
ODNode* GenerateVehicleController::_decideGoalRandomly(ODNode* start)
{
    ODNode* result = NULL;

    int level[3];
    for(int i=0; i<3; i++)
    {
        level[i] = _defaultTrafficVolume[i];
        if (_goalLevel[i].size()==0)
        {
            level[i] = 0;
        }
    }

    unsigned int total = 0;
    for (int i=0; i<3; i++)
    {
        total += level[i] * _goalLevel[i].size();
    }
    unsigned int goalLevel = 0;

    if (total==0)
    {
        // totalが0になるのは想定していないが...
        cerr << "sum of DEFAULT_TRAFFIC_VOLUME must not be 0" << endl;
        return NULL;
    }
    // 目的地のレベルをランダムに選択
    unsigned int r1 = Random::uniform(_rnd, RAND_MAX) % total;
    for (unsigned int i=0; i<3; i++)
    {
        if (r1<level[i]*_goalLevel[i].size())
        {
            goalLevel = i;
        }
        else
        {
            r1 -= level[i] * _goalLevel[i].size();
            continue;
        }
    }

    // 指定されたレベルから目的地をランダムに選択
    unsigned int r2 = Random::uniform(_rnd,
                                      _goalLevel[goalLevel].size());
    while(true)
    {
        result = _goalLevel[goalLevel][r2];

#ifdef UNIQUE_NETWORK
        if (start != result)
        {
            // start以外のODノードであればwhileループから抜ける
            break;
        }
#else  //UNIQUE_NETWORK
        if (start != result
            && start->isNetworked(start, result))
        {
            // start以外であり，startから到達可能であればwhileループから抜ける
            break;
        }
#endif //UNIQUE_NETWORK

        // startと同じODノードを選択してしまった場合は再度選択する
        if (_goalLevel[goalLevel].size()==1)
        {
            total = 0;
            for (unsigned int i=0; i<3; i++)
            {
                if (i != goalLevel)
                {
                    total += level[i] * _goalLevel[i].size();
                }
            }
            r1 = Random::uniform(_rnd, RAND_MAX) % total;
            for (unsigned int i=0; i<3; i++)
            {
                if (r1<level[i]*_goalLevel[i].size())
                {
                    goalLevel = i;
                }
                else
                {
                    r1 -= level[i] * _goalLevel[i].size();
                }
            }
        }
        // 区間[1, _goalLevel[goalLevel].size()-1]の乱数ををrに加えて更新する
        r2 = (r2 + Random::uniform(_rnd,
                                   1, _goalLevel[goalLevel].size()))
            % _goalLevel[goalLevel].size();
    }
    assert(result);
    return result;
}

//======================================================================
void GenerateVehicleController::_classifyODNodes()
{
    vector<string> excludedStarts;
    vector<string> excludedGoals;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 除外ファイルの読み込み
    string fODExFile;
    GVManager::getVariable("OD_NODE_EXCLUSION_FILE", &fODExFile);
    ifstream inODExFile(fODExFile.c_str(), ios::in);
    if (inODExFile.good())
    {
        string str;
        while (inODExFile.good())
        {
            getline(inODExFile, str);
            AmuStringOperator::getAdjustString(&str);
            if (!str.empty())
            {
                vector<string> tokens;
                AmuStringOperator::getTokens(&tokens, str, ',');
                assert(tokens.size()==2);

                // 第1カラムは"O", "D", "OD"のいずれか
                // 第2カラムは交差点ID
                if (tokens[0]=="O")
                {
                    excludedStarts.push_back
                        (AmuConverter::formatId(tokens[1],
                                                NUM_FIGURE_FOR_INTERSECTION));
                }
                else if (tokens[0]=="D")
                {
                    excludedGoals.push_back
                        (AmuConverter::formatId(tokens[1],
                                                NUM_FIGURE_FOR_INTERSECTION));
                }
                else if (tokens[0]=="OD")
                {
                    excludedStarts.push_back
                        (AmuConverter::formatId(tokens[1],
                                                NUM_FIGURE_FOR_INTERSECTION));
                    excludedGoals.push_back
                        (AmuConverter::formatId(tokens[1],
                                                NUM_FIGURE_FOR_INTERSECTION));
                }
            }
        }
        inODExFile.close();

        cout << endl << "*** Excluded Random Origin ***" << endl;
        for (unsigned int i=0; i<excludedStarts.size(); i++)
        {
            cout << excludedStarts[i] << endl;
        }
        cout << "*** Excluded Random Destination ***" << endl;
        for (unsigned int i=0; i<excludedGoals.size(); i++)
        {
            cout << excludedGoals[i] << endl;
        }
        cout << endl;
    }
    else
    {
        cout << "no ODNode exclusion file: " << fODExFile << endl;
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 除外ファイルで指定されていないODノードをレベル分けして格納する
    vector<ODNode*> odNodes = _roadMap->odNodes();
    vector<string>::iterator its;
    for (unsigned int i=0; i<odNodes.size(); i++)
    {
        if (0<=_odNodeStartLevel(odNodes[i])
            && _odNodeStartLevel(odNodes[i])<3)
        {
            // excludedStartsに登録されていなければ分類する
            its = find(excludedStarts.begin(),
                       excludedStarts.end(),
                       odNodes[i]->id());
            if (its == excludedStarts.end())
            {
                _startLevel[_odNodeStartLevel(odNodes[i])]
                    .push_back(odNodes[i]);
            }
        }
        if (0<=_odNodeGoalLevel(odNodes[i])
            && _odNodeGoalLevel(odNodes[i])<3)
        {
            // excludedGoalsに登録されていなければ分類する
            its = find(excludedGoals.begin(),
                       excludedGoals.end(),
                       odNodes[i]->id());
            if (its == excludedGoals.end())
            {
                _goalLevel[_odNodeGoalLevel(odNodes[i])]
                    .push_back(odNodes[i]);
            }
        }
    }
}

//======================================================================
int GenerateVehicleController::_odNodeStartLevel(ODNode* node) const
{
    int result = -1;
    // ODノードから見たnumOut
    if (node->border(0)->numOut()>=3)
    {
        result = 0;
    } else if (node->border(0)->numOut()==2)
    {
        result = 1;
    } else if (node->border(0)->numOut()==1)
    {
        result = 2;
    }
    return result;
}

//======================================================================
int GenerateVehicleController::_odNodeGoalLevel(ODNode* node) const
{
    // ODノードから見たnumIn
    int result = -1;
    if (node->border(0)->numIn()>=3)
    {
        result = 0;
    }
    else if (node->border(0)->numIn()==2)
    {
        result = 1;
    }
    else if (node->border(0)->numIn()==1)
    {
        result = 2;
    }
    return result;
}

// by takusagawa 2018/10/1
//======================================================================
double GenerateVehicleController::maxOdDistance()
{
    vector<ODNode*> odNodes = _roadMap->odNodes();
    // 最長距離
    double maxD = 0.0;
    // 書き換え必須
    // vector<ODNode*>::iterator ite1 = odNodes.begin();
    // while (ite1 != odNodes.end())
    // {
    //     vector<ODNode*>::iterator ite2 = odNodes.begin();
    //     while (ite2 != odNodes.end())
    //     {
    //         if (ite1 == ite2) continue;
    //
    //         double distance = (*ite1)->center().distance((*ite2)->center());
    //
    //         if (distance > maxD)
    //         {
    //             maxD = distance;
    //             ite2++;
    //         }
    //         else
    //         {
    //             ite2++;
    //         }
    //     }
    //     ite1++;
    // }
    cout << "@@@@@@@@@@@@@@@@@@@@@@@@@ " << maxD << " @@@@@@@@@@@@@@@@@@@@@@@@@@@" << endl;
    return maxD;
}

//======================================================================
void GenerateVehicleController::_readRouteParameter()
{
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * 経路探索用のパラメータの設定
     * 0番目は距離に関するコスト（これが大きいと距離が短い経路が高効用）
     * 1番目は時間に関するコスト（時間が短い経路が高効用）
     * 2番目は交差点での直進に関するコスト（直進が少ない経路が高効用）
     * 3番目は交差点での左折に関するコスト（左折が少ない経路が高効用）
     * 4番目は交差点での右折に関するコスト（右折が少ない経路が高効用）
     * 5番目は道路の広さに関するコスト（道路幅が広い経路が高効用）
     *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */

    _vehicleRoutingParams.resize(1);
    _vehicleRoutingParams[0].resize(VEHICLE_ROUTING_PARAMETER_SIZE);

    _vehicleRoutingParams[0][0] = 1;
    _vehicleRoutingParams[0][1] = 0;
    _vehicleRoutingParams[0][2] = 0;
    _vehicleRoutingParams[0][3] = 0;
    _vehicleRoutingParams[0][4] = 0;
    _vehicleRoutingParams[0][5] = 0;

    string fParamFile;
    GVManager::getVariable("VEHICLE_ROUTE_PARAM_FILE",
                           &fParamFile);

    ifstream inParamFile(fParamFile.c_str(), ios::in);
    if (inParamFile.good())
    {
        string str;
        int index = 0;
        while (inParamFile.good())
        {
            getline(inParamFile, str);
            AmuStringOperator::getAdjustString(&str);
            if (!str.empty())
            {
                vector<string> tokens;
                AmuStringOperator::getTokens(&tokens, str, ',');
                if (tokens.size()==VEHICLE_ROUTING_PARAMETER_SIZE)
                {
                    // パラメータ指定が有効な行
                    _vehicleRoutingParams.resize(index+1);
                    _vehicleRoutingParams[index]
                        .resize(VEHICLE_ROUTING_PARAMETER_SIZE);

                    for (unsigned int i=0; i<tokens.size(); i++)
                    {
                        _vehicleRoutingParams[index][i]
                            = AmuConverter::strtod(tokens[i]);
                    }
                    index++;
                }
            }
        }
        inParamFile.close();
    }
    else
    {
        if (GVManager::getFlag("FLAG_INPUT_VEHICLE"))
        {
            // 入力ファイルが存在しない場合
            cout << "no vehicle routing parameter file: "
                 << fParamFile << endl;
        }
        _vehicleRoutingParams.resize(3);
        for (unsigned int i=1; i<_vehicleRoutingParams.size(); i++)
        {
            _vehicleRoutingParams[i]
                .resize(VEHICLE_ROUTING_PARAMETER_SIZE);
        }

        _vehicleRoutingParams[1][0] = 0;
        _vehicleRoutingParams[1][1] = 1;
        _vehicleRoutingParams[1][2] = 0;
        _vehicleRoutingParams[1][3] = 0;
        _vehicleRoutingParams[1][4] = 0;
        _vehicleRoutingParams[1][5] = 0;

        _vehicleRoutingParams[2][0] = 1;
        _vehicleRoutingParams[2][1] = 1;
        _vehicleRoutingParams[2][2] = 0;
        _vehicleRoutingParams[2][3] = 0;
        _vehicleRoutingParams[2][4] = 0;
        _vehicleRoutingParams[2][5] = 0;
    }

    if (GVManager::getFlag("FLAG_VERBOSE"))
    {
        cout << endl << "*** Vehicle Routing Parameters ***" << endl;
        cout << "NumParams: " << _vehicleRoutingParams.size() << ", "
             << _vehicleRoutingParams[0].size() << endl;
        for (unsigned int i=0; i<_vehicleRoutingParams.size(); i++)
        {
            cout << i << ":";
            for (unsigned int j=0; j<_vehicleRoutingParams[i].size(); j++)
            {
                cout << _vehicleRoutingParams[i][j] << ",";
            }
            cout << endl;
        }
        cout << endl;
    }
}
