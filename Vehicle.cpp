/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "VehicleRunningMode.h"
#include "Vehicle.h"
#include "RoadMap.h"
#include "LaneBundle.h"
#include "Intersection.h"
#include "ODNode.h"
#include "Section.h"
#include "Lane.h"
#include "RoadEntity.h"
#include "TimeManager.h"
#include "Random.h"
#include "Route.h"
#include "ARouter.h"
#include "Router.h"
#include "LocalLaneRoute.h"
#include "LocalLaneRouter.h"
#include "Signal.h"
#include "SignalColor.h"
#include "VirtualLeader.h"
#include "GVManager.h"
#include "AmuPoint.h"
#include "AmuConverter.h"
#include "VehicleEV.h"
#include <cassert>
#include <cmath>
#include <algorithm>

using namespace std;

//======================================================================
Vehicle::Vehicle():_id()
{

    _bodyLength = 4.400;
    _bodyWidth  = 1.830;
    _bodyHeight = 1.315;
    _bodyColorR = 1.0;
    _bodyColorG = 0.0;
    _bodyColorB = 0.0;

    _roadMap      = NULL;
    _intersection = NULL;
    _prevIntersection = NULL;
    _section      = NULL;
    _lane         = NULL;
    _nextLane     = NULL;
    _prevLane     = NULL;

    _length        = 0;
    _oldLength     = -10;
    _totalLength   = 0;
    _tripLength    = 0;
    _error         = 0;
    _velocity      = 0;
    _velocityHistory.clear();
    _errorVelocity = 0;
    _accel         = 0;
    _vMax          = 60.0/60.0/60.0;
    _startTime     = 0;
    _startChargingTime = 0;
    _restartTime   = 0;
    _genTime       = TimeManager::time();

    _rnd = Random::randomGenerator();

    _blinker.setNone();

    _laneShifter.setVehicle(this);

    _strictCollisionCheck
        = (GVManager::getNumeric("STRICT_COLLISION_CHECK") == 1);
    _entryTime     = 0;
    _isNotifying   = false;
    _hasPaused     = false;
    _sleepTime     = 0;
    _isNotifySet   = false;
    _isUnnotifySet = false;

    _route  = new Route();
    _router = new Router();
    _router->setVehicle(this);

    _localRouter.setVehicle(this);
    _localRouter.setLocalRoute(&_localRoute);
    _localRoute.clear();

    // by uchida 2016/5/10
    // 充電フラグは普通車の場合常にfalse
    _isCharge = false;
    _onCharging = false;

    _waiting = false;
    _swapTime = 0;

    // by uchida 2016/5/22
    // EV側では基本的に_SOCを参照しない
    _SOC = -1;

    _startSN  = NULL;
    _startSec = NULL;

}

//======================================================================
void Vehicle::setId(const string& id)
{
    _id = id;
}

//======================================================================
Vehicle::~Vehicle()
{
    if(_router) delete _router;
    if(_route) delete _route;

    if(_intersection) _intersection->eraseWatchedVehicle(this);
    if(_section) _section->eraseWatchedVehicle(this);

    for (int i=0; i<static_cast<signed int>(_leaders.size()); i++)
    {
        delete _leaders[i];
    }
    _leaders.clear();

    Random::releaseRandomGenerator(_rnd);
}

//======================================================================
const string&  Vehicle::id() const
{
    return _id;
}

//======================================================================
VehicleType Vehicle::type() const
{
    return _type;
}

//======================================================================
void Vehicle::setType(VehicleType type)
{
    _type = type;
}

//======================================================================
// by uchida
void Vehicle::setRunningMode(VehicleRunningMode* runningMode)
{
    _runningMode = runningMode;
}

//======================================================================
double Vehicle::bodyWidth() const
{
    return _bodyWidth;
}

//======================================================================
double Vehicle::bodyLength() const
{
    return _bodyLength;
}

//======================================================================
double Vehicle::bodyHeight() const
{
    return _bodyHeight;
}

//======================================================================
void Vehicle::setBodySize(double length, double width, double height)
{
    _bodyLength = length;
    _bodyWidth = width;
    _bodyHeight = height;
}

//======================================================================
void Vehicle::setPerformance(double accel, double brake)
{
    assert(accel>0 && brake<0);
    _maxAcceleration = accel*1.0e-6;
    _maxDeceleration = brake*1.0e-6;
}

//======================================================================
void Vehicle::setBodyColor(double r, double g, double b)
{
    _bodyColorR = r;
    _bodyColorG = g;
    _bodyColorB = b;
}
//======================================================================
void Vehicle::getBodyColor(double* result_r,
                           double* result_g,
                           double* result_b) const
{
    *result_r = _bodyColorR;
    *result_g = _bodyColorG;
    *result_b = _bodyColorB;
}

//======================================================================
string Vehicle::sSOC() const
{
    double tmpSOC = _SOC * 100000;
    return AmuConverter::itos((int)tmpSOC, 6);
}

//======================================================================
double Vehicle::SOC()
{
    return _SOC;
}

//======================================================================
double Vehicle::length() const
{
    return _length;
}

//======================================================================
double Vehicle::oldLength() const
{
    return _oldLength;
}

//======================================================================
double Vehicle::totalLength() const
{
    return _totalLength;
}

//======================================================================
double Vehicle::tripLength() const
{
    return _tripLength;
}

//======================================================================
double Vehicle::x() const
{
    AmuVector pv(_lane->beginConnector()->point(),
                 _lane->endConnector()->point());
    pv.normalize();

    double x = _lane->beginConnector()->point().x()+ _length*pv.x();

    pv.revoltXY(M_PI_2);
    x += _error * pv.x();

    return x;
}

//======================================================================
double Vehicle::y() const
{
    AmuVector pv(_lane->beginConnector()->point(),
                 _lane->endConnector()->point());
    pv.normalize();

    double y = _lane->beginConnector()->point().y()+ _length*pv.y();

    pv.revoltXY(M_PI_2);
    y += _error * pv.y();

    return y;
}

//======================================================================
double Vehicle::z() const
{
    AmuVector pv(_lane->beginConnector()->point(),
                 _lane->endConnector()->point());
    pv.normalize();

    double z = _lane->beginConnector()->point().z()+ _length*pv.z();

    pv.revoltXY(M_PI_2);
    z += _error * pv.z();

    return z;
}

//======================================================================
double Vehicle::phi() const
{
  double phi = atan(_lane->gradient()/100.0)*180*M_1_PI;
  return phi;
}

//======================================================================
LaneBundle* Vehicle::laneBundle() const
{
    assert((!_section && _intersection)
           || (_section && !_intersection));

    if (_section)
    {
        return _section;
    }
    else if (_intersection)
    {
        return _intersection;
    }
    else
    {
        return NULL;
    }
}

//======================================================================
Section* Vehicle::section() const
{
    return _section;
}

//======================================================================
Intersection* Vehicle::intersection() const
{
    return _intersection;
}

//======================================================================
Lane* Vehicle::lane() const
{
    return _lane;
}

//======================================================================
bool Vehicle::isAwayFromOriginNode() const
{
    bool check = true;

    if (_route->lastPassedIntersectionIndex()==0
        && _route->start()==_router->start()
        && _totalLength
        < GVManager::getNumeric("NO_OUTPUT_LENGTH_FROM_ORIGIN_NODE"))
    {
        check = false;
    }
    return check;
}

//======================================================================
double Vehicle::velocity() const
{
    return _velocity;
}

//======================================================================
double Vehicle::aveVelocity() const
{
    if (_velocityHistory.empty())
    {
        return 1.0;
    }

    double sum = 0.0;
    for (unsigned int i=0; i<_velocityHistory.size(); i++)
    {
        sum += _velocityHistory[i];
    }
    return sum / _velocityHistory.size();
}

//======================================================================
double Vehicle::accel() const
{
    return _accel;
}

//======================================================================
const AmuVector Vehicle::directionVector() const
{
    assert(_lane!=NULL);
    return _lane->directionVector();
}

//======================================================================
void Vehicle::notify()
{
    if (_section)
    {
        _section->addWatchedVehicle(this);
    }
    else
    {
        _intersection->addWatchedVehicle(this);
    }
    _isNotifying = true;
}

//======================================================================
void Vehicle::unnotify()
{
    if (_section)
    {
        _section->eraseWatchedVehicle(this);
    }
    else
    {
        _intersection->eraseWatchedVehicle(this);
    }
    _isNotifying = false;
}

//======================================================================
Blinker Vehicle::blinker() const
{
    return _blinker;
}

//======================================================================
int Vehicle::directionFrom() const
{
    Intersection* inter;
    if (_intersection)
    {
        inter = _intersection;
    }
    else
    {
        inter = _section->intersection(_section->isUp(_lane));
    }
    if (inter)
    {
        const vector<Lane*>* liInter = _localRoute.lanesInIntersection();
        assert(inter->isMyLane((*liInter)[0]));
        return inter->direction((*liInter)[0]->beginConnector());
    }
    else
    {
        return -1;
    }
}

//======================================================================
int Vehicle::directionTo() const
{
    Intersection* inter;
    if (_intersection)
    {
        inter = _intersection;
    }
    else
    {
        inter = _section->intersection(_section->isUp(_lane));
    }
    if (inter)
    {
        const vector<Lane*>* liInter = _localRoute.lanesInIntersection();
        assert(inter->isMyLane((*liInter)[liInter->size()-1]));
        return inter
            ->direction((*liInter)[liInter->size()-1]->endConnector());
    }
    else
    {
        return -1;
    }
}

//======================================================================
VehicleLaneShifter& Vehicle::laneShifter()
{
    return _laneShifter;
}

//======================================================================
bool Vehicle::isSleep() const
{
    return (_sleepTime>0);
}

//======================================================================
void Vehicle::setStartTime(ulint startTime)
{
    _startTime = startTime;
}

//======================================================================
ulint Vehicle::startTime() const
{
    return _startTime;
}

// by uchida 2017/2/8
// CSで充電中のEVは旅行時間が充電時間分長く見積もられるため
// それを回避するためvehicleTrip.txtにCS入庫時刻と出庫時刻を登録したい
//======================================================================
void Vehicle::setStartChargingTime(ulint startChargingTime)
{
    _startChargingTime = startChargingTime;
}

//======================================================================
ulint Vehicle::startChargingTime() const
{
    return _startChargingTime;
}

//======================================================================
void Vehicle::setRestartTime(ulint restartTime)
{
    _restartTime = restartTime;
}

//======================================================================
ulint Vehicle::restartTime() const
{
    return _restartTime;
}

//======================================================================
ulint Vehicle::startStep() const
{
    return _startTime / TimeManager::unit();
}

//======================================================================
ulint Vehicle::genTime() const
{
    return _genTime;
}

//======================================================================
const vector<VirtualLeader*>* Vehicle::virtualLeaders() const
{
    return &_leaders;
}

//======================================================================
void Vehicle::addStrandedEVs(VehicleEV* strandedEV)
{
    strandedEV->_lane->addStrandedVehicles(strandedEV);
//    strandedEV->_roadMap->addStrandedVehicles(strandedEV);
}

//======================================================================
bool Vehicle::isCharge() const
{
    return _isCharge;
}

// by uchida 2016/5/25?
//======================================================================
CSNode* Vehicle::target() const
{
    vector<CSNode*> csNodes = _roadMap->csNodes();
    for (int i = 0; i < csNodes.size(); i++)
    {
        if (csNodes[i]->id() == _stopCS)
            return csNodes[i];
    }

    return NULL;
}

// by uchida 2016/5/26
//======================================================================
void Vehicle::_searchCSRand()
{
    vector<CSNode*> csNodes = _roadMap->csNodes();
    _stopCS =  (csNodes[(int)(Random::uniform(_rnd)*csNodes.size())])->id();
}

//======================================================================
void Vehicle::_searchCSEuclid()
{
    // by uchida 2016/5/27
    // 全てのCSNodeというのはそのうちかえないとね
    vector<CSNode*> csNodes = _roadMap->csNodes();
    double min = 1000000;// 十分大きいということで
    int min_index = -1;

    for (int i = 0; i < csNodes.size(); i++)
    {
        double tmp
            = pow( (csNodes[i]->center().x() - x() ),2)
            + pow( (csNodes[i]->center().y() - y() ),2)
            + pow( (csNodes[i]->center().z() - z() ),2);
        tmp = sqrt(tmp);

        if (min >= tmp)
        {
            min = tmp;
            min_index = i;
        }
    }
    assert(min_index >= 0);

    // by uchida 2016/5/25
    // 目的地のほうがより近ければそちらへ
    // でもその先に別の目的地があると仮定するならやっぱ不要かもな
    double tmp = pow( (_route->goal()->center().x() - x() ),2)
               + pow( (_route->goal()->center().y() - y() ),2)
               + pow( (_route->goal()->center().z() - z() ),2);
    tmp = sqrt(tmp);

    if (min >= tmp)
    {
        _stopCS = _route->goal()->id();
    }
    else
    {
        _stopCS = csNodes[min_index]->id();
    }
}

//======================================================================
void Vehicle::_searchCSCost()
{
    Intersection* start;
    start = _intersection->next(_intersection->direction(_section));
    Intersection* past = const_cast<Intersection*>(_intersection);

    Intersection* goal;
    vector<CSNode*> csNodes = _roadMap->csNodes();
    double min = 1000000;// 十分大きいということで
    int min_index = -1;
    int step;
    double GV;

    for (int i = 0; i < csNodes.size(); i++)
    {
        step = 10000;
        goal = dynamic_cast<Intersection*>(csNodes[i]);
        // GVを返す的なイメージなんだよなぁ
        GV = _router->searchSegmentGV(start, goal, past, step, goal->id());

        if (min >= GV)
        {
            min = GV;
            min_index = i;
        }
    }
    assert(min_index >= 0);

    step = 10000;
    goal = const_cast<Intersection*>(_router->goal());
    // GVを返す的なイメージなんだよなぁ
    GV = _router->searchSegmentGV(start, goal, past, step,"");

    if (min >= GV)
    {
        _stopCS = goal->id();
    }
    else
    {
        _stopCS = csNodes[min_index]->id();
    }
}

//======================================================================
std::string Vehicle::_searchCSCost(RoadMap* roadMap,
                                   const Section* section,
                                   const Intersection* intersection)
{
    _roadMap = roadMap;
    _section = const_cast<Section*>(section);
    // by uchida 2016/5/30
    // でも_intersectionはNULLでなくてはならないので
    // 最後に初期化する
    // この関数はODノードでしか呼ばれないしいいか
    _intersection = const_cast<Intersection*>(intersection);

    Intersection* start;
    start = _intersection->next(_intersection->direction(_section));
    Intersection* past = const_cast<Intersection*>(_intersection);

    Intersection* goal;
    vector<CSNode*> csNodes = _roadMap->csNodes();
    double min = 1000000;// 十分大きいということで
    int min_index = -1;
    int step;
    double GV;

    for (int i = 0; i < csNodes.size(); i++)
    {
        step = 10000;
        goal = dynamic_cast<Intersection*>(csNodes[i]);
        // GVを返す的なイメージなんだよなぁ
        GV = _router->searchSegmentGV(start, goal, past, step, goal->id());

        if (min >= GV)
        {
            min = GV;
            min_index = i;
        }
    }
    assert(min_index >= 0);

    step = 10000;
    goal = const_cast<Intersection*>(_router->goal());
    // GVを返す的なイメージなんだよなぁ
    GV = _router->searchSegmentGV(start, goal, past, step, "");

    // by uchida 2016/5/30
    // ここでNULLに戻す
    _intersection = NULL;

    if (min >= GV)
    {
        _stopCS = goal->id();
    }
    else
    {
        _stopCS = csNodes[min_index]->id();
    }

    return _stopCS;
}

//======================================================================
void Vehicle::_searchCSSumCost()
{
    Intersection* start;
    start = _intersection->next(_intersection->direction(_section));
    Intersection* past = const_cast<Intersection*>(_intersection);

    Intersection* goal;
    vector<CSNode*> csNodes = _roadMap->csNodes();
    double min = 1000000;// 十分大きいということで
    int min_index = 0;
    int step;
    double GV;

    for (int i = 0; i < csNodes.size(); i++)
    {
        step = 10000;
        goal = dynamic_cast<Intersection*>(csNodes[i]);

        GV = _router->searchSegmentGV(start, goal, past, step, goal->id())
           + _router->searchSegmentGV(goal, _router->goal(), NULL, step, goal->id());

        if (min >= GV)
        {
            min = GV;
            min_index = i;
        }
    }
    assert(min_index >= 0);

    // 暫定的に選択されたCSまでのコスト
    step = 10000;
    goal = dynamic_cast<Intersection*>(csNodes[min_index]);
    double min_cs;
    min_cs = _router->searchSegmentGV(start, goal, past, step, goal->id());

    // Dまで経由無しの場合のコスト
    step = 10000;
    goal = const_cast<Intersection*>(_router->goal());
    GV = _router->searchSegmentGV(start, goal, past, step, "");

    if (min_cs >= GV)
    {
        _stopCS = goal->id();
    }
    else
    {
        _stopCS = csNodes[min_index]->id();
    }
}

//======================================================================
std::string Vehicle::_searchCSSumCost(RoadMap* roadMap,
                                      const Section* section,
                                      const Intersection* intersection)
{
    _roadMap = roadMap;
    _section = const_cast<Section*>(section);
    // by uchida 2016/5/30
    // でも_intersectionはNULLでなくてはならないので
    // 最後に初期化する
    // この関数はODノードでしか呼ばれないしいいか
    _intersection = const_cast<Intersection*>(intersection);

    Intersection* start;
    start = _intersection->next(_intersection->direction(_section));
    Intersection* past = const_cast<Intersection*>(_intersection);

    Intersection* goal;
    vector<CSNode*> csNodes = _roadMap->csNodes();
    double min = 1000000;// 十分大きいということで
    int min_index = -1;
    int step;
    double GV;

    for (int i = 0; i < csNodes.size(); i++)
    {
        step = 10000;
        goal = dynamic_cast<Intersection*>(csNodes[i]);

        GV = _router->searchSegmentGV(start, goal, past, step, goal->id())
           + _router->searchSegmentGV(goal, _router->goal(), NULL, step, goal->id());

        if (min >= GV)
        {
            min = GV;
            min_index = i;
        }
    }
    assert(min_index >= 0);

    // 暫定的に選択されたCSまでのコスト
    step = 10000;
    goal = dynamic_cast<Intersection*>(csNodes[min_index]);
    double min_cs;
    min_cs = _router->searchSegmentGV(start, goal, past, step, goal->id());

    // Dまで経由無しの場合のコスト
    step = 10000;
    goal = const_cast<Intersection*>(_router->goal());
    GV = _router->searchSegmentGV(start, goal, past, step, "");

    // by uchida 2016/5/30
    // ここでNULLに戻す
    _intersection = NULL;

    if (min_cs >= GV)
    {
        _stopCS = goal->id();
    }
    else
    {
        _stopCS = csNodes[min_index]->id();
    }

    return _stopCS;
}

//======================================================================
const Intersection* Vehicle::destination() const
{
    return _router->goal();
}

//======================================================================
const Route* Vehicle::route() const
{
    return _route;
}

//======================================================================
ARouter* Vehicle::router()
{
    return _router;
}

//======================================================================
/*
 * この関数は以前は全ての再探索で使用されていたが，
 * 現在は車両発生時にしか呼ばれなくなった．
 * ("reroute"という関数名は正しくない)
 */
void Vehicle::reroute()
{
    assert(_section);

    reroute(_section, _router->start());
    _localRouter.setRoadMap(_roadMap);
    _localRouter.setRoute(_route);
    _localRouter.setLocalRoute(&_localRoute);
}

//======================================================================
bool Vehicle::reroute(const Section* section,
                      const Intersection* start)
{
    bool succeed = true;
    int _routerIStep = 10000;
    Route* new_route = NULL;

    // cout << "vehicle:" << _id << " reroute." << endl;
    // by uchida 2016/5/10
    _router->search(section, start, _routerIStep, new_route);

    if(new_route == NULL)
    {
        succeed = false;
    }
    else if(new_route->size() == 0)
    {
        succeed = false;
    }

    // 古い経路を削除
    if(_route != NULL)
    {
        delete _route;
    }

    // 新しい経路を設定
    if(succeed)
    {
        _route = new_route;
        _localRouter.setRoute(_route);
    }
    else
    {
        if(new_route != NULL)
        {
            delete new_route;
            new_route = NULL;
        }
        // ルート探索失敗した場合は仕方が無いのでスタート地点とゴール地点を
        // 直接結ぶ経路を設定する(ほとんどの場合、このような経路はない。)
        // 落ちるのを防ぐための苦肉の策
        _route = new Route();
        _route->push(const_cast<Intersection*>(_router->start()));
        _route->push(const_cast<Intersection*>(_router->goal()));
        _localRouter.setRoute(_route);
    }

    return succeed;
}

// by uchida 2016/5/12
//======================================================================
bool Vehicle::CSreroute(const Section* section,
                      const Intersection* start,
                      string stopCS)
{
    bool succeed = true;
    int _routerIStep = 10000;
    Route* new_route = NULL;

    _router->CSsearch(section, start, _routerIStep, new_route, stopCS);

    if(new_route == NULL)
    {
        succeed = false;
    }
    else if(new_route->size() == 0)
    {
        succeed = false;
    }

    // 古い経路を削除
    if(_route != NULL)
    {
        delete _route;
    }

    // 新しい経路を設定
    if(succeed)
    {
        _route = new_route;
        _localRouter.setRoute(_route);
    }
    else
    {
        if(new_route != NULL)
        {
            delete new_route;
            new_route = NULL;
        }
        // ルート探索失敗した場合は仕方が無いのでスタート地点とゴール地点を
        // 直接結ぶ経路を設定する(ほとんどの場合、このような経路はない。)
        // 落ちるのを防ぐための苦肉の策
        _route = new Route();
        _route->push(const_cast<Intersection*>(_router->start()));
        _route->push(const_cast<Intersection*>(_router->goal()));
        _localRouter.setRoute(_route);
    }

    return succeed;
}

//======================================================================
const vector<Lane*>* Vehicle::lanesInIntersection() const
{
    return _localRoute.lanesInIntersection();
}

//======================================================================
RandomGenerator* Vehicle::randomGenerator()
{
    assert(_rnd);
    return _rnd;
}

//======================================================================
void Vehicle::print() const
{
    cout << "--- Vehicle Information ---" << endl;
    cout << "ID: " << _id << ", Type: " << _type << endl;

    // 位置と速度に関するもの
    if (_section!=NULL)
    {
        cout << "Section ID, Lane ID: " << _section->id();
    }
    else
    {
        cout << "Intersection ID, Lane ID: " << _intersection->id();
    }
    cout << ", " << _lane->id() << endl;
    cout << "Length, Error: " << _length << ", " << _error << endl;
    cout << "Velocity, ErrorVelocity: "
         << _velocity <<", "<< _errorVelocity << endl;
    cout << "(x,y,z)=("
         << x() << ", " << y() << ", " << z() << ")"<< endl;

    // 経路に関するもの
    _router->printParam();
    _route->print(cout);
    _localRoute.print();

    // 交錯レーンに関するもの
    if (_section!=NULL)
    {
        Intersection* nextInter
            = _section->intersection(_section->isUp(_lane));
        if (nextInter)
        {
            vector<Lane*> cli;
            vector<Lane*> cls;
            nextInter->collisionLanes(_localRoute.lanesInIntersection(),
                                      &cli, &cls);
            cout << "Collision Lanes in Intersection:" << endl;
            for (unsigned int i=0; i<cli.size(); i++)
            {
                cout << "\t" << cli[i]->id() << endl;
            }
            cout << "Collision Lanes in Section:" << endl;
            for (unsigned int i=0; i<cls.size(); i++)
            {
                cout << "\t" << cls[i]->id() << " of section "
                     << nextInter->nextSection
                    (nextInter->direction
                     (cls[i]->endConnector()))->id()
                     << endl;
            }
        }
    }

    // 車線変更に関するもの
    if (_laneShifter.isActive())
    {
        cout << "Shift Lane:" << endl;
        cout << "  Target Lane  : " << _laneShifter.laneTo()->id() << endl;
        cout << "  Target Length: " << _laneShifter.lengthTo() << endl;
    }

#ifdef VL_DEBUG
    cout << "Virtual Leaders:" << endl;
    for (unsigned int i=0; i<_leaders.size(); i++)
    {
        _leaders[i]->print();
    }
#endif //VL_DEBUG

    cout << endl;
}

//by uchida 2017/2/2
//====================================================================
bool Vehicle::threshold()
{
    return false;
}

// Chargeflagは充電走行用の経路探索を実行するかどうか
//====================================================================
void Vehicle::offChargeflag()
{
    if (GVManager::getFlag("FLAG_OUTPUT_SCORE"))
    {
        scoringSubNodes();
    }
    _isCharge = false;
}

//====================================================================
void Vehicle::onChargeflag()
{
    if (GVManager::getFlag("FLAG_OUTPUT_SCORE"))
    {
        nearestSubNode();
    }
    _isCharge = true;

    // 2017/8/23 by uchida
    // 充電走行開始時にゴースト生成のためroadmapにフラグ登録
    // ここだけコメントアウトすればゴースト生成をなしにできる予定
    _roadMap->setGhostFlag(_id);
}

// onChargingはCS内で充電しているかどうか
//====================================================================
bool Vehicle::onCharging()
{
    return _onCharging;
}

//====================================================================
void Vehicle::setonCharging(bool flag)
{
    if (_onCharging != flag)
    {
        _onCharging = flag;
        _swapTime = 1;
        // debug by uchida 2017/6/23
//        cout << "~ " << TimeManager::time() / 1000 << " [s] "
//             << _id << " : start charging at " << _intersection->id() << endl;
    }
}

//====================================================================
void Vehicle::setWaiting(bool flag)
{
    _waiting = flag;
}

//by uchida 2017/2/10
//====================================================================
void Vehicle::nearestSubNode()
{
    vector<SubNode*> subnodes;

    // intersectionにいる場合SubNodeは一意に定まる
    if (_intersection != NULL)
    {
        subnodes = _intersection->SubNodes();
        _startSN = subnodes[0];
        _startSec = NULL;
    }
    // sectionにいる場合は検索する必要がある
    // SubNodeがsection内部にある場合
    else if (_section->SubNodes().size() != 0)
    {
        subnodes = _section->SubNodes();
        double min = 1000000;
        int min_index = 0;

        for (int i = 0; i < subnodes.size(); i++)
        {
            double dis = subnodes[i]->point().distance(x(), y(), z());
            if (min > dis)
            {
                min = dis;
                min_index = i;
            }
        }
        _startSN = subnodes[min_index];
        _startSec = _section;
    }
    // SubNodeがsection内部にない場合
    else if (_section->SubNodes().size() == 0)
    {
        Intersection* first  = _section->intersection(true);
        Intersection* second = _section->intersection(false);
        if (first->center().distance(x(), y(), z()) < second->center().distance(x(), y(), z()) )
        {
            subnodes = first->SubNodes();
        }
        else
        {
            subnodes = second->SubNodes();
        }
        _startSN = subnodes[0];
        _startSec = NULL;
    }
}

//====================================================================
SubNode* Vehicle::_nearestSubNode()
{
    vector<SubNode*> subnodes;

    // intersectionにいる場合SubNodeは一意に定まる
    if (_intersection != NULL)
    {
        subnodes = _intersection->SubNodes();
        return subnodes[0];
    }
    // sectionにいる場合は検索する必要がある
    // SubNodeがsection内部にある場合
    else if (_section->SubNodes().size() != 0)
    {
        subnodes = _section->SubNodes();
        double min = 1000000;
        int min_index = 0;

        for (int i = 0; i < subnodes.size(); i++)
        {
            double dis = subnodes[i]->point().distance(x(), y(), z());
            if (min > dis)
            {
                min = dis;
                min_index = i;
            }
        }
        return subnodes[min_index];
    }
    // SubNodeがsection内部にない場合
    else if (_section->SubNodes().size() == 0)
    {
        Intersection* first  = _section->intersection(true);
        Intersection* second = _section->intersection(false);
        if (first->center().distance(x(), y(), z()) < second->center().distance(x(), y(), z()) )
        {
            subnodes = first->SubNodes();
        }
        else
        {
            subnodes = second->SubNodes();
        }
        return subnodes[0];
    }
}

//====================================================================
void Vehicle::getSubNodes_re()
{
    // 同一Section内で完結する場合
    if (_startSec == _section && _startSec != NULL)
    {
        vector<SubNode*> vec = _startSec->SubNodes();

        int j = 0;
        while (vec[j] != _startSN)
        {
            j++;
        }

        double min = 1000000;
        int min_index = 0;

        for (int i = 0; i < vec.size(); i++)
        {
            double dis = vec[i]->point().distance(x(), y(), z());
            if (min > dis)
            {
                min = dis;
                min_index = i;
            }
        }

        if (min_index > j)
        {
            for (int i = j; i <= min_index; i++)
            {
                _allsubnodes.push_back(vec[i]);
            }
        }
        else
        {
            for (int i = min_index; i <= j; i++)
            {
                _allsubnodes.push_back(vec[i]);
            }
        }

    }
    // sectionで充電走行開始だった場合
    else if (_startSec != NULL)
    {
        vector<Intersection*> route = *(_route->route());
        vector<SubNode*> vec = _startSec->SubNodes();
        vector<SubNode*> vec_f;

        int j = 0;
        while (vec[j] != _startSN)
        {
            j++;
        }

        // _adjInter[1]と一致する場合、正方向へSubNodeをたどる
        if (_startSec->intersection(true) == route[0])
        {
            while (j < vec.size())
            {
                vec_f.push_back(vec[j]);
                j++;
            }
        }
        //　_adjInter[0]と一致する場合、反対方向へSubNodeをたどる
        else if (_startSec->intersection(false) == route[0])
        {
            while (j >= 0)
            {
                vec_f.push_back(vec[j]);
                j--;
            }
        }

        // (1)始端sectionのSubNode追加
        _allsubnodes.insert(_allsubnodes.end(), vec_f.begin(), vec_f.end());

        // @CSで実行されている場合(CSreroute)
        if (_intersection != NULL)
        {

            // (2)Intersectionの組み合わせでSectionを取得する
            for(int i = 0; i < route.size()-1; i++)
            {
                // intersectionのSubNode追加
                vector<SubNode*> vec_i = route[i]->SubNodes();
                _allsubnodes.insert(_allsubnodes.end(), vec_i.begin(), vec_i.end());
                // sectionのSubNode追加
                vector<SubNode*> vec_s = route[i]->nextSection(route[i+1])->SubNodes();

                // _adjInter[1]と一致する場合、正方向へSubNodeをたどる
                if (route[i]->nextSection(route[i+1])->intersection(true) == route[i+1])
                {
                    _allsubnodes.insert(_allsubnodes.end(), vec_s.begin(), vec_s.end());
                }
                //　_adjInter[0]と一致する場合、反対方向へSubNodeをたどる
                else if (route[i]->nextSection(route[i+1])->intersection(false) == route[i+1])
                {
                    reverse(vec_s.begin(), vec_s.end());
                    _allsubnodes.insert(_allsubnodes.end(), vec_s.begin(), vec_s.end());
                }
            }
            // (3)終端intersectionのSubNode追加
            vector<SubNode*> vec_l = route[route.size()-1]->SubNodes();
            _allsubnodes.insert(_allsubnodes.end(), vec_l.begin(), vec_l.end());

        }

    }
    // intersectionで充電開始だった場合
    else if (_startSec == NULL)
    {
        // by uchida 2017/2/15
        //intersectionとroute[0]が一致しているか等によって判定すべし
    }

}

//====================================================================
void Vehicle::getSubNodes_del()
{

    // 充電切れSubNodeの取得
    SubNode* stranded = _nearestSubNode();

    // 同一Section内で完結する場合
    if (_startSec == _section && _startSec != NULL)
    {
        vector<SubNode*> vec = _startSec->SubNodes();
        int j = 0;
        while (vec[j] != _startSN)
        {
            j++;
        }

        int k = 0;
        while (vec[k] != stranded)
        {
            k++;
        }

        if (j < k)
        {
            for (int i = j; i <= k; i++)
            {
                _allsubnodes.push_back(vec[i]);
            }
        }
        else
        {
            for (int i = j; i >= k; i--)
            {
                _allsubnodes.push_back(vec[i]);
            }
        }
    }
    // sectionで充電開始だった場合
    else if (_startSec != NULL)
    {
        vector<Intersection*> route = *(_route->route());
        vector<SubNode*> vec = _startSec->SubNodes();
        vector<SubNode*> vec_f;
        int j = 0;
        while (vec[j] != _startSN)
        {
            j++;
        }

        int k = 0;
        while (route[k] != _startSec->intersection(true) && route[k] != _startSec->intersection(false))
        {
            k++;
            if (k > route.size())
            {
                k = -1;
                break;
            }
        }

        if (k == -1)
        {
            k = 0;
            route.clear();
            route = _realPassedRoute;
            while (route[k] != _startSec->intersection(true) && route[k] != _startSec->intersection(false))
            {
                k++;
            }
        }

        // _adjInter[1]と一致する場合、正方向へSubNodeをたどる
        if (_startSec->intersection(false) == route[k])
        {
            while (j < vec.size())
            {
                vec_f.push_back(vec[j]);
                j++;
            }
        }
        //　_adjInter[0]と一致する場合、反対方向へSubNodeをたどる
        else if (_startSec->intersection(true) == route[k])
        {
            while (j >= 0)
            {
                vec_f.push_back(vec[j]);
                j--;
            }
        }

        // (1)始端sectionのSubNode追加
        _allsubnodes.insert(_allsubnodes.end(), vec_f.begin(), vec_f.end());

        //yt @section上で実行されている場合(addStrandedEVs)
        if (_intersection == NULL)
        {

            // (2)Intersectionの組み合わせでSectionを取得する
            for(int i = k+1; i < route.size()-1; i++)
            {
                // intersectionのSubNode追加
                vector<SubNode*> vec_i = route[i]->SubNodes();
                _allsubnodes.insert(_allsubnodes.end(), vec_i.begin(), vec_i.end());

                // (3)終端SectionのSubNode追加
                if (_section == route[i]->nextSection(route[i+1]))
                {
                    // 終端SubNodeがSection上に存在する
                    if (_section->SubNodes().size() != 0)
                    {
                        vector<SubNode*> vec_l = _section->SubNodes();

                        // _adjInter[1]と一致する場合、正方向へSubNodeをたどる
                        if (_section->intersection(true) == route[i+1])
                        {
                            int j = 0;
                            while (vec_l[j] != stranded)
                            {
                                _allsubnodes.push_back(vec_l[j]);
                                j++;
                            }
                        }
                        //　_adjInter[0]と一致する場合、反対方向へSubNodeをたどる
                        else if (_section->intersection(false) == route[i+1])
                        {
                            int j = vec_l.size()-1;
                            while (vec_l[j] != stranded)
                            {
                                _allsubnodes.push_back(vec_l[j]);
                                j--;
                            }
                        }
                    }
                    break;
                }

                // sectionのSubNode追加
                vector<SubNode*> vec_s = route[i]->nextSection(route[i+1])->SubNodes();
                // _adjInter[1]と一致する場合、正方向へSubNodeをたどる
                if (route[i]->nextSection(route[i+1])->intersection(true) == route[i+1])
                {
                    _allsubnodes.insert(_allsubnodes.end(), vec_s.begin(), vec_s.end());
                }
                //　_adjInter[0]と一致する場合、反対方向へSubNodeをたどる
                else if (route[i]->nextSection(route[i+1])->intersection(false) == route[i+1])
                {
                    reverse(vec_s.begin(), vec_s.end());
                    _allsubnodes.insert(_allsubnodes.end(), vec_s.begin(), vec_s.end());
                }

            }

        }
        // @intersection上で実行されている場合(deleteAgent)
        else if (_intersection != NULL)
        {
            // (2)Intersectionの組み合わせでSectionを取得する
            for(int i = k+1; i < route.size()-1; i++)
            {
                // intersectionのSubNode追加
                vector<SubNode*> vec_i = route[i]->SubNodes();
                _allsubnodes.insert(_allsubnodes.end(), vec_i.begin(), vec_i.end());

                if (_intersection == route[i])
                {
                    break;
                }

                // sectionのSubNode追加
                vector<SubNode*> vec_s = route[i]->nextSection(route[i+1])->SubNodes();
                // _adjInter[1]と一致する場合、正方向へSubNodeをたどる
                if (route[i]->nextSection(route[i+1])->intersection(true) == route[i+1])
                {
                    _allsubnodes.insert(_allsubnodes.end(), vec_s.begin(), vec_s.end());
                }
                //　_adjInter[0]と一致する場合、反対方向へSubNodeをたどる
                else if (route[i]->nextSection(route[i+1])->intersection(false) == route[i+1])
                {
                    reverse(vec_s.begin(), vec_s.end());
                    _allsubnodes.insert(_allsubnodes.end(), vec_s.begin(), vec_s.end());
                }

            }

        }

    }
    // intersectionで充電開始だった場合
    else if (_startSec == NULL)
    {
        // by uchida 2017/2/15
        //intersectionとroute[0]が一致しているか等によって判定すべし
    }
}

//====================================================================
void Vehicle::scoringSubNodes()
{
    for (int i = 0; i < _allsubnodes.size(); i++)
    {
        double x = i/_allsubnodes.size();

        switch ((int)(GVManager::getNumeric("SCOREING_METHOD")))
        {
            case 1:
                // (1)一定
                _allsubnodes[i]->score(1.0);
                break;
            case 2:
                // (2)単調増加
                _allsubnodes[i]->score(x);
                break;
            case 3:
                // (3)単調減少
                _allsubnodes[i]->score(1.0 - x);
                break;
            case 4:
                // (4)上に凸・減少
                _allsubnodes[i]->score(cos((x*M_PI)/2.0));
                break;
            case 5:
                // (5)上に凸・増加
                _allsubnodes[i]->score(cos(((x+1.0)*M_PI)/2.0));
                break;
            case 6:
                // (6)下に凸・減少
                _allsubnodes[i]->score(cos(((x+1.0)*M_PI)/2.0)+1.0);
                break;
            case 7:
                // (7)下に凸・増加
                _allsubnodes[i]->score(cos(((x+2.0)*M_PI)/2.0)+1.0);
                break;
            case 8:
                // (8)シグモイド・減少
                _allsubnodes[i]->score(exp(-10.0*(x-0.5))/(1.0+exp(-10.0*(x-0.5))));
                break;
            case 9:
                // (9)シグモイド・増加
                _allsubnodes[i]->score(1.0/(1.0+exp(-10.0*(x-0.5))));
                break;
        }
    }

    // 初期化しておく
    _allsubnodes.erase(_allsubnodes.begin(), _allsubnodes.end());
    _startSN  = NULL;
    _startSec = NULL;
}
