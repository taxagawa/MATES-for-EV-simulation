/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __VEHICLE_EV_H__
#define __VEHICLE_EV_H__

#include "Vehicle.h"
#include "SubNode.h"

using namespace std;

class VehicleEV : public Vehicle{
 public:
  VehicleEV();
  virtual ~VehicleEV();

  /// 移動する
  virtual void run();

//  //====================================================================
//  /** @name EVの性能に関する関数 */
//  /// @{

  /// バッテリー残量を返す
  double batteryRemain();

  /// 充電率を返す
  virtual double SOC();

  /// 初期充電率を返す
  double initialSoC();

  /// 電池特性を返す
  // Battery Characteristics
  double BC();

  /// 回生率を返す
  double regenerativeRate();

  void charge();

  // by takusagawa 2018/01/12
  void setInitSoC();

  // by takusagawa 2018/9/27
  // _batteryCapacityを返す
  double getBatteryCapacity() const;

  // 目的のCSに向かうため経路を探索する
//  bool CSreroute(const Section* section, const Intersection* start, string stopCS);

  // CSを探索できるように用意
//  void _runIntersection2Section();

//  /// バッテリーを充電する
//  void chargeBattery(double charge);

//  /// 充電フラグを返す
//  int chargeFlag() const;

//  /// 充電ステーション探索開始時刻を返す
//  ulint startSearchTime() const;

//  /// 充電ステーション探索開始時刻を設定する
//  void setStartSearchTime(ulint time);

//  /// 充電ステーション到着時刻を返す
//  ulint arrivalTime() const;

//  /// 充電ステーション到着時刻を設定する
//  void setArrivalTime(ulint time);

//  /// @}

 private:
//  bool _searchStation();

//    /// 航続距離[m]
//    double _cruise;

  /// 重量[kg]（車体重量+乗員重量）
  double _mass;

  /// 全面投影面積[m2]
  double _fronralProjectedArea;

  // 回生率
  double _regenerativeRate;

  /// 電池容量[Wsec]
  // CSNode::estimatedWaitingTimeCalc()内でgetするためにVehicle.hに移動
  // by takusagawa 2018/9/27
  // double _batteryCapacity;

  /// 充電残量[Wsec]
  double _batteryRemain;

  /// 充電電力量[Wsec]
  double _chargingValue;

  /// 充電率
  double _stateOfCharge;

  /// 初期充電率
  double _initialSoC;

  /// 1タイムステップの電力消費量[W]（瞬時値）
  double instantaneousValue;

  /// 電装品＋空調等の消費電力[Wsec]
  double accessory;

//  /// 充電閾値
//  double _chargeThreshold;

//  /// 充電探索モード
//  /**
//   * @note 0:充電必要なし、1:消極的探索、2:積極的探索
//   */
//  int _chargeFlag;

//  /// 充電ステーション探索開始時刻
//  ulint _startSearchTime;

//  /// 充電ステーション到着時刻
//  ulint _arrivalTime;

    /// 車体色を設定する]

 protected:
  Vehicle* _parent;

public:
    //by uchida 2017/2/2
    //====================================================================
    bool threshold();

    double _threshold;

};

#endif //__VEHICLE_EV_H__
