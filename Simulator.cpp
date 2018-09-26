/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "Simulator.h"
#include "GVManager.h"
#include "GVInitializer.h"
#include "RoadMapBuilder.h"
#include "RoadMap.h"
#include "ObjManager.h"
#include "FileManager.h"
#include "LaneBundle.h"
#include "Intersection.h"
#include "ODNode.h"
#include "CSNode.h"
#include "NCSNode.h"
#include "Section.h"
#include "Lane.h"
#include "RoadEntity.h"
#include "Vehicle.h"
#include "VehicleFamily.h"
#include "VehicleIO.h"
#include "SignalIO.h"
#include "Router.h"
#include "Random.h"
#include "DetectorIO.h"
#include "DetectorUnit.h"
#include "GenerateVehicleIO.h"
#include "GenerateVehicleController.h"
#include "GenerateVehicleRunningMode.h"
#include "VehicleRunningMode.h"
#include "VehicleFamilyManager.h"
#include "VehicleFamilyIO.h"
#include "ScheduleManager.h"
#include "AmuStringOperator.h"
#include "AmuConverter.h"
#include "Conf.h"
#include "SubNode.h"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <cmath>
#include <map>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif //_OPENMP

using namespace std;

//======================================================================
Simulator::Simulator()
{
    _roadMap = 0;
    _checkLaneError = false;

    _genVehicleController = &GenerateVehicleController::instance();

    _vehicleIO = &VehicleIO::instance();
    _signalIO = &SignalIO::instance();
}

//======================================================================
Simulator::~Simulator()
{
    TimeManager::printAllClockers();

    // Managerで管理するオブジェクトの開放
    TimeManager::deleteAllClockers();
    FileManager::deleteAllOFStreams();
    ObjManager::deleteAll();

    if (_roadMap!=NULL)
    {
        delete _roadMap;
    }

    Random::finalize();
}

//======================================================================
bool Simulator::hasInit() const
{
    if (_roadMap)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//======================================================================
bool Simulator::getReadyRoadMap()
{
    RoadMapBuilder builder;

    // 道路ネットワークの作成
    builder.buildRoadMap();

    // 制限速度の設定
    builder.setSpeedLimit();

    // 単路の通行規制の設定
    builder.setTrafficControlSection();

    // 単路の選択確率の設定
    builder.setRoutingProbability();

    if (GVManager::getFlag("FLAG_INPUT_SIGNAL"))
    {
        // 信号の作成
        builder.buildSignals();
    }
    else
    {
        // 全青信号の作成
        builder.buildSignalsAllBlue();
    }

    _roadMap = builder.roadMap();
    _signalIO->setRoadMap(_roadMap);

    // mapInfo.txtの作成
    if (_roadMap)
    {
        _roadMap->writeMapInfo();
    }

    // コンソールへ地図情報を表示する
    if (GVManager::getFlag("FLAG_VERBOSE"))
    {
        _roadMap->dispIntersections();
    }

    // signal_count.txtの出力
    string fSignalCount;
    GVManager::getVariable("RESULT_SIGNAL_COUNT_FILE", &fSignalCount);
    ofstream ofs(fSignalCount.c_str(), ios::out);

    CITRMAPSI its;
    // 信号の総数 (信号ID=ノードIDの数とは一致しないので注意)
    int totalNumberOfSignals=0;
    for (its=_roadMap->signals()->begin();
         its!=_roadMap->signals()->end();
         its++)
    {
        totalNumberOfSignals += (*its).second->numDirections();
    }

    ofs << _roadMap->intersections()->size() << "\n" // 交差点の総数
        << totalNumberOfSignals;		     // 信号機の総数

    return _roadMap;
}

//======================================================================
bool Simulator::getReadyRoadsideUnit()
{
    assert(_roadMap);

    // 車両感知器設定ファイルの読み込み
    DetectorIO::getReadyDetectors(_roadMap);

    // 感知器データ出力ファイルの準備
    vector<DetectorUnit*>* detectorUnits = ObjManager::detectorUnits();
    DetectorIO::getReadyOutputFiles(detectorUnits);
    if (GVManager::getFlag("FLAG_VERBOSE"))
    {
        DetectorIO::print();
    }

    // 車両発生カウンタの設定ファイル読み込みと準備
    GenerateVehicleIO::getReadyCounters(_roadMap);

    return true;
}

//======================================================================
bool Simulator::getReadyVehicles()
{
    assert(_roadMap);

    _genVehicleController->setRoadMap(_roadMap);
    // by uchida
    _genVehicleController->setRunningMode(_runningMode);

    _genVehicleController->getReadyGeneration();

    return true;
}

//======================================================================
// by uchida
bool Simulator::getReadyRunningMode()
{

    assert(_roadMap);

    GenerateVehicleRunningMode runningMode;
    runningMode.runningModeGenerate();
    _runningMode = runningMode.runningMode();

    return _runningMode;
}

//======================================================================
void Simulator::checkLane()
{
    // レーンチェック、エラー時は表示確認のため run のみ止める
    _checkLaneError = !_roadMap->checkIntersectionLane();
}

//======================================================================
bool Simulator::checkLaneError()
{
    return _checkLaneError;
}

//======================================================================
bool Simulator::run(ulint time)
{
    // レーンチェックエラー、表示確認のため run のみ止める
    if (_checkLaneError)
    {
        return false;
    }
    if (time>TimeManager::time())
    {
        TimeManager::startClock("TOTALRUN");
        while (time>TimeManager::time())
        {
            timeIncrement();
        }
        TimeManager::stopClock("TOTALRUN");
        return true;
    }
    else
    {
        return false;
    }
}

//======================================================================
bool Simulator::timeIncrement()
{
    // 時刻の更新
    TimeManager::increment();
    if (GVManager::getFlag("FLAG_VERBOSE"))
    {
        if (TimeManager::time()%1000==0)
        {
            cout << "Time: "
                 << TimeManager::time()/1000 << "[sec]" << endl;
        }
    }

    // by uchida 2017/2/27
    if (TimeManager::time() == 1000 && GVManager::getFlag("FLAG_OUTPUT_SCORE"))
    {

        string fNearestFile;
        GVManager::getVariable("NEAREST_CS_FILE",
                               &fNearestFile);
        ifstream inNearestFile(fNearestFile.c_str(), ios::in);
        vector<double> distance;

        if (inNearestFile.good())
        {
            string str;
            while (inNearestFile.good())
            {
                getline(inNearestFile, str);
                AmuStringOperator::getAdjustString(&str);
                if (!str.empty())
                {
                    double value = AmuConverter::strtod(str);
                    distance.push_back(value);
                }
            }
        }

        if (distance.size() == (_roadMap->SubNodes()).size())
        {
            // ファイルに記録されていた距離を格納
            int max = (_roadMap->SubNodes()).size();
            for (int i = 0; i < max; i++)
            {
                (_roadMap->SubNodes())[i]->setNearestCS(distance[i]);
            }
        }
        else
        {
            // ファイルがない場合は距離を記録
            ofstream ofs(fNearestFile.c_str(), ios::trunc);
            if (!ofs.fail())
            {
                // 最近傍CSとの距離を格納
                int p = 0;
                int max = (_roadMap->SubNodes()).size();
                for (int i = 0; i < max; i++)
                {
                    double progress = i*100.0/max;
                    if ((int)progress != p)
                    {
                        p = (int)progress;
                        cout << "progress: " << p << "%" << endl;
                    }
                    (_roadMap->SubNodes())[i]->nearestCS(_roadMap);

                    ofs << (_roadMap->SubNodes())[i]->distanceCS() << "\n";
                }
                ofs.close();
            }

        }

    }

    // by takusagawa 2018/9/25
    // 30秒ごとに全CSの推定待ち時間を更新
    // 一定時間ごとに一斉更新するか,EVからの問い合わせがあるごとに最新の推定待ち時間を返すかの
    // ２つの方法が考えられるが,充電車両が多くなると後者は計算時間が大きくなりそうなので
    // とりあえず前者にしておく.
    if (TimeManager::time() % 30000 == 0)
    {
        vector<CSNode*> csNodes = _roadMap->csNodes();
        for (int i = 0; i < csNodes.size(); i++)
        {
            cout << "i: " << i << endl;
            csNodes[i]->estimatedWaitingTimeCalc();
        }
    }

    // by uchida 2017/2/27
    if (TimeManager::time() % 60000 == 0 && GVManager::getFlag("FLAG_OUTPUT_SCORE"))
    {
//        cout << "SN: x y z z_dummy score score_d" << endl;
//        vector<SubNode*> vec = _roadMap->SubNodes();
//        for (int i = 0; i < vec.size(); i++)
//        {
//            vec[i]->print();
            _roadMap->writeScoreData(TimeManager::time());
//        }
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // エージェント列の更新
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef MEASURE_TIME
    TimeManager::startClock("RENEW_AGENT");
#endif //MEASURE_TIME

    // レーン上のエージェントを更新
    _roadMap->renewAgentLine();

#ifdef MEASURE_TIME
    TimeManager::stopClock("RENEW_AGENT");
#endif //MEASURE_TIME

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // モニタリング
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef MEASURE_TIME
    TimeManager::startClock("MONITORING");
#endif //MEASURE_TIME

    if (GVManager::getFlag("FLAG_OUTPUT_MONITOR_D")
        || GVManager::getFlag("FLAG_OUTPUT_MONITOR_S"))
    {
        vector<DetectorUnit*>* detectorUnits
            = ObjManager::detectorUnits();
        for_each(detectorUnits->begin(),
                 detectorUnits->end(),
                 mem_fun(&DetectorUnit::monitorLanes));
        DetectorIO::writeTrafficData(detectorUnits);
    }

//#define TMP_CODE
#ifdef TMP_CODE
    /**
     * @todo サンプルコード． いずれ書き直し．
     */
    string resultPath;
    Section* section = _roadMap->section("009843009915");
    if (section)
    {
        GVManager::getVariable("RESULT_OUTPUT_DIRECTORY", &resultPath);
        ofstream& ofs = FileManager::getOFStream(resultPath+"special.csv");
        ofs << TimeManager::time();
        const RMAPLAN* lanes = section->lanes();
        CITRMAPLAN citl;
        for (citl = lanes->begin(); citl != lanes->end(); citl++)
        {
            if (!(section->isUp((*citl).second)))
            {
                bool isStopVehicleFound = false;
                ofs << ", " << (*citl).second->id();
                // エージェントの取得
                vector<RoadOccupant*>* agents = (*citl).second->agents();
                // agentsは始点に近い順に並んでいる
                for (unsigned int i=0; i<agents->size(); i++)
                {
                    if ((*agents)[i]->velocity() < 1.0e-6)
                    {
                        ofs << ", "
                            << section->lengthToNext((*citl).second,
                                                     (*agents)[i]->length());
                        isStopVehicleFound = true;
                        break;
                    }
                }
                // 停止車両が見つからなかった場合の処理
                if (!isStopVehicleFound)
                {
                    ofs << ", 0";
                }
            }
        }
        ofs << endl;
    }
    /// サンプルコードここまで．
#endif

#ifdef MEASURE_TIME
    TimeManager::stopClock("MONITORING");
#endif //MEASURE_TIME

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // リンク旅行時間の更新
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef EXCLUDE_VEHICLES

#ifdef MEASURE_TIME
    TimeManager::startClock("RENEW_PASSTIME");
#endif //MEASURE_TIME

    /**
     * @note
     * この部分の処理を並列化する場合には
     * Intersectionをvectorで(も)管理する必要がある
     * あるいはOpenMPのtask構文を使用できるかも？
     */
    if (TimeManager::time()
        % (static_cast<int>(GVManager::getNumeric
                            ("INTERVAL_RENEW_LINK_TRAVEL_TIME"))*1000)
        == 0)
    {
        const RMAPI* intersections = _roadMap->intersections();
        for (CITRMAPI iti = intersections->begin();
             iti != intersections->end();
             iti++)
        {
            (*iti).second->renewPassTimeForGlobal();
        }
    }

    // by uchida 2016/5/23
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // CSNodeの充電電力量の更新
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    if (TimeManager::time()
//        % (static_cast<int>(GVManager::getNumeric
//                            ("INTERVAL_RENEW_LINK_TRAVEL_TIME"))*1000)
//        == 0)
//    {

        vector<CSNode*> csNodes = _roadMap->csNodes();
        for (int i = 0; i < csNodes.size(); i++)
        {
            csNodes[i]->integrated(csNodes[i]->charged());
            csNodes[i]->initCharge();
        }
        vector<NCSNode*> ncsNodes = _roadMap->ncsNodes();
        for (int i = 0; i < ncsNodes.size(); i++)
        {
            //if (ncsNodes[i]->id() == "000232")
            //{
            //    cout << ncsNodes[i]->id() << ": " << ncsNodes[i]->charged() << endl;
            //}
            ncsNodes[i]->integrated(ncsNodes[i]->charged());
            ncsNodes[i]->initCharge();
        }

        // CSの充電出力を出力する
        if (GVManager::getFlag("FLAG_GEN_CSs"))
        {
            // **sec毎に充電電力量を出力
            int interval = GVManager::getNumeric("OUTPUT_CSs_INTERVAL");
            if (TimeManager::time() % interval == 0)
            {

                string outputDir, prefixD;
                GVManager::getVariable("RESULT_OUTPUT_DIRECTORY", &outputDir);

                if (!outputDir.empty())
                {
                    string fLog = outputDir + "chargeLog001.csv";
                    ofstream outLogFile(fLog.c_str(), ios::app);

                    const RMAPI* intersections = _roadMap->intersections();
                    CITRMAPI iti = intersections->begin();
                    if (!outLogFile.fail())
                    {
                        if (TimeManager::time() == interval)
                        {
                            outLogFile << " ,";
                            for (int i = 0; i < intersections->size(); i++)
                            {
                                if (dynamic_cast<CSNode*>((*iti).second))
                                {
                                    outLogFile << ((*iti).second)->id() << ",";
                                }
                                else if (dynamic_cast<NCSNode*>((*iti).second))
                                {
                                    outLogFile << ((*iti).second)->id() << ",";
                                }
                                iti++;
                            }
                            outLogFile << endl;
                        }


                        outLogFile << TimeManager::time() << ",";
                    	iti = intersections->begin();
                        for (int i = 0; i < intersections->size(); i++)
                        {
                            if (dynamic_cast<CSNode*>((*iti).second))
                            {
                                CSNode* node = dynamic_cast<CSNode*>((*iti).second);
                                outLogFile << node->integratedCharge()/(1000.0*3600.0) << ",";
                                node->initIntegrated();
                            }
                            else if (dynamic_cast<NCSNode*>((*iti).second))
                            {
                                NCSNode* node = dynamic_cast<NCSNode*>((*iti).second);
                                outLogFile << node->integratedCharge()/(1000.0*3600.0) << ",";
                                node->initIntegrated();
                            }
                            iti++;
                        }
                        outLogFile << endl;
                    }
                }
            }
        }

#ifdef MEASURE_TIME
    TimeManager::stopClock("RENEW_PASSTIME");
#endif //MEASURE_TIME

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 環境の更新
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef MEASURE_TIME
    TimeManager::startClock("RENEW_ENV");
#endif //MEASURE_TIME

    ScheduleManager::renewSpeedLimit();

#ifdef MEASURE_TIME
    TimeManager::stopClock("RENEW_ENV");
#endif //MEASURE_TIME

#endif //EXCLUDE_VEHICLES

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // エージェントの消去
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef MEASURE_TIME
    TimeManager::startClock("DELETE_AGENT");
#endif //MEASURE_TIME

    _roadMap->deleteStrandedAgents();
    _roadMap->deleteArrivedAgents();

#ifdef MEASURE_TIME
    TimeManager::stopClock("DELETE_AGENT");
#endif //MEASURE_TIME

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // エージェントの発生
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 車両の発生
#ifndef EXCLUDE_VEHICLES

#ifdef MEASURE_TIME
    TimeManager::startClock("GENERATE");
#endif //MEASURE_TIME

    // 車両の生成と登場
    _genVehicleController->generateVehicle();

    // 2017/8/25 by uchida
    // ゴースト発生判定
    // if (_roadMap->ghostList().size()!=0)
    // {
    //     vector<string> vec = _roadMap->ghostList();
    //     for (int i=0; i<vec.size(); i++)
    //     {
    //         _genVehicleController
    //             ->generateVehicleGhostEV(ObjManager::vehicle(vec[i]));
    //     }
    //     // 毎ステップリストを初期化する
    //     _roadMap->ghostList().clear();
    // }

#ifdef MEASURE_TIME
    TimeManager::stopClock("GENERATE");
#endif //MEASURE_TIME

#endif //EXCLUDE_VEHICLES

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 認知
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef EXCLUDE_VEHICLES
    vector<Vehicle*>* vehicles = ObjManager::vehicles();

#ifdef _OPENMP //-------------------------------------------------------

    int vehiclesSize = vehicles->size();

#ifdef MEASURE_TIME
    TimeManager::startClock("RECOGNIZE");
#endif //MEASURE_TIME

#pragma omp parallel for schedule (dynamic)
    for (int i = 0; i < vehiclesSize; i++)
    {
        (*vehicles)[i]->recognize();
    }

#ifdef MEASURE_TIME
    TimeManager::stopClock("RECOGNIZE");
#endif //MEASURE_TIME

#else //_OPENMP not defined --------------------------------------------

#ifdef MEASURE_TIME
    TimeManager::startClock("RECOGNIZE");
#endif //MEASURE_TIME

    for_each(vehicles->begin(), vehicles->end(),
             mem_fun(&Vehicle::recognize));

#ifdef MEASURE_TIME
    TimeManager::stopClock("RECOGNIZE");
#endif //MEASURE_TIME

#endif //_OPENMP -------------------------------------------------------

#endif //EXCLUDE_VEHICLES

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 意志決定
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef EXCLUDE_VEHICLES

#ifdef _OPENMP //-------------------------------------------------------

#ifdef MEASURE_TIME
    TimeManager::startClock("READYTORUN");
#endif //MEASURE_TIME

#pragma omp parallel for schedule (dynamic)
    for (int i = 0; i < vehiclesSize; i++)
    {
        (*vehicles)[i]->determineAcceleration();
    }

#ifdef MEASURE_TIME
    TimeManager::stopClock("READYTORUN");
#endif //MEASURE_TIME

#else //_OPENMP not defined --------------------------------------------

#ifdef MEASURE_TIME
    TimeManager::startClock("READYTORUN");
#endif //MEASURE_TIME

    for_each(vehicles->begin(), vehicles->end(),
             mem_fun(&Vehicle::determineAcceleration));

#ifdef MEASURE_TIME
    TimeManager::stopClock("READYTORUN");
#endif //MEASURE_TIME

#endif //_OPENMP -------------------------------------------------------

#endif //EXCLUDE_VEHICLES
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 行動
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef EXCLUDE_VEHICLES

#ifdef _OPENMP //-------------------------------------------------------

#ifdef MEASURE_TIME
    TimeManager::startClock("RUN");
#endif //MEASURE_TIME

#pragma omp parallel for schedule (dynamic)
    for (int i = 0; i < vehiclesSize; i++)
    {
        (*vehicles)[i]->run();
    }

#ifdef MEASURE_TIME
    TimeManager::stopClock("RUN");
#endif //MEASURE_TIME

#else //_OPENMP not defined --------------------------------------------

#ifdef MEASURE_TIME
    TimeManager::startClock("RUN");
#endif //MEASURE_TIME

    for_each(vehicles->begin(), vehicles->end(),
             mem_fun(&Vehicle::run));

#ifdef MEASURE_TIME
    TimeManager::stopClock("RUN");
#endif //MEASURE_TIME

#endif //_OPENMP -------------------------------------------------------

#endif //EXCLUDE_VEHICLES
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 時系列データ出力
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef MEASURE_TIME
    TimeManager::startClock("WRITE_RESULT");
#endif //MEASURE_TIME

    writeRunInfo();
    if (GVManager::getFlag("FLAG_OUTPUT_TIMELINE_D")
        || GVManager::getFlag("FLAG_OUTPUT_TIMELINE_S"))
    {
        writeResult();
    }

#ifdef MEASURE_TIME
    TimeManager::stopClock("WRITE_RESULT");
#endif //MEASURE_TIME

    return true;
}

//======================================================================
void Simulator::writeResult() const
{
    const RMAPSI* signals
        = _roadMap->signals();
    _signalIO
        ->writeSignalsDynamicData(TimeManager::time(), signals);

    vector<Vehicle*>* vehicles = ObjManager::vehicles();
    _vehicleIO
        ->writeVehiclesDynamicData(TimeManager::time(), vehicles);
}

//======================================================================
void Simulator::writeRunInfo() const
{
    string fRunInfo;
    GVManager::getVariable("RESULT_RUN_INFO_FILE", &fRunInfo);
    ofstream ofs(fRunInfo.c_str(), ios::trunc);
    if (!ofs.fail())
    {
        ofs << TimeManager::step() << "\n"
            << TimeManager::unit();
        ofs.close();
    }
}

//======================================================================
RoadMap* Simulator::roadMap()
{
    return _roadMap;
}

//======================================================================
GenerateVehicleController* Simulator::generateVehicleController()
{
    return _genVehicleController;
}

//======================================================================
//GenerateVehicleRunningMode* Simulator::generateVehicleRunningMode()
//{
//    return _genVehicleRunningMode;
//}
