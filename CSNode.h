/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __CS_NODE_H__
#define __CS_NODE_H__

#include "Intersection.h"
#include "ODNode.h"
#include "TimeManager.h"
#include <string>
#include <deque>
#include <vector>

class RoadMap;
class Vehicle;
class RandomGenerator;

/// CSノードクラス
/*
 * Intersectionの派生クラスのODNodeの派生クラス
 *
 * @ingroup roadNetwork
 */
class CSNode : public ODNode{
public:
    CSNode(const std::string& id,
           const std::string& type,
           RoadMap* parent);
    ~CSNode();

    // ランダムなCSノードを返す
    CSNode* getRandCS();

    // 充電電力量（瞬時）[Wsec]を合計する
    void sumCharge(double chargingValue);

    // 充電電力量を初期化する
    void initCharge();

    // by takusagawa 2018/3/26
    // 充電電力量（瞬時）[Wsec]を返す
    double instantaneousCharge();

    // by uchida 2017/11/24
    // 充電電力を時間方向に積算する．
    void integrated(double instantaneousCharge);

    // 積算充電電力を返す
    double integratedCharge();

    // 積算充電電力を初期化する
    void initIntegrated();

    // 充電電力量（積算）[Wsec]
    double _integratedCharge;

    // 待機列
    // commented by takusagawa 2018/9/28
    // 実体はVehicleEV
    vector<Vehicle*> waitingLine;

    // 待機列に追加する
    void addEV(Vehicle* vehicle);

    // 待機列から削除する
    void removeEV();

    // by uchida 2017/6/23
    // CSの収容台数
    int _capacity;
    // kW性能
    double _outPower;

    // 収容台数を設定する
    void setCapacity(int capacity);
    // kW性能を設定する
    void setOutPower(double outPower);
    // kW性能を返す
    double outPower();

    // by takusagawa 2018/9/25
    // 推定待ち時間[sec]
    double _estimatedWaitingTime;

    // 推定待ち時間を返す
    double estimatedWaitingTime() const;
    // 推定待ち時間を計算する
    void estimatedWaitingTimeCalc();

    // by takusagawa 2018/10/8
    // 待ち行列の長さを返す
    int waitingLineSize() const;

    // by takusagawa 2018/11/1
    // 過去の待ち時間履歴
    vector<double> waitingTimeHistory;

    // by takusagawa 2018/11/1
    // 待ち時間履歴の最大保持数
    // 一応double型にする.
    // 2018/11/2 やはりint型に.
    int waitingTimeHistoryMaxSize;

    // by takusagwa 2018/11/1
    // waitingTimeHistoryに推定待ち時間を追加
    void addWaitingTimeHistory(double estimatedTime);

    // by takusagawa 2018/11/2
    // 未来の予測待ち時間のリスト.
    // 先頭の要素が最も近い未来の予測待ち時間になる.
    // (CS_WAITING_TIME_UPDATE_INTERVAL)ミリ秒ごとに中身がすべて更新される.
    vector<double> futureWaitingTimeList;

    // by takusagawa 2018/11/2
    // 未来の予測待ち時間のリストを作成する.
    // 予測はとりあえず一次近似.
    void createFutureWaitingTimeList();

    // by takusagawa 2018/11/4
    // CSまでの推定旅行時間から,予想到着時刻における待ち時間を返す
    double estimatedFutureWaitingTime(double cost);

    // by takusagwa 2018/11/12
    // 呼びだされた時刻の10分後の予測値を返す
    double returnPredictionWaitingTime() const;

    // by takusagwa 2018/11/6
    // 捌け台数
    // int _servedEV;

    // by takusagwa 2018/11/6
    // 捌け台数を返す
    // int servedEV() const;

//    //====================================================================
//    /** @name 車両の発生と消去にかかわる関数 */
//    /// @{

//    /// _waitingVehiclesに車両がセットされているか
//    bool hasWaitingVehicles() const;

//    /// _waitingVehiclesに車両を追加する
//    void addWaitingVehicle(Vehicle* vehicle);

//    /// _waitingVehiclesの先頭に車両を追加する
//    /**
//     * マニュアルで追加するときやバスなど
//     */
//    void addWaitingVehicleFront(Vehicle* vehicle);

//    /// _waitingVehicleから車両をポップしシミュレーションに登場させる
//    void pushVehicleToReal(RoadMap* roadMap);

//    /// ODノードがpushVehicleToRealの対象となっているか
//    bool isWaitingToPushVehicle() const;

//    /// pushVehicleToRealフラグをセットする
//    void setWaitingToPushVehicle(bool flag);

//    /// レーン上にいるエージェントを消去する
//    void deleteAgent();

//    /// @}

//    /// 発生車両データを格納する構造体
//    struct GeneratedVehicleData
//    {
//        Vehicle* vehicle;
//        Lane* lane;
//        ulint headway;
//        ulint genTime;
//        ulint genInterval;
//    };

//    /// 発生車両データ出力フラグを設定する
//    void setOutputGenerateVehicleDataFlag(bool flag);

//    /// 発生車両データを格納する構造体を初期化する
//    void clearGVD(GeneratedVehicleData* gvd);

//protected:
//    /// シミュレーションへの登場を待つ車両【メインコンテナ】
//    std::deque<Vehicle*> _waitingVehicles;

//    /// このODノードがpushVehicleToRealの対象となっているか
//    bool _isWaitingToPushVehicle;

//    /// 直前の車両を発生させた時刻
//    ulint _lastGenTime;

//    /// 直前の車両を流入させた時刻
//    ulint _lastInflowTime;

//    /// 発生車両データ
//    std::vector<GeneratedVehicleData> _nodeGvd;

//    /// 発生車両データを出力するかどうか
//    bool _isOutputGenerateVehicleData;

//    /// 乱数生成器
//    RandomGenerator* _rnd;

//    //====================================================================
//public:
//    /**
//     * @name ODノードの詳細構造を作成する関数群
//     * @note 長いので別ファイルで定義する
//     * @sa IntersectionBuilder.cpp
//     */
//    /// @{

//    /// 車道頂点の作成
//    bool createDefaultStreetVertices();
//    /// 頂点の作成
//    bool createVertices();
//    /// 境界の作成
//    bool createDefaultBorders();
//    /// サブセクションの作成
//    bool createDefaultSubsections();
//    /// レーンの作成
//    bool createDefaultLanes();
//    /// 属性指定ファイルからレーンの作成
//    bool createLanesFromFile();

//    /// @}

};
#endif //__CS_NODE_H__
