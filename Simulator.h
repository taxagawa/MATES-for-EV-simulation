/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include "RoadMap.h"
#include "TimeManager.h"
#include "VehicleFamily.h"
#include "VehicleIO.h"
#include "SignalIO.h"

class GenerateVehicleController;
class VehicleRunningMode;
class ODNode;
class Vehicle;

/**
 * @addtogroup Running
 * @brief シミュレーションの実行
 * @ingroup Procedure
 */

/// シミュレータ本体
/**
 * @ingroup Initialization Running
 */
class Simulator
{
public:
    Simulator();
    ~Simulator();

    //====================================================================
    /** @name シミュレーションの準備 */
    //@{

    /// 初期化は適切に完了しているか
    bool hasInit() const;

    /// ファイルを読み込んでRoadMapを作成する
    /**
     * @return 作成に成功したかどうか
     */
    bool getReadyRoadMap();

    /// by uchida 走行モードを作成する
    bool getReadyRunningMode();

    /// RoadsideUnitに関する初期設定を行う
    /**
     * @return 設定に成功したかどうか
     */
    bool getReadyRoadsideUnit();

    /// Vehicleに関する初期設定を行う
    /**
     * @return 設定に成功したかどうか
     */
    bool getReadyVehicles();
  
    /// レーンチェック、エラー時は表示確認のため run のみ止める
    void checkLane();

    /// レーンチェックエラー取得
    bool checkLaneError();

    //@}

    //====================================================================
    /** @name シミュレーションの実行 */
    //@{

    /// @p time までシミュレータを動かす
    bool run(ulint time);

    /// 1ステップ分シミュレータを動かす
    bool timeIncrement();

    //@}

public:
    //====================================================================
    /** @name ファイル入出力 */
    //@{

    /// 時系列データを出力する
    void writeResult() const;

    /// run_infoを出力する
    /**
     * @note
     * 本来はシミュレーション終了時に出力すればよいはずだが，
     * 実行時エラーが発生したりCtrl-Cで強制終了した場合に対応するため
     * 各タイムステップの処理が終わるたびに更新することにする．
     */
    void writeRunInfo() const;

    //@}

    //====================================================================
    /** @name 内部の地図や車両発生コントローラへのアクセッサ */
    //@{

    /// 地図を返す
    RoadMap* roadMap();

    /// 車両発生コントローラを返す
    GenerateVehicleController* generateVehicleController();

    /// 走行モードの速度テーブルを返す
    // GenerateVehicleRunningMode* generateVehicleRunningMode();

    //@}

private:
    /// 地図オブジェクト
    RoadMap* _roadMap;

    /// 車両発生を担当するコントローラ
    GenerateVehicleController* _genVehicleController;

    /// 走行モードの速度テーブル
    VehicleRunningMode* _runningMode;

    /// レーンチェックエラー、表示確認のため run のみ止める
    bool _checkLaneError;

    //====================================================================
    /** @name 入出力用オブジェクトとフラグ*/
    /// @{

    /// 車両情報の入出力を担当するオブジェクト
    VehicleIO* _vehicleIO;

    /// 信号情報の入出力を担当するオブジェクト
    SignalIO* _signalIO;
    //@}
};

#endif //__SIMULATOR_H__
