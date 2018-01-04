/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __GENERATE_VEHICLE_CONTROLLER_H__
#define __GENERATE_VEHICLE_CONTROLLER_H__

#include "GeneratingTable.h"
#include "VehicleFamily.h"
#include <map>
#include <string>
#include <vector>

class RoadMap;
class VehicleRunningMode;
class Intersection;
class ODNode;
class Section;
class Vehicle;
class VehicleEV;
class RandomGenerator;

/// 車両の発生を制御するクラス
/**
 * @ingroup Running
 */
class GenerateVehicleController
{
public:
    /// 唯一のインスタンスを返す
    static GenerateVehicleController& instance();

    /// RoadMapをセットする
    void setRoadMap(RoadMap* roadMap);

    /// RunningModeをセットする
    // by uchida
    void setRunningMode(VehicleRunningMode* runningMode);

    /// 車両発生に関する初期設定を行う
    /**
     * @return 設定に成功したかどうか
     */
    bool getReadyGeneration();

    //======================================================================
    /** @name エージェントの発生 */
    //@{

    /// 車両を発生させる
    void generateVehicle();

    /// 手動で車両を発生させる
    void generateVehicleManual(const std::string& startId,
                               const std::string& goalId,
                               std::vector<std::string> stopPoints,
                               VehicleType vehicleType,
                               std::vector<double> params);

    /// ゴーストEVを発生させる
   void generateVehicleGhostEV(Vehicle* vehicle);


    //@}

private:
    GenerateVehicleController();
    ~GenerateVehicleController();

    //======================================================================
    /** @name エージェントの発生に用いるprivate関数 */
    //@{

    /// ランダムテーブルを設定する
    void _createRandomTable();

    /// 条件を満たすGTセルを有効化する
    /**
     * Generate Table，Default Generate Table，Fixed Generate Tableが持つ各GTCellおよび
     * 発生交通量がファイルで指定されていない各交差点のGTCellのうち，
     * 現在時刻に適用されるものを有効化し，最初の車両発生時刻を決定する．
     * 以降，車両発生時に次の車両発生時刻を決定する
     */
    void _activatePresentGTCells();

    /// 次の車両発生時刻を設定する
    /**
     * 車両の発生はポワソン分布を仮定する．
     * すなわち，単位時間あたりの車両発生台数がポワソン分布に従う．
     * このとき，車両発生の時間間隔は指数分布となる．
     *
     * @param startTime 時間間隔の基準となる時刻
     * @param cell 処理するGTCell
     * @return 有効な時刻を設定できたか
     */
    bool _addNextGenerateTime(ulint startTime, const GTCell* cell);

    /// 発生時刻が厳密に指定されたGTCellに従い車両発生情報を入力する
    bool _addFixedGenerateTime(const GTCell* cell);

    /// 車両を発生させる
    void _generateVehicleFromQueue();

    /// 優先的に車両を発生させる
    void _generateVehicleFromPriorQueue();

    /// 車両を生成し，経路選択する
    /**
     * 乱数によって車両生成フラグが立った後の処理．
     * ObjManager::createVehicle()で車両を生成し，経路選択．
     */
    Vehicle* _createVehicle(ODNode* start,
                            ODNode* goal,
                            Intersection* past,
                            Section* section,
                            OD* od,
                            VehicleType vehicleType);

    /// 車両を生成し，経路選択する
    /**
     * 引数として経路選択パラメータを与える．
     */
    Vehicle* _createVehicle(ODNode* start,
                            ODNode* goal,
                            Intersection* past,
                            Section* section,
                            OD* od,
                            VehicleType vehicleType,
                            std::vector<double> params);

    // added by uchida 2017/8/17
    // ゴーストEVを追加する
    // TODO 削除時の処理
    Vehicle* _createGhostEV(Vehicle* vehicle);

    /// 車両の属性を設定
    void _setVehicleStatus(Vehicle* vehicle);

    /// ゴールをランダムに決定する
    ODNode* _decideGoalRandomly(ODNode* start);

    //@}

    /// ODノードをレベル分けする
    void _classifyODNodes();

    /// ODノードのスタートレベルを返す
    /**
     * 単路への流入点でstartLebelを決定する
     * "単路の"流入点・流出点は"ODノードの"流入点・流出点と反対
     */
    int _odNodeStartLevel(ODNode* node) const;

    /// ODノードのゴールレベルを返す
    /**
     * 単路からの流出点でgoalLevelを決定する
     * "単路の"流入点・流出点は"ODノードの"流入点・流出点と反対
     */
    int _odNodeGoalLevel(ODNode* node) const;

    /// Router用パラメータの設定
    void _readRouteParameter();

private:
    /// 地図オブジェクト
    RoadMap* _roadMap;

    /// 走行モードオブジェクト
    // by uchida
    VehicleRunningMode* _runningMode;

    /// 車両発生イベント管理キュー
    /**
     * mapのインデックスは車両発生時刻，値は該当するGTCell
     */
    std::multimap<unsigned long, const GTCell*> _generateVehicleQueue;

    /// 優先車両発生イベント管理キュー
    /**
     * mapのインデックスは車両発生時刻，値は該当するGTCell
     *
     * 発生時刻が厳密に指定されたバスなどはpush_backでなく
     * push_frontする必要があるため別に管理する
     */
    std::multimap<unsigned long, const GTCell*> _generateVehiclePriorQueue;

    /// 車両発生を待つODノード
    std::vector<ODNode*> _waitingODNodes;

    /** @name 車両発生定義テーブル */
    //@{
    GeneratingTable _table;        //!< 始点終点の双方を設定
    GeneratingTable _defaultTable; //!< 始点のみ設定
    GeneratingTable _fixedTable;   //!< 時刻，始点，終点を指定
    GeneratingTable _randomTable;  //!< 無設定（デフォルト交通量）
    //@}

    /// レベル分けされたOriginノード
    std::vector<ODNode*> _startLevel[3];

    /// レベル分けされたDestinationノード
    std::vector<ODNode*> _goalLevel[3];

    /// デフォルトのレベル別交通量
    int _defaultTrafficVolume[3];

    /// Router用パラメータセット
    std::vector<std::vector<double> > _vehicleRoutingParams;

    /// 乱数生成器
    RandomGenerator* _rnd;
};

#endif //__GENERATE_VEHICLE_CONTROLLER_H__
