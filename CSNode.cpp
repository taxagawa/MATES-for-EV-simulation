/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "AmuConverter.h"
#include "CSNode.h"
#include "LaneBundle.h"
#include "Intersection.h"
#include "ODNode.h"
#include "CSNode.h"
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
#include <cmath>

using namespace std;

//======================================================================
CSNode::CSNode(const string& id,
               const string& type,
               RoadMap* parent)
    : ODNode(id, type, parent)
{
    // by uchida 2016/5/23
    _isCS = true;
    _integratedCharge = 0.0;
    // by takusagawa 2018/9/25
    _estimatedWaitingTime = 0.0;
    // by takusagawa 2018/11/1
    waitingTimeHistoryMaxSize = (CS_WAITING_TIME_HISTORY_LIMIT / (CS_WAITING_TIME_UPDATE_INTERVAL / 1000)) + 1;

    for (int i = -1 * (waitingTimeHistoryMaxSize-1); i <= 0; i+=2) _xdata.push_back(double(i));
    assert(_xdata.size() == ((waitingTimeHistoryMaxSize+1) / 2));

    // by takusagawa 2019/1/4
    _IV = 0.0;

    // by takusagawa 2018/11/6
    // _servedEV = 0;

//    _lastGenTime = 0;
//    _nodeGvd.clear();
//    _isWaitingToPushVehicle = false;
//    _isOutputGenerateVehicleData = false;
//    _rnd = Random::randomGenerator();
}

//======================================================================
CSNode::~CSNode()
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

//======================================================================
CSNode* CSNode::getRandCS()
{


}

// by uchida 2016/5/23
////======================================================================
void CSNode::sumCharge(double chargingValue)
{
    _instantaneousCharge += chargingValue;
}

////======================================================================
void CSNode::initCharge()
{
    _instantaneousCharge = 0.0;
}

// by takusagawa 2018/3/26
////======================================================================
double CSNode::instantaneousCharge()
{
    return _instantaneousCharge;
}

// by uchida 2017/11/24
////======================================================================
void CSNode::integrated(double instantaneousCharge)
{
    _integratedCharge += instantaneousCharge;
}

////======================================================================
double CSNode::integratedCharge()
{
    return _integratedCharge;
}

////======================================================================
void CSNode::initIntegrated()
{
    _integratedCharge = 0.0;
}

////======================================================================
void CSNode::addEV(Vehicle* vehicle)
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
            // debug by takusagawa 2018/11/10
            // cout << "CSid: " << id() << " ID: " << (*itr)->id() << endl;
            (*itr)->setonCharging(true);
            itr++;
        }
    }
}

////======================================================================
void CSNode::removeEV()
{
    vector<Vehicle*>::iterator itr = waitingLine.begin();
    (*itr)->setWaiting(false);
    // debug by takusagawa 2018/11/10
    // cout << "erase vehicle ID: " << (*itr)->id() << " SOC: " << (*itr)->SOC() << endl;

    // debug by uchida 2017/6/23
    // cout << "~ " << TimeManager::time() / 1000 << " [s] "
    //     << (*itr)->id() << " : restart from " << _id << endl;

    // by takusagawa 2018/11/10
    // waitingLine内において,removeEV()を呼び出した車両より前にまだ充電が終わっていない
    // 車両が存在する場合の処理を追加
    for (int i = 0; i < _capacity; i++)
    {
        if ((*itr)-> SOC() < 0.8)
        {
            itr++;
            continue;
        }
        else if (itr == waitingLine.end())
        {
            break;
        }
        else
        {
            waitingLine.erase(itr);
            break;
        }
    }

    // _servedEV++;

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
                // debug by takusagawa 2018/11/10
                // cout << "CSid: " << id() << " *ID: " << (*itr)->id() << endl;
                (*itr)->setonCharging(true);
                itr++;
            }
        }
    }
}

////======================================================================
void CSNode::setCapacity(int capacity)
{
    _capacity = capacity;
}

////======================================================================
void CSNode::setOutPower(double outPower)
{
    _outPower = outPower;
}

////======================================================================
double CSNode::outPower()
{
    return _outPower;
}

// by takusagawa 2018/9/25
////======================================================================
double CSNode::estimatedWaitingTime() const
{
    return _estimatedWaitingTime;
}

// by takusagawa 2018/9/25
////======================================================================
void CSNode::estimatedWaitingTimeCalc()
{
    cout << "waitingLine size: " << waitingLine.size() << endl;
    // 初期化
    _estimatedWaitingTime = 0.0;
    int size = waitingLine.size();

    // commented by takusagawa 2018/11/1
    // 冗長なのでif-elseで書くべきだが,面倒くさいのこのままで.
    // CSの箇所が多くない限り,そこまで呼び出される関数ではないため.

    // 待機列が空かどうかチェック
    // 空なら待ち時間は0.0秒
    // あるいはCSが1台でも空いていれば待ち時間はとりあえず0.0秒
    if (waitingLine.empty() || size < _capacity)
    {
        // by takusagawa 2018/11/1
        // 待ち時間履歴に追加
        if (GVManager::getFlag("FLAG_USE_FUTURE_WAITING_LINE"))
        {
            addWaitingTimeHistory(_estimatedWaitingTime);
            // by takusagawa 2018/12/7
            // 複数のモードを追加
            if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 0)
            {
                createFutureWaitingTimeList();
            }
            else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 1)
            {
                predictByApproximationFunc(1);
            }
            else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 2)
            {
                predictByApproximationFunc(2);
            }
            else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 3)
            {
                predictByApproximationFunc(3);
            }

            if (GVManager::getFlag("FLAG_USE_INTEGRAL_CONTROLLER"))
            {
                addUnlimitedWaitingTimeHistory(_estimatedWaitingTime);
                calcIV();
            }
        }
        return;
    }

    // cout << "size:" << size << endl;
    // by takusagawa 2018/11/1
    // 仕様変更:終了判定にsizeをそのまま用いると,余分に推定待ち時間が加算されてしまう.
    int realSize = size - _capacity + 1;

    assert(realSize > 0);

    for (int i = 0; i < realSize; i++)
    {
        //cout << "i: " << i << endl;
        double batteryCapacity = waitingLine[i]->getBatteryCapacity();
        double requiredChagingPower = waitingLine[i]->requiredChagingPowerCalc(batteryCapacity);
        _estimatedWaitingTime += (1000.0 * requiredChagingPower) / (10 * _outPower * 1000.0 * (TimeManager::unit() / 1000.0));
    }

    // by takusagawa 2018/11/1
    // 待ち時間履歴に追加
    if (GVManager::getFlag("FLAG_USE_FUTURE_WAITING_LINE"))
    {
        addWaitingTimeHistory(_estimatedWaitingTime);
        // by takusagawa 2018/12/7
        // 複数のモードを追加
        if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 0)
        {
            createFutureWaitingTimeList();
        }
        else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 1)
        {
            predictByApproximationFunc(1);
        }
        else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 2)
        {
            predictByApproximationFunc(2);
        }
        else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 3)
        {
            predictByApproximationFunc(3);
        }

        if (GVManager::getFlag("FLAG_USE_INTEGRAL_CONTROLLER"))
        {
            addUnlimitedWaitingTimeHistory(_estimatedWaitingTime);
            calcIV();
        }
    }

    return;
}

// by takusagawa 2018/10/8
////======================================================================
int CSNode::waitingLineSize() const
{
    return waitingLine.size();
}

// by takusagawa 2018/11/1
////======================================================================
void CSNode::addWaitingTimeHistory(double estimatedTime)
{
    if (waitingTimeHistory.size() < waitingTimeHistoryMaxSize)
    {
        waitingTimeHistory.push_back(estimatedTime);
    }
    else
    {
        // 先頭（最も古い履歴）を削除
        vector<double>::iterator itr = waitingTimeHistory.begin();
        waitingTimeHistory.erase(itr);

        waitingTimeHistory.push_back(estimatedTime);
    }

    // debug by takusagawa 2018/11/1
    // int size = waitingTimeHistory.size();
    // for (int i = 0; i < size; i++)
    // {
    //     cout << waitingTimeHistory[i] << ", ";
    // }
    // cout << endl;

    return;
}

// by takusagawa 2019/1/4
////======================================================================
void CSNode::addUnlimitedWaitingTimeHistory(double estimatedTime)
{
    unlimitedWaitingTimeHistory.push_back(estimatedTime);

    // debug by takusagawa 2019/1/4
    // int size = unlimitedWaitingTimeHistory.size();
    // for (int i = 0; i < size; i++)
    // {
    //     cout << unlimitedWaitingTimeHistory[i] << ", ";
    // }
    // cout << endl;

    return;
}

// by takusagawa 2019/1/4
////======================================================================
void CSNode::calcIV()
{
    int size = unlimitedWaitingTimeHistory.size();
    double tmp = 0.0

    if (size == 1) return;

    double current = unlimitedWaitingTimeHistory[size-1];

    for (int i = 0; i < size-1; i++)
    {
        tmp += current - unlimitedWaitingTimeHistory[i];
    }

    // debug
    // cout << "_IV: " << tmp << endl;

    _IV = tmp;
    return;
}

// by takusagawa 2019/1/4
////======================================================================
void CSNode::IV() const
{
    return _IV;
}

// by takusagawa 2018/11/2
////======================================================================
void CSNode::createFutureWaitingTimeList()
{
    futureWaitingTimeList.clear();

    int futureListSize = waitingTimeHistoryMaxSize-1;
    futureWaitingTimeList.reserve(futureListSize);

    int historySize = waitingTimeHistory.size();
    assert(historySize > 0);

    // 最新の待ち時間
    double latestWaitingTime = waitingTimeHistory[historySize-1];

    // まず予測するためのデータが足りない場合の処理
    if (historySize < waitingTimeHistoryMaxSize)
    {
        // 一番最初に待ち台数履歴が登録されたときの処理
        if (historySize == 1)
        {
            for (int i = 0; i < futureListSize; i++)
            {
                futureWaitingTimeList.push_back(latestWaitingTime);
            }
            // fillで埋めようとしたらメモリリークがおきたので不採用.なぜ
            // fill(futureWaitingTimeList.begin(), futureWaitingTimeList.end(), latestWaitingTime);
        }
        else
        {
            // 最も古い履歴と最新の差
            double largeTimeDiff = latestWaitingTime - waitingTimeHistory[0];
            for (int i = 0; i < futureListSize; i++)
            {
                futureWaitingTimeList.push_back(largeTimeDiff);
            }
            // fill(futureWaitingTimeList.begin(), futureWaitingTimeList.end(), largeTimeDiff);

            for (int i = 0; i < historySize-1; i++)
            {
                double timeDiff = latestWaitingTime - waitingTimeHistory[historySize-(i+2)];
                futureWaitingTimeList[i] = latestWaitingTime + timeDiff;

                // 場合によっては負になってしまうので,その対応
                if (futureWaitingTimeList[i] < 0)
                {
                    futureWaitingTimeList[i] = 0;
                }
                // cout << timeDiff << ":" << futureWaitingTimeList[i] << endl;
            }
        }
    }
    else
    {
        for (int i = 0; i < historySize-1; i++)
        {
            double timeDiff = latestWaitingTime - waitingTimeHistory[historySize-(i+2)];
            futureWaitingTimeList[i] = latestWaitingTime + timeDiff;

            // 場合によっては負になってしまうので,その対応
            if (futureWaitingTimeList[i] < 0)
            {
                futureWaitingTimeList[i] = 0;
            }
            // cout << timeDiff << ":" << futureWaitingTimeList[i] << endl;
        }
    }

    // debug by takusagawa 2018/11/2
    // for (int i = 0; i < futureListSize; i++)
    // {
    //     cout << futureWaitingTimeList[i] << ", ";
    // }
    // cout << endl;

    return;
}

// by takusagawa 2018/11/4
// 30秒間隔で待ち時間情報を更新するので,現状では最大29秒のズレが生じる.
// より正確を期するのであれば,CS探索リクエストが出た瞬間の時刻からの到着予想時刻と
// 最新の待ち時間情報登録時刻の差を考慮するべき.
////======================================================================
double CSNode::estimatedFutureWaitingTime(double cost)
{
    int index = -1;
    int icost = round(cost);

    index = icost / CS_WAITING_TIME_UPDATE_INTERVAL;

    if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 0)
    {
        if (index > waitingTimeHistoryMaxSize-2)
        {
            index = waitingTimeHistoryMaxSize-2;
        }

    // debug
    // cout << "Use " << ((index + 1) * CS_WAITING_TIME_UPDATE_INTERVAL) / 1000 << "seconds ago" << endl;

        assert(index >= 0);

        return futureWaitingTimeList[index];
    }
    else
    {
        if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 1)
        {
            double val = _coefficient[1] * index + _coefficient[0];
            return val;
        }
        else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 2)
        {
            double val = _coefficient[2] * index * index + _coefficient[1] * index + _coefficient[0];
            return val;
        }
        else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 3)
        {
            double val = _coefficient[3] * index * index * index + _coefficient[2] * index * index + _coefficient[1] * index + _coefficient[0];
            return val;
        }
    }
}

// by takusagawa 2018/11/12
////======================================================================
double CSNode::returnPredictionWaitingTime() const
{
    return futureWaitingTimeList[waitingTimeHistoryMaxSize-2];
}

// by takusagawa 2018/12/12
////======================================================================
double CSNode::returnApproximationWaitingTime(int min)
{
    int tmp = min * 2;

    if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 1)
    {
        double val = _coefficient[1] * tmp + _coefficient[0];
        return val;
    }
    else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 2)
    {
        double val = _coefficient[2] * tmp * tmp + _coefficient[1] * tmp + _coefficient[0];
        return val;
    }
    else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 3)
    {
        double val = _coefficient[3] * tmp * tmp * tmp + _coefficient[2] * tmp * tmp + _coefficient[1] * tmp + _coefficient[0];
        return val;
    }
}

// by takusagawa 2018/12/10
////======================================================================
void CSNode::predictByApproximationFunc(int degree)
{
    _coefficient.clear();

    // データ数
    int num = (waitingTimeHistoryMaxSize + 1) / 2;
    int size = waitingTimeHistory.size();
    // y
    vector<double> _ydata;

    if (size < waitingTimeHistoryMaxSize)
    {
        // まず足りない部分を0で埋める
        if (size % 2 == 0)
        {
            int tmp = num - (size / 2);
            for (int i = 0; i < tmp; i++) _ydata.push_back(0.0);
        }
        else
        {
            int tmp = num - ((size + 1) / 2);
            for (int i = 0; i < tmp; i++) _ydata.push_back(0.0);
        }
        // 新しい順かつ1分おきのデータを入れる
        for (int i = size-1; i >= 0 ; i-=2)
        {
            _ydata.push_back(waitingTimeHistory[i]);
        }
    }
    else
    {
        for (int i = 0; i < waitingTimeHistoryMaxSize; i+=2)
        {
            _ydata.push_back(waitingTimeHistory[i]);
        }
    }
    assert(_ydata.size() == num);

    // 本当は_xdataを渡す必要はない
    lstsq(_xdata, _ydata, num, degree, _coefficient);
}

// by takusagawa 2018/12/5
////======================================================================
void CSNode::lstsq(vector<double>& x, vector<double>& y, int n, int m, vector<double>& c)
{
	int i, j, k, m2, mp1, mp2;
	double *a, aik, pivot, *w, w1, w2, w3;

	if (m >= n || m < 1)
	{
		cerr << "Error : Illegal parameter in lstsq()" << endl;
        assert(0);
	}
	mp1 = m + 1;
	mp2 = m + 2;
	m2 = 2 * m;
	a = (double *)malloc(mp1 * mp2 * sizeof(double));
	if (a == NULL)
	{
		cerr << "Error : Out of memory  in lstsq()" << endl;
		assert(0);
	}
	w = (double *)malloc(mp1 * 2 * sizeof(double));
	if (w == NULL)
	{
        free(a);
		cerr << "Error : Out of memory  in lstsq()" << endl;
		assert(0);
	}

	for (i = 0; i < m2; i++)
	{
		w1 = 0.;
		for (j = 0; j < n; j++)
		{
			w2 = w3 = x[j];
			for (k = 0; k < i; k++)	w2 *= w3;
			w1 += w2;
		}
		w[i] = w1;
	}
	a[0] = n;
	for (i = 0; i < mp1; i++)
		for (j = 0; j < mp1; j++)	if (i || j)	a[i * mp2 + j] = w[i + j - 1];

	w1 = 0.;
	for (i = 0; i < n; i++)	w1 += y[i];
	a[mp1] = w1;
	for (i = 0; i < m; i++)
	{
		w1 = 0.;
		for (j = 0; j < n; j++)
		{
			w2 = w3 = x[j];
			for (k = 0; k < i; k++)	w2 *= w3;
			w1 += y[j] * w2;
		}
		a[mp2 * (i + 1) + mp1] = w1;
	}
	for (k = 0; k < mp1; k++)
	{
		pivot = a[mp2 * k + k];
		a[mp2 * k + k] = 1.0;
		for (j = k + 1; j < mp2; j++)	a[mp2 * k + j] /= pivot;
		for (i = 0; i < mp1; i++)
		{
			if (i != k)
			{
				aik = a[mp2 * i + k];
				for (j = k; j < mp2; j++)
					a[mp2 * i + j] -= aik * a[mp2 * k + j];
			}
		}
	}
	for (i = 0; i < mp1; i++)
    {
        c.push_back(a[mp2 * i + mp1]);
    }
	free(w);
	free(a);
	return;
}

// by takusagawa 2018/12/14
////======================================================================
double CSNode::getPredictiveGradient(double cost)
{
    int x = -1;
    int icost = round(cost);

    x = icost / CS_WAITING_TIME_UPDATE_INTERVAL;
    assert(x >= 0);

    if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 1)
    {
        return _coefficient[1];
    }
    else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 2)
    {
        double val = 2 *_coefficient[2] * x + _coefficient[1];
        return val;
    }
    else if (GVManager::getNumeric("WAITING_TIME_APPROXIMATION_DEGREE") == 3)
    {
        double val = 3 * _coefficient[3] * x * x + 2 *_coefficient[2] * x + _coefficient[1];
        return val;
    }
}

// by takusagawa 2018/11/6
////======================================================================
// int CSNode::servedEV() const
// {
//     return _servedEV;
// }

////======================================================================
//bool ODNode::hasWaitingVehicles() const
//{
//    if (!_waitingVehicles.empty())
//    {
//        return true;
//    }
//    else
//    {
//        return false;
//    }
//}

////======================================================================
//void ODNode::addWaitingVehicle(Vehicle* vehicle)
//{
//    assert(vehicle);

//#ifdef _OPENMP
//#pragma omp critical (addWaitingVehicle)
//    {
//#endif //_OPENMP

//    _waitingVehicles.push_back(vehicle);

//#ifdef _OPENMP
//    }
//#endif //_OPENMP
//}

////======================================================================
//void ODNode::addWaitingVehicleFront(Vehicle* vehicle)
//{
//    assert(vehicle);

//#ifdef _OPENMP
//#pragma omp critical (addWaitingVehicle)
//    {
//#endif //_OPENMP

//        _waitingVehicles.push_front(vehicle);

//#ifdef _OPENMP
//    }
//#endif //_OPENMP
//}

////======================================================================
//void ODNode::pushVehicleToReal(RoadMap* roadMap)
//{
//    assert(!(_waitingVehicles.empty()));
//
//    Section* section = _incSections[0];
//    vector<Lane*> lanes = section->lanesFrom(this);

//    // 他車両の車線変更先となっている車線を取得する
//    // 発生点付近で車線変更が行われている場合は車両は発生できない
//    vector<Lane*> shiftTargetLanes;
//    list<Vehicle*>* notifyVehicles = section->watchedVehicles();

//    list<Vehicle*>::iterator itv;
//    for (itv = notifyVehicles->begin();
//         itv != notifyVehicles->end();
//         itv++)
//    {
//        VehicleLaneShifter anotherLaneShifter = (*itv)->laneShifter();
//        if (anotherLaneShifter.isActive() &&
//            anotherLaneShifter.lengthTo() < 25)
//        {
//            shiftTargetLanes.push_back(const_cast<Lane*>(anotherLaneShifter.laneTo()));
//        }
//    }
//
//    // 十分な空きのあるレーンを取得
//    deque<Lane*> possibleLanes;
//    for (int i=0;
//         i<static_cast<signed int>(lanes.size());
//         i++)
//    {

//        vector<Lane*>::iterator itl = find(shiftTargetLanes.begin(),
//                                           shiftTargetLanes.end(),
//                                           lanes[i]);
//        if (itl!=shiftTargetLanes.end()) continue;
//
//        if (lanes[i]->tailAgent()==NULL)
//        {
//            possibleLanes.push_back(lanes[i]);
//        }
//#ifdef GENERATE_VELOCITY_0
//        else if (lanes[i]->tailAgent()->length()
//                 - lanes[i]->tailAgent()->bodyLength()/2
//                 - _waitingVehicles.front()->bodyLength()
//                 > 1.0)
//        {
//            possibleLanes.push_back(lanes[i]);
//        }
//#else // GENERATE_VELOCITY_0 not defined
//        else if (lanes[i]->tailAgent()->length()
//                 - lanes[i]->tailAgent()->bodyLength()/2
//                 - _waitingVehicles.front()->bodyLength()
//                 > 1.38+lanes[i]->speedLimit()/60.0/60.0*740)
//        {
//            possibleLanes.push_back(lanes[i]);
//        }
//#endif // GENERATE_VELOCITY_0
//    }

//    vector<Vehicle*> skipVehicles;
//    if (!possibleLanes.empty())
//    {
//        // 空きレーンがある場合に限り車両を発生させる
//        vector<int> order = Random::randomOrder(_rnd, possibleLanes.size());

//        while (!_waitingVehicles.empty() && !order.empty())
//        {
//            Vehicle* tmpVehicle = _waitingVehicles.front();
//            _waitingVehicles.pop_front();

//            Lane* generateLane = NULL;
//            generateLane = possibleLanes[order.back()];
//            order.pop_back();
//
//            if (generateLane == NULL)
//            {
//                skipVehicles.push_back(tmpVehicle);
//            }
//            else
//            {
//                // 経路選択は車両登場時に実行する
//                tmpVehicle->reroute(section, this);

//                // 登場時刻の記録
//                tmpVehicle->setStartTime(TimeManager::time());

//                // 道路上に登場
//                tmpVehicle->addToSection(roadMap, section, generateLane,
//                                         tmpVehicle->bodyLength()/2);
//                const_cast<Route*>(tmpVehicle->route())->setLastPassedIntersection(this);

//                bool result = ObjManager::addVehicleToReal(tmpVehicle);
//                assert(result);

//                // 作成した車両属性情報を出力する
//                /*
//                 * addVehicleToRealの後でなければIDが決まらないため，
//                 * ここで出力する
//                 */
//                VehicleIO::instance().writeVehicleStaticData(tmpVehicle);
//
//                if (GVManager::getFlag("FLAG_OUTPUT_GEN_COUNTER"))
//                {
//                    GeneratedVehicleData gvd;
//                    gvd.vehicle = tmpVehicle;
//                    gvd.lane = generateLane;
//                    gvd.headway = TimeManager::time()-_lastInflowTime;
//                    gvd.genTime = tmpVehicle->genTime();
//                    gvd.genInterval = gvd.genTime-_lastGenTime;
//                    _nodeGvd.push_back(gvd);
//                }
//                _lastInflowTime = TimeManager::time();
//                _lastGenTime    = tmpVehicle->genTime();
//            }
//        }
//    }

//    for (unsigned int i = 0; i < skipVehicles.size(); i++)
//    {
//        _waitingVehicles.push_front(skipVehicles[i]);
//    }

//    // 車両発生情報の出力
//    if (GVManager::getFlag("FLAG_OUTPUT_GEN_COUNTER")
//        && _isOutputGenerateVehicleData
//        && !_nodeGvd.empty())
//    {
//        GenerateVehicleIO::writeGeneratedVehicleData(this, &_nodeGvd);
//        _nodeGvd.clear();
//    }
//}

////======================================================================
//bool ODNode::isWaitingToPushVehicle() const
//{
//    return _isWaitingToPushVehicle;
//}

////======================================================================
//void ODNode::setWaitingToPushVehicle(bool flag)
//{
//#ifdef _OPENMP
//#pragma omp critical (setWaitingToPushVehicle)
//    {
//#endif //_OPENMP
//        _isWaitingToPushVehicle = flag;
//#ifdef _OPENMP
//    }
//#endif //_OPENMP
//}

////======================================================================
//void ODNode::deleteAgent()
//{
//    ITRMAPLAN itl;

//    for (itl=_lanes.begin(); itl!=_lanes.end(); itl++)
//    {
//        vector<RoadOccupant*>* agentsOfLane = (*itl).second->agents();
//        if (!agentsOfLane->empty())
//        {
//            (*itl).second->setUsed();
//        }
//        for (unsigned int i=0; i<agentsOfLane->size(); i++)
//        {
//            if (dynamic_cast<Vehicle*>((*agentsOfLane)[i]))
//            {
//                if (GVManager::getFlag("FLAG_OUTPUT_TRIP_INFO"))
//                {
//                    // 消去する前にトリップ長の出力
//                    VehicleIO::instance().writeVehicleDistanceData
//                        (dynamic_cast<Vehicle*>((*agentsOfLane)[i]));
//                }

//                ObjManager::deleteVehicle(
//                    dynamic_cast<Vehicle*>((*agentsOfLane)[i]));
//            }
//        }
//    }
//}

////======================================================================
//void ODNode::setOutputGenerateVehicleDataFlag(bool flag)
//{
//    _isOutputGenerateVehicleData = flag;
//}

////======================================================================
//void ODNode::clearGVD(GeneratedVehicleData* gvd)
//{
//    gvd->vehicle     = NULL;
//    gvd->lane        = NULL;
//    gvd->headway     = 0;
//    gvd->genTime     = 0;
//    gvd->genInterval = 0;
//}
