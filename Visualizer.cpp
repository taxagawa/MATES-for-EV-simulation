/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "Visualizer.h"
#include "Simulator.h"
#include "RoadMap.h"
#include "Drawer.h"
#include "GLColor.h"
#include "TimeManager.h"
#include "ObjManager.h"
#include "GVManager.h"
#include "AmuConverter.h"
#include "AmuStringOperator.h"
#include "GenerateVehicleController.h"
#include "VehicleFamily.h"
#include "Vehicle.h"
#include "Conf.h"
#include "autogl_mates.h"
#include <autogl.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <deque>
#include <climits>

using namespace std;

Simulator* Visualizer::_sim;

double Visualizer::_xmin = -100.0;
double Visualizer::_xmax =  100.0;
double Visualizer::_ymin = -100.0;
double Visualizer::_ymax =  100.0;

double Visualizer::_viewSize        = 100;
double Visualizer::_viewPositionX  = 0.0;
double Visualizer::_viewPositionY  = 0.0;
double Visualizer::_viewPositionZ  = 0.0;
double Visualizer::_viewDirectionX = 0.0;
double Visualizer::_viewDirectionY = 0.0;
double Visualizer::_viewDirectionZ = 1.0;
double Visualizer::_viewUpVectorX  = 0.0;
double Visualizer::_viewUpVectorY  = 1.0;
double Visualizer::_viewUpVectorZ  = 0.0;

double Visualizer::_svpSize        = 100;
double Visualizer::_svpPositionX  = 0.0;
double Visualizer::_svpPositionY  = 0.0;
double Visualizer::_svpPositionZ  = 0.0;
double Visualizer::_svpDirectionX = 0.0;
double Visualizer::_svpDirectionY = 0.0;
double Visualizer::_svpDirectionZ = 1.0;
double Visualizer::_svpUpVectorX  = 0.0;
double Visualizer::_svpUpVectorY  = 1.0;
double Visualizer::_svpUpVectorZ  = 0.0;

int Visualizer::_poseTime;
int Visualizer::_idleEventIsOn     = 0;
int Visualizer::_withCapturing     = 0;
int Visualizer::_outputTimelineD   = 0;
int Visualizer::_outputTimelineS   = 0;
int Visualizer::_outputInstrumentD = 0;
int Visualizer::_outputInstrumentS = 0;
int Visualizer::_outputGenCounter  = 0;
int Visualizer::_outputTripInfo    = 0;

int Visualizer::_timeUnitRate;
int Visualizer::_viewingTimeRate;
int Visualizer::_skipRunTimeRate;
int Visualizer::_frameNumber;

int Visualizer::_isInterId         = 0;
int Visualizer::_isCSId            = 0;
int Visualizer::_isCSValue         = 0;
int Visualizer::_isLaneId          = 0;
int Visualizer::_isLanesInter      = 1;
int Visualizer::_isLanesSection    = 1;
int Visualizer::_isSimpleMap       = 0;
int Visualizer::_isSubsectionShape = 1;
int Visualizer::_isSubnetwork      = 0;
int Visualizer::_isSubsectionId    = 0;
int Visualizer::_surfaceMode       = 0;
int Visualizer::_isSignals         = 1;
int Visualizer::_isRoadsideUnits   = 0;
int Visualizer::_connectorIdMode   = 0;

int Visualizer::_isVehicleId       = 0;
int Visualizer::_vehicleColorMode  = 0;
int Visualizer::_isVehicleSOC      = 0;

char Visualizer::_infoVehicleId[16];
char Visualizer::_infoIntersectionId[16];

int  Visualizer::_genVehicleType   = VehicleFamily::passenger();
char Visualizer::_genVehicleStart[16];
char Visualizer::_genVehicleGoal[16];
char Visualizer::_genVehicleParams[256];
char Visualizer::_genVehicleStopPoints[512]; 

//======================================================================
Visualizer::Visualizer()
{
    _sim = NULL;
    _viewSize
        = _svpSize
        = GVManager::getNumeric("VIS_VIEW_SIZE");

    _viewPositionX
        = _svpPositionX
        = GVManager::getNumeric("VIS_VIEW_CENTER_X");
    _viewPositionY
        = _svpPositionY
        = GVManager::getNumeric("VIS_VIEW_CENTER_Y");
    _viewPositionZ
        = _svpPositionZ
        = GVManager::getNumeric("VIS_VIEW_CENTER_Z");
 
    _viewDirectionX
       = _svpDirectionX
       = GVManager::getNumeric("VIS_VIEW_DIRECTION_X");
    _viewDirectionY
       = _svpDirectionY
       = GVManager::getNumeric("VIS_VIEW_DIRECTION_Y");
    _viewDirectionZ
       = _svpDirectionZ
       = GVManager::getNumeric("VIS_VIEW_DIRECTION_Z");

    _viewUpVectorX
       = _svpUpVectorX
       = GVManager::getNumeric("VIS_VIEW_UPVECTOR_X");
    _viewUpVectorY
       = _svpUpVectorY
       = GVManager::getNumeric("VIS_VIEW_UPVECTOR_Y");
    _viewUpVectorZ
       = _svpUpVectorZ
       = GVManager::getNumeric("VIS_VIEW_UPVECTOR_Z");

    _xmin = _viewPositionX - _viewSize - 10;
    _xmax = _viewPositionX + _viewSize + 10;
    _ymin = _viewPositionY - _viewSize - 10;
    _ymax = _viewPositionY + _viewSize + 10;

    if (GVManager::getMaxTime()<INT_MAX)
    {
        _poseTime = GVManager::getMaxTime();
    }
    else
    {
        _poseTime = INT_MAX;
    }

    _timeUnitRate = TimeManager::unit();
    // 1ステップごとの描画
    _viewingTimeRate = _skipRunTimeRate = _timeUnitRate;
    _frameNumber = 1;

    // 出力フラグの設定
    _outputTimelineD
        = GVManager::getFlag("FLAG_OUTPUT_TIMELINE_D")?1:0;
    _outputTimelineS
        = GVManager::getFlag("FLAG_OUTPUT_TIMELINE_S")?1:0;
    _outputInstrumentD
        = GVManager::getFlag("FLAG_OUTPUT_MONITOR_D")?1:0;
    _outputInstrumentS
        = GVManager::getFlag("FLAG_OUTPUT_MONITOR_S")?1:0;
    _outputGenCounter
        = GVManager::getFlag("FLAG_OUTPUT_GEN_COUNTER")?1:0;
    _outputTripInfo
        = GVManager::getFlag("FLAG_OUTPUT_TRIP_INFO")?1:0;

    // 描画フラグの初期化
    GVManager::setNewFlag("VIS_INTER_ID",
                          (_isInterId==1));
    GVManager::setNewFlag("VIS_CS_ID",
                          (_isCSId==1));
    GVManager::setNewFlag("VIS_CS_VALUE",
                          (_isCSValue==1));
    GVManager::setNewFlag("VIS_LANE_ID",
                          (_isLaneId==1));
    GVManager::setNewFlag("VIS_LANE_INTER",
                          (_isLanesInter==1));
    GVManager::setNewFlag("VIS_LANE_SECTION",
                          (_isLanesSection==1));
    GVManager::setNewFlag("VIS_SIMPLE_MAP",
                          (_isSimpleMap==1));
    GVManager::setNewFlag("VIS_SUBSECTION_SHAPE",
                          (_isSubsectionShape==1));
    GVManager::setNewFlag("VIS_SUBSECTION_ID",
                          (_isSubsectionId==1));
    GVManager::setNewFlag("VIS_SUBNETWORK",
                          (_isSubnetwork==1));
    GVManager::setNewNumeric("VIS_SURFACE_MODE",
                             (double)_surfaceMode);
    GVManager::setNewFlag("VIS_SIGNAL",
                          (_isSignals==1));
    GVManager::setNewFlag("VIS_ROADSIDE_UNIT",
                          (_isRoadsideUnits==1));
    GVManager::setNewNumeric("VIS_CONNECTOR_ID_MODE",
                             (double)_connectorIdMode);

    GVManager::setNewFlag("VIS_VEHICLE_ID",
                          (_isVehicleId==1));
    GVManager::setNewNumeric("VIS_VEHICLE_COLOR_MODE",
                             (double)_vehicleColorMode);

    // by uchida 2016/5/22
    GVManager::setNewFlag("VIS_VEHICLE_SOC",
                          (_isVehicleSOC==1));
}

//======================================================================
void Visualizer::setSimulator(Simulator* simulator)
{
    assert(simulator->hasInit());
    _sim = simulator;
}

//======================================================================
void Visualizer::viewRedrawCallback()
{
    // 地図の描画
    RoadMap* roadMap = _sim->roadMap();
    if (roadMap!=NULL)
    {

        if (!(GVManager::getFlag("VIS_SIMPLE_MAP")) && _isSubsectionShape)
        {
            // 地面を描画する
            GLColor::setGround();
            AutoGL_DrawQuadrangle(_xmin, _ymin, -2.0,
                                  _xmax, _ymin, -2.0,
                                  _xmax, _ymax, -2.0,
                                  _xmin, _ymax, -2.0);
        }
        // 道路を描画する
        RoadMapDrawer& roadMapDrawer = RoadMapDrawer::instance();
        roadMapDrawer.draw(*roadMap);
    }

    // 路側器の描画
    if (_isRoadsideUnits)
    {
        drawRoadsideUnits();
    }

    // 自動車の描画
    drawVehicles();

    // 時刻の描画
    AutoGL_SetColor(0,0,0);
    ulint presentTime = TimeManager::time()/1000;
    string timeString
        = AmuConverter::itos(presentTime, NUM_FIGURE_FOR_DRAW_TIME)
        + "[sec]";
    AutoGL_DrawStringAtScaledPosition(0.01, 0.97, timeString.c_str());
}

//======================================================================
void Visualizer::viewRenderCallback()
{
    // ビューのサイズ（中心から端までの長さ）
    AutoGL_SetViewSize(_viewSize);
    // ビューの中心
    AutoGL_SetViewCenter(_viewPositionX,
                         _viewPositionY,
                         _viewPositionZ);
    // 視線方向
    AutoGL_SetViewDirection(_viewDirectionX,
                            _viewDirectionY,
                            _viewDirectionZ);
    // 画面上方向
    AutoGL_SetViewUpVector(_viewUpVectorX,
                           _viewUpVectorY,
                           _viewUpVectorY);

    // 広域の場合はシンプル地図モードに
    if (_viewSize>1000)
    {
        GVManager::resetFlag("VIS_SIMPLE_MAP", true);
    }

    // シミュレータを停止する時刻の決定
    ulint maxTime = _poseTime;
    ulint mt = GVManager::getMaxTime();
    if (mt>100)
    {
        maxTime = mt;
    }

    string imgDir = "/tmp/mates_images/";
    GVManager::getVariable("RESULT_IMG_DIRECTORY", &imgDir);

    cout << "*** Run advmates-sim (off-screen rendering): max time=" << maxTime
         << " (dt=" << TimeManager::unit() << ") ***" << endl;

    // シミュレータを進める
    while (TimeManager::time()<maxTime)
    {
        _sim->run(TimeManager::time() + _viewingTimeRate);

        // ビューをファイルに出力する
        string fileName = imgDir;
        fileName += AmuConverter::itos(_frameNumber, 6);
        fileName += ".ppm";
        _frameNumber++;
        AutoGL_SetImageFileName(fileName.c_str());
        AutoGL_DrawView();
    }
}

//======================================================================
void Visualizer::drawRoadsideUnits()
{
    DetectorDrawer* detectorDrawer = &DetectorDrawer::instance();
    vector<DetectorUnit*>* detectorUnits = ObjManager::detectorUnits();

    for (unsigned int i=0; i<detectorUnits->size(); i++)
    {
        detectorDrawer->draw(*(*detectorUnits)[i]);
    }
}

//======================================================================
void Visualizer::drawVehicles()
{
    VehicleDrawer* vehicleDrawer = &VehicleDrawer::instance();
    vector<Vehicle*>* allVehicles = ObjManager::vehicles();

    // drawSimple用の図形サイズを決定
    double size = 0.01*AutoGL_GetViewSize();

    for (unsigned int i=0; i<allVehicles->size(); i++)
    {
        if ((*allVehicles)[i]->isAwayFromOriginNode())
        {
            if (GVManager::getFlag("VIS_SIMPLE_MAP"))
            {
                vehicleDrawer->drawSimple(*(*allVehicles)[i], size);
            }
            else
            {
                vehicleDrawer->draw(*(*allVehicles)[i]);
            }
        }
    }
}

//======================================================================
void Visualizer::renewFlag()
{
    // 描画フラグの更新
    GVManager::resetFlag("VIS_INTER_ID",
                         (_isInterId==1));
    GVManager::resetFlag("VIS_CS_ID",
                         (_isCSId==1));
    GVManager::resetFlag("VIS_CS_VALUE",
                         (_isCSValue==1));
    GVManager::resetFlag("VIS_LANE_ID",
                         (_isLaneId==1));
    GVManager::resetFlag("VIS_LANE_INTER",
                         (_isLanesInter==1));
    GVManager::resetFlag("VIS_LANE_SECTION",
                         (_isLanesSection==1));
    GVManager::resetFlag("VIS_SIMPLE_MAP",
                         (_isSimpleMap==1));
    GVManager::resetFlag("VIS_SUBSECTION_SHAPE",
                         (_isSubsectionShape==1));
    GVManager::resetFlag("VIS_SUBSECTION_ID",
                         (_isSubsectionId==1));
    GVManager::resetFlag("VIS_SUBNETWORK",
                         (_isSubnetwork==1));
    GVManager::resetNumeric("VIS_SURFACE_MODE",
                            (double)_surfaceMode);
    GVManager::resetFlag("VIS_SIGNAL",
                         (_isSignals==1));
    GVManager::resetFlag("VIS_ROADSIDE_UNIT",
                         (_isRoadsideUnits==1));
    GVManager::resetNumeric("VIS_CONNECTOR_ID_MODE",
                            (double)_connectorIdMode);

    GVManager::resetFlag("VIS_VEHICLE_ID",
                         (_isVehicleId==1));
    GVManager::resetNumeric("VIS_VEHICLE_COLOR_MODE",
                            (double)_vehicleColorMode);

    // by uchida 2016/5/22
    GVManager::resetFlag("VIS_VEHICLE_SOC",
                         (_isVehicleSOC==1));

    // シミュレータの出力フラグを更新
    GVManager::resetFlag("FLAG_OUTPUT_TIMELINE_D",
                         (_outputTimelineD==1));
    GVManager::resetFlag("FLAG_OUTPUT_TIMELINE_S",
                         (_outputTimelineS==1));
    GVManager::resetFlag("FLAG_OUTPUT_MONITOR_D",
                         (_outputInstrumentD==1));
    GVManager::resetFlag("FLAG_OUTPUT_MONITOR_S",
                         (_outputInstrumentS==1));
    GVManager::resetFlag("FLAG_OUTPUT_GEN_COUNTER",
                         (_outputGenCounter==1));
    GVManager::resetFlag("FLAG_OUTPUT_TRIP_INFO",
                         (_outputTripInfo==1));
}

//======================================================================
void Visualizer::timeIncrement()
{
    // シミュレータを進める
    TimeManager::setUnit(_timeUnitRate);
    if (_viewingTimeRate>0)
    {
        _sim->run(TimeManager::time() + _viewingTimeRate);
    }
    drawButtonCallback();

    // 静止画の保存
    if (_withCapturing)
    {
        string fileName = "/tmp/mates_images/";
        GVManager::getVariable("RESULT_IMG_DIRECTORY", &fileName);
        fileName += AmuConverter::itos(_frameNumber, 6);
        fileName += ".ppm";
        _frameNumber++;
        AutoGL_SaveViewImageToPPMFile(fileName.c_str());
    }
}

//======================================================================
void Visualizer::run()
{
    if (_sim->checkLaneError())
        return;

    // 描画・出力フラグを更新する
    renewFlag();
    while (TimeManager::time()<static_cast<unsigned int>(_poseTime))
    {
        timeIncrement();
    }
}

//======================================================================
void Visualizer::generateVehicleManual()
{
    // start,goalともODノードが実在するかどうかは
    // Simulator::generateVehicleManual()で判断する．

    // startIdの入力は必須
    if (strcmp(_genVehicleStart, "")==0)
    {
        cout << "input origin node ID." << endl;
        return;
    }
    string startId
        = AmuConverter::formatId(_genVehicleStart,
                                 NUM_FIGURE_FOR_INTERSECTION);

    // goalIdは入力されなければランダム
    string goalId;
    if (strcmp(_genVehicleGoal, "")==0)
    {
        goalId = "******";
    }
    else
    {
        goalId
            = AmuConverter::formatId(_genVehicleGoal,
                                     NUM_FIGURE_FOR_INTERSECTION);
    }

    string str;
    vector<string> tokens;

    // 経路選択パラメータの切り出し
    vector<double> params;
    str = _genVehicleParams;
    AmuStringOperator::getAdjustString(&str);
    AmuStringOperator::getTokens(&tokens, str, ',');
    if (str!="")
    {
        for (unsigned int i=0; i<tokens.size(); i++)
        {
            // isNumeric 
            params.push_back(AmuConverter::strtod(tokens[i]));
        }
        tokens.clear();
    }

    // 経由地の切り出し
    vector<string> stopPoints;
    str = _genVehicleStopPoints;
    AmuStringOperator::getAdjustString(&str);
    AmuStringOperator::getTokens(&tokens, str, ',');
    if (str!="")
    {
        for (unsigned int i=0; i<tokens.size(); i++)
        {
            stopPoints.push_back(
                AmuConverter::formatId(tokens[i],
                                       NUM_FIGURE_FOR_INTERSECTION));
        }
    }

    // 入力された情報から車両を生成
    _sim->generateVehicleController()
        ->generateVehicleManual(startId, goalId, stopPoints,
                                _genVehicleType, params);

}

//======================================================================
void Visualizer::showVehicleInfo()
{
    string vehicleId = AmuConverter::formatId(_infoVehicleId,
                                              NUM_FIGURE_FOR_VEHICLE);
    Vehicle* tmpVehicle = ObjManager::vehicle(vehicleId);
    if (tmpVehicle==NULL)
    {
        cerr << "cannot find vehicle " << vehicleId << endl;
    }
    else
    {
        // comented by uchida 2016/5/22
        // 今後ここでSOCを表示しなくては
        tmpVehicle->print();
    }
}

//======================================================================
void Visualizer::showIntersectionInfo()
{
    string intersectionId
        = AmuConverter::formatId(_infoIntersectionId,
                                 NUM_FIGURE_FOR_INTERSECTION);
    Intersection* tmpInter
        = _sim->roadMap()->intersection(intersectionId);
    if (!tmpInter)
    {
        cerr << "cannot find intersection " << intersectionId << endl;
    }
    else
    {
        tmpInter->printDetail(dynamic_cast<ODNode*>(tmpInter) != NULL);
    }
}

//======================================================================
void Visualizer::searchVehicle()
{
    string vehicleId
        = AmuConverter::formatId(_infoVehicleId,
                                 NUM_FIGURE_FOR_VEHICLE);
    Vehicle* tmpVehicle = ObjManager::vehicle(vehicleId);
    if (!tmpVehicle)
    {
        cerr << "cannot find vehicle " << vehicleId << endl;
    }
    else
    {
        // ビューの中心を移動する
        AutoGL_SetViewCenter(tmpVehicle->x(),
                             tmpVehicle->y(),
                             tmpVehicle->z());
        AutoGL_DrawView();
    }
}

//======================================================================
void Visualizer::searchIntersection()
{
    string intersectionId
        = AmuConverter::formatId(_infoIntersectionId,
                                 NUM_FIGURE_FOR_INTERSECTION);
    Intersection* tmpInter
        = _sim->roadMap()->intersection(intersectionId);
    if (!tmpInter)
    {
        cerr << "cannot find intersection " << intersectionId << endl;
    }
    else
    {
        // ビューの中心を移動する
        AutoGL_SetViewCenter(tmpInter->center().x(),
                             tmpInter->center().y(),
                             tmpInter->center().z());
        AutoGL_DrawView();
    }
}

//======================================================================
void Visualizer::drawButtonCallback()
{
    // 描画・出力フラグを更新する
    renewFlag();

    // シンプル地図モードでは視線方向をリセット
    if (GVManager::getFlag("VIS_SIMPLE_MAP"))
    {
        AutoGL_SetViewDirection(0, 0, 1);
        AutoGL_SetViewUpVector(0, 1, 0);
    }
    AutoGL_DrawView();
}

//======================================================================
void Visualizer::printViewingParamsCallback()
{
    double size;
    double cx, cy, cz;
    double dx, dy, dz;
    double ux, uy, uz;

    size = AutoGL_GetViewSize();
    AutoGL_GetViewCenter(&cx, &cy, &cz);
    AutoGL_GetViewDirection(&dx, &dy, &dz);
    AutoGL_GetViewUpVector(&ux, &uy, &uz);

    cout << "*** Viewing Params ***" << endl;
    cout << "ViewSize: " << size << endl;
    cout << "ViewCenter: ("
         << cx << ", " << cy << ", " << cz << ")" << endl;
    cout << "ViewDirection: ("
         << dx << ", " << dy << ", " << dz << ")" << endl;
    cout << "ViewUpVector: ("
         << ux << ", " << uy << ", " << uz << ")" << endl;
}

//======================================================================
void Visualizer::setViewingParamsCallback()
{
    _viewSize = _svpSize;

    _viewPositionX  = _svpPositionX;
    _viewPositionY  = _svpPositionY;
    _viewPositionZ  = _svpPositionZ;

    _viewDirectionX = _svpDirectionX;
    _viewDirectionY = _svpDirectionY;
    _viewDirectionZ = _svpDirectionZ;

    _viewUpVectorX  = _svpUpVectorX;
    _viewUpVectorY  = _svpUpVectorY;
    _viewUpVectorZ  = _svpUpVectorZ;

    /* 更新されたパラメータで再描画する */
    AutoGL_SetViewSize(_viewSize);
    AutoGL_SetViewCenter(_viewPositionX,
                         _viewPositionY,
                         _viewPositionZ);
    AutoGL_SetViewDirection(_viewDirectionX,
                            _viewDirectionY,
                            _viewDirectionZ);
    AutoGL_SetViewUpVector(_viewUpVectorX,
                           _viewUpVectorY,
                           _viewUpVectorY);

    AutoGL_DrawView();
}

//======================================================================
void Visualizer::quitButtonCallback()
{
    if (_outputTripInfo)
    {
        VehicleIO::instance().writeAllVehiclesDistanceData();
    }
    exit(0);
}

//======================================================================
void Visualizer::resetButtonCallback()
{
    if (_sim->hasInit())
    {
        cout << "inited" << endl;
    }
    drawButtonCallback();
}

//======================================================================
void Visualizer::autoIncrementButtonCallback()
{
    if (_idleEventIsOn)
    {
        AutoGL_SetIdleEventCallback(0);
        _idleEventIsOn = false;
    }
    else
    {
        AutoGL_SetIdleEventCallback(timeIncrement);
        _idleEventIsOn = true;
    }
}

//======================================================================
void Visualizer::skipRunButtonCallback()
{
    int saveTimeRate = _viewingTimeRate;
    _viewingTimeRate = _skipRunTimeRate;
    run();
    _viewingTimeRate = saveTimeRate;
}

//======================================================================
void Visualizer::visualize()
{
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // AutoGLのウィンドウを用意する

    // ビューのサイズ（中心から端までの長さ）
    AutoGL_SetViewSize(_viewSize);
    // ビューの中心
    AutoGL_SetViewCenter(_viewPositionX,
                         _viewPositionY,
                         _viewPositionZ);
    // 視線方向
    AutoGL_SetViewDirection(_viewDirectionX,
                            _viewDirectionY,
                            _viewDirectionZ);
    // 画面上方向
    AutoGL_SetViewUpVector(_viewUpVectorX,
                           _viewUpVectorY,
                           _viewUpVectorY);

    // 背景色の設定
    GLColor::setBackground();

    // コンターマップの用意
    /* 赤->青の順 */
    AutoGL_SetContourMap_RB();

    // ドラッグ有効
    AutoGL_EnableDragInMode3D();

    // コールバックを登録する
    AutoGL_SetDefaultCallbackInMode3D (NULL);
    AutoGL_SetViewRedrawCallback(viewRedrawCallback);
    AutoGL_SetBatchProcessCallback(viewRenderCallback);

    // アイドルイベント処理を有効にする
    AutoGL_EnableIdleEvent();

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // View Controlパネル
    AutoGL_AddGroup(" View Control ");

    // 拡大縮小・移動・回転などのボタンを用意する
    AutoGL_SetPanelInMode3D();
    AutoGL_SetMode3D(AUTOGL_MODE_3D_TRANSLATE);

    AutoGL_AddComment();
    AutoGL_SetLabel("Viewing Parameters");
    AutoGL_AddCallback(printViewingParamsCallback,
                       "printViewingParamsCallback");
    AutoGL_SetLabel("Print");
    AutoGL_AddComment();
    AutoGL_SetLabel("Set Parameters");
    AutoGL_AddReal(&_svpSize, "_svpSize");
    AutoGL_SetLabel("ViewSize");
    AutoGL_AddReal(&_svpPositionX, "_svpPositionX");
    AutoGL_SetLabel("ViewCenter_X");
    AutoGL_AddReal(&_svpPositionY, "_svpPositionY");
    AutoGL_SetLabel("ViewCenter_Y");
    AutoGL_AddReal(&_svpPositionZ, "_svpPositionZ");
    AutoGL_SetLabel("ViewCenter_Z");
    AutoGL_AddReal(&_svpDirectionX, "_svpDirectionX");
    AutoGL_SetLabel("ViewDirection_X");
    AutoGL_AddReal(&_svpDirectionY, "_svpDirectionY");
    AutoGL_SetLabel("ViewDirection_Y");
    AutoGL_AddReal(&_svpDirectionZ, "_svpDirectionZ");
    AutoGL_SetLabel("ViewDirection_Z");
    AutoGL_AddReal(&_svpUpVectorX, "_svpUpVectorX");
    AutoGL_SetLabel("ViewUpVector_X");
    AutoGL_AddReal(&_svpUpVectorY, "_svpUpVectorY");
    AutoGL_SetLabel("ViewUpVector_Y");
    AutoGL_AddReal(&_svpUpVectorZ, "_svpUpVectorZ");
    AutoGL_SetLabel("ViewUpVector_Z");
    AutoGL_AddCallback(setViewingParamsCallback,
                       "setViewingParamsCallback");
    AutoGL_SetLabel("Set");
  
    AutoGL_AddComment();
    AutoGL_AddCallback(drawButtonCallback, "drawButtonCallback");
    AutoGL_SetLabel("Draw");
    AutoGL_AddCallback(quitButtonCallback, "quitButtonCallback");
    AutoGL_SetLabel("Quit");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Visualizer Controlパネル
    AutoGL_AddGroup(" Visualizer Control ");

    AutoGL_AddComment();
    AutoGL_SetLabel("General");
    AutoGL_AddBoolean(&_isSimpleMap, "_isSimpleMap");
    AutoGL_SetLabel("Show Simple Map");

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Vehicle");
    AutoGL_AddBoolean(&_isVehicleId, "_isVehicleId");
    AutoGL_SetLabel("Show ID");
    // by uchida 2016/5/22
    AutoGL_AddBoolean(&_isVehicleSOC, "_isVehicleSOC");
    AutoGL_SetLabel("Show SOC");
    //
    AutoGL_AddInteger(&_vehicleColorMode, "_vehicleColorMode");
    AutoGL_SetLabel("Vehicle Color Mode");
    AutoGL_AddIntegerItem("vehicle family");
    AutoGL_AddIntegerItem("average velocity");

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Intersection");
    AutoGL_AddBoolean(&_isInterId, "_isInterId");
    AutoGL_SetLabel("Show ID");

    // by uchida 2016/5/23
    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Charging Station");
    AutoGL_AddBoolean(&_isCSId, "_isCSId");
    AutoGL_SetLabel("Show ID");
    AutoGL_AddBoolean(&_isCSValue, "_isCSValue");
    AutoGL_SetLabel("Show Value");

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Lane");
    AutoGL_AddBoolean(&_isLaneId, "_isLaneId");
    AutoGL_SetLabel("Show ID");
    AutoGL_AddBoolean(&_isLanesInter, "_isLanesInter");
    AutoGL_SetLabel("Show Intersection Lane");
    AutoGL_AddBoolean(&_isLanesSection, "_isLanesSection");
    AutoGL_SetLabel("Show Section Lane");

#ifdef NO_USE
    /*
     * 自動車交通流のみを扱う場合には，
     * Subsectionの描画方法の選択に意味はない
     */
    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Subsection");
    AutoGL_AddBoolean(&_isSubsectionShape, "_isSubsectionShape");
    AutoGL_SetLabel("Show Shape");
    AutoGL_AddBoolean(&_isSubnetwork, "_isSubnetwork");
    AutoGL_SetLabel("Show Subnetwork");
    AutoGL_AddBoolean(&_isSubsectionId, "_isSubsectionId");
    AutoGL_SetLabel("Show ID");
    AutoGL_AddInteger(&_surfaceMode, "_surfaceMode");
    AutoGL_SetLabel("Surface Mode");
    AutoGL_AddIntegerItem("attribute");
    AutoGL_AddIntegerItem("access right");
#endif

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Signal");
    AutoGL_AddBoolean(&_isSignals, "_isSignals");
    AutoGL_SetLabel("Show");

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Roadside Unit");
    AutoGL_AddBoolean(&_isRoadsideUnits, "_isRoadsideUnits");
    AutoGL_SetLabel("Show");

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Connector");
    AutoGL_AddInteger(&_connectorIdMode, "_connectorIdMode");
    AutoGL_SetLabel("Show Connector ID");
    AutoGL_AddIntegerItem("Disable");
    AutoGL_AddIntegerItem("Global ID");
    AutoGL_AddIntegerItem("Local ID");

    AutoGL_AddComment();
    AutoGL_AddCallback(drawButtonCallback, "drawButtonCallback");
    AutoGL_SetLabel("Draw");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Simulator Controlパネル
    AutoGL_AddGroup(" Simulator Control ");

    AutoGL_AddComment();
    AutoGL_SetLabel(" Time Flow Control ");
    AutoGL_AddCallback(timeIncrement, "timeIncrement");
    AutoGL_SetLabel(" Time Increment ");
    AutoGL_AddCallback(autoIncrementButtonCallback,
                       "autoIncrementButtonCallback");
    AutoGL_SetLabel("Auto Time Increment");
  
    AutoGL_AddInteger(&_poseTime, "_poseTime");
    AutoGL_SetLabel("Time To Pose");
    AutoGL_AddCallback(run, "run");
    AutoGL_SetLabel("Continuous Run");

    AutoGL_AddInteger(&_skipRunTimeRate, "_skipRunTimeRate");
    AutoGL_SetLabel("Skip Time Rate");
    AutoGL_AddCallback(&skipRunButtonCallback,
                       "skipRunButtonCallback");
    AutoGL_SetLabel("Skip Continuous Run");

    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Output File");
    AutoGL_AddBoolean(&_outputTimelineD, "_outputTimelineD");
    AutoGL_SetLabel("Output Timeline Detail Data");
    AutoGL_AddBoolean(&_outputTimelineS, "_outputTimelineS");
    AutoGL_SetLabel("Output Timeline Statistic Data");
    AutoGL_AddBoolean(&_outputInstrumentD, "_outputInstrumentD");
    AutoGL_SetLabel("Output Monitoring Detail Data");
    AutoGL_AddBoolean(&_outputInstrumentS, "_outputInstrumentS");
    AutoGL_SetLabel("Output Monitoring Statistic Data");
    AutoGL_AddBoolean(&_outputGenCounter, "_outputGenCounter");
    AutoGL_SetLabel("Output Generate Counter Data");
    AutoGL_AddBoolean(&_outputTripInfo, "_outputTripInfo");
    AutoGL_SetLabel("Output Trip Info Data");
    AutoGL_AddBoolean(&_withCapturing, "_withCapturing");
    AutoGL_SetLabel("With Capturing");

    AutoGL_AddComment();
    AutoGL_SetPanelForSave();

    AutoGL_AddComment();
    AutoGL_AddCallback(quitButtonCallback, "quitButtonCallback");
    AutoGL_SetLabel("Quit");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Vehicle Generation パネル
    AutoGL_AddGroup(" Agent Generation ");
    AutoGL_AddComment();
    AutoGL_SetLabel("Vehicle");
    AutoGL_AddInteger(&_genVehicleType, "_genVehicleType");
    AutoGL_SetLabel("Type ID ");
    AutoGL_AddString(_genVehicleStart, "_genVehicleStart", 16);
    AutoGL_SetLabel("Origin ");
    AutoGL_AddString(_genVehicleGoal, "_genVehicleGoal", 16);
    AutoGL_SetLabel("Destination ");
    AutoGL_AddString(_genVehicleParams, "_genVehicleParams", 256);
    AutoGL_SetLabel("Routing Parameters");
    AutoGL_AddString(_genVehicleStopPoints, "_genVehicleStopPoints", 512);
    AutoGL_SetLabel("Stop Points");
    AutoGL_AddCallback(generateVehicleManual, "generateVehicleManual");
    AutoGL_SetLabel("Manually Generate");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Detail Information パネル
    AutoGL_AddGroup(" Detail Information ");
    AutoGL_AddComment();
    AutoGL_SetLabel("Vehicle");
    AutoGL_AddString(_infoVehicleId, "_infoVehicleId", 16);
    AutoGL_SetLabel("Vehicle ID");
    AutoGL_AddCallback(searchVehicle, "searchVehicle");
    AutoGL_SetLabel("Search Vehicle");
    AutoGL_AddCallback(showVehicleInfo, "showVehicleInfo");
    AutoGL_SetLabel("Show Vehicle Info");
    AutoGL_AddComment();
    AutoGL_AddComment();
    AutoGL_SetLabel("Intersection");
    AutoGL_AddString(_infoIntersectionId, "_infoIntersectionId", 16);
    AutoGL_SetLabel("Intersection ID");
    AutoGL_AddCallback(searchIntersection, "searchIntersection");
    AutoGL_SetLabel("Search Intersection");
    AutoGL_AddCallback(showIntersectionInfo, "showIntersectionInfo");
    AutoGL_SetLabel("Show Intersection Info");
}
