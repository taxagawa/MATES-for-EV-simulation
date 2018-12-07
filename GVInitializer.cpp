/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "GVInitializer.h"
#include "GVManager.h"
#include <iostream>

using namespace std;

//======================================================================
void GVInitializer::init(const string& dataPath)
{
    GVManager::setNewString("DATA_DIRECTORY",
                            dataPath);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // フラグの定義
    /*
     * 既に別の箇所で定義されている場合は再定義できない
     */

    // 詳細情報を出力するか
    GVManager::setNewFlag("FLAG_VERBOSE", true);

    // 地図情報を入力するか
    GVManager::setNewFlag("FLAG_INPUT_MAP", true);

    // 信号情報を入力するか
    GVManager::setNewFlag("FLAG_INPUT_SIGNAL", true);

    // 車両情報を入力するか
    GVManager::setNewFlag("FLAG_INPUT_VEHICLE", true);

    // 車両発生情報が定義されていない交差点から車両を発生させるか
    GVManager::setNewFlag("FLAG_GEN_RAND_VEHICLE", true);

    // 時系列詳細データを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_TIMELINE_D", true);

    // 時系列統計データを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_TIMELINE_S", true);

    // 計測機器の詳細データを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_MONITOR_D", true);

    // 計測機器の統計データを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_MONITOR_S", true);

    // エージェント発生データを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_GEN_COUNTER", true);

    // エージェントの走行距離，旅行時間を出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_TRIP_INFO", true);

    //by uchida
    // EVを発生・CSを設置するか
    GVManager::setNewFlag("FLAG_GEN_EVs", false);
    GVManager::setNewFlag("FLAG_GEN_CSs", false);
    // by takusagawa 2018/10/8
    // 待ち行列の長さを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_WAITING_LINE", false);
    // by takusagawa 2018/11/1
    // eオプション指定時に,予測待ち時間を使うか,充電フラグが立った瞬間の待ち時間を使うかを
    // 選択できるようにするためのフラグ. init.txtにおいてtrueに変更可.
    GVManager::setNewFlag("FLAG_USE_FUTURE_WAITING_LINE_MODE", false);
    // by takusagawa 2018/11/1
    // CS経由経路選択の際に予測した待ち時間を使用するかどうか.
    // eオプションを指定し,かつinit.txtにおいて上のFLAG_USE_FUTURE_WAITING_LINE_MODEを
    // trueにしておかなければならない.
    GVManager::setNewFlag("FLAG_USE_FUTURE_WAITING_LINE", false);
    // by takusagawa 2018/12/7
    // 予測待ち時間を算出方法を指定する.
    // 0が一次点対称近似(default)
    // 1が一次直線近似
    // 2が二次曲線近似
    GVManager::setNewNumeric("WAITING_TIME_APPROXIMATION_DEGREE", 0);
    //by uchida 2017/12/26
    //CSの充電出力を出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_CSs", false);
    //出力の集計間隔（標準では10分）
    GVManager::setNewNumeric("OUTPUT_CSs_INTERVAL", 600000);
    // 待ち行列の出力の集計間隔(標準では60秒)
    GVManager::setNewNumeric("OUTPUT_WAITING_LINE_INTERVAL", 60000);

    // by uchida 2017/3/6
    // 時系列のCS配置スコアを出力するか
    GVManager::setNewFlag("FLAG_OUTPUT_SCORE", false);


    // by uchida 2017/3/6
    GVManager::setNewString("RESULT_SCORE_PREFIX",
                            "score");
    GVManager::setNewNumeric("SCORING_METHOD",
                             1);


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // フラグの定義 (デバッグ用)
    /*
     * デバッグ用であり，trueにする場合は注意が必要
     */
    // 時刻表を持った車両を一斉に発生させるか(デバッグ用)
    GVManager::setNewFlag("DEBUG_FLAG_GEN_FIXED_VEHICLE_ALL_AT_ONCE",
                          false);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // グローバル変数設定ファイル
    GVManager::setNewString("GV_INIT_FILE",
                            dataPath + "init.txt");

    // 地図に関連するファイル
    GVManager::setNewString("MAP_POSITION_FILE",
                            dataPath + "mapPosition.txt");
    GVManager::setNewString("MAP_NETWORK_FILE",
                            dataPath + "network.txt");
    GVManager::setNewString("OD_NODE_EXCLUSION_FILE",
                            dataPath + "odNodeExclusion.txt");
    GVManager::setNewString("SPEED_LIMIT_FILE",
                            dataPath + "speedLimit.txt");
    GVManager::setNewString("TRAFFIC_CONTROL_SECTION_FILE",
                            dataPath + "trafficControlSection.txt");
    GVManager::setNewString("CS_LIST_FILE",
                            dataPath + "csList.txt");

    // 車両発生に関するファイル
    GVManager::setNewString("GENERATE_TABLE",
                            dataPath + "generateTable.txt");
    GVManager::setNewString("DEFAULT_GENERATE_TABLE",
                            dataPath + "defaultGenerateTable.txt");
    GVManager::setNewString("FIXED_GENERATE_TABLE",
                            dataPath + "fixedGenerateTable.txt");

    // 車種に関するファイル
    GVManager::setNewString("VEHICLE_FAMILY_FILE",
                            dataPath + "vehicleFamily.txt");

    // 自動車エージェントの経路選択パラメータに関するファイル
    GVManager::setNewString("VEHICLE_ROUTE_PARAM_FILE",
                            dataPath + "vehicleRoutingParam.txt");


    // 自動車エージェントの経路選択確率に関するファイル
    GVManager::setNewString("ROUTING_PROBABILITY_FILE",
                            dataPath + "vehicleRoutingProbability.txt");

    // by uchida
    // 最近傍CSに関するファイル
    GVManager::setNewString("RUNNING_MODE_FILE",
                            dataPath + "runningMode.txt");

    // by uchida 2017/2/27
    // 自動車の走行モードに関するファイル
    GVManager::setNewString("NEAREST_CS_FILE",
                            dataPath + "nearestCS.txt");

    // 路側器に関するファイル
    GVManager::setNewString("GENCOUNTER_FILE",
                            dataPath + "genCounter.txt");
    GVManager::setNewString("DETECTOR_FILE",
                            dataPath + "detector.txt");

    // 信号に関するディレクトリorファイルor拡張子
    GVManager::setNewString("SIGNAL_CONTROL_DIRECTORY",
                            dataPath + "signals/");
    GVManager::setNewString("SIGNAL_CONTROL_FILE_DEFAULT",
                            "default");
    GVManager::setNewString("SIGNAL_ASPECT_FILE_DEFAULT_PREFIX",
                            "defaultInter");
    GVManager::setNewString("CONTROL_FILE_EXTENSION",
                            ".msf");
    GVManager::setNewString("ASPECT_FILE_EXTENSION",
                            ".msa");

    // 交差点属性指定ファイル
    GVManager::setNewString("INTERSECTION_ATTRIBUTE_DIRECTORY",
                            dataPath + "intersection/");

    // 道路形状に関するファイル
    GVManager::setNewString("INTERSECTION_STRUCT_FILE",
                            dataPath + "intersectionStruct.txt");
    GVManager::setNewString("SECTION_STRUCT_FILE",
                            dataPath + "sectionStruct.txt");

    // 出力先
    string resultPath = dataPath + "result/";
    GVManager::setNewString("RESULT_OUTPUT_DIRECTORY",
                            resultPath);
    GVManager::setNewString("RESULT_TIMELINE_DIRECTORY",
                            resultPath + "timeline/");
    GVManager::setNewString("RESULT_IMG_DIRECTORY",
                            resultPath + "img/");
    GVManager::setNewString("RESULT_INSTRUMENT_DIRECTORY",
                            resultPath + "inst/");
    GVManager::setNewString("RESULT_DETECTORD_PREFIX",
                            "detD");
    GVManager::setNewString("RESULT_DETECTORS_PREFIX",
                            "detS");
    GVManager::setNewString("RESULT_GENCOUNTER_PREFIX",
                            "gen");
    GVManager::setNewString("RESULT_RUN_INFO_FILE",
                            resultPath + "runInfo.txt");
    GVManager::setNewString("RESULT_NODE_SHAPE_FILE",
                            resultPath + "nodeShape.txt");
    GVManager::setNewString("RESULT_LINK_SHAPE_FILE",
                            resultPath + "linkShape.txt");

    GVManager::setNewString("RESULT_SIGNAL_COUNT_FILE",
                            resultPath + "signalCount.txt");

    GVManager::setNewString("RESULT_VEHICLE_ATTRIBUTE_FILE",
                            resultPath + "vehicleAttribute.txt");
    GVManager::setNewString("RESULT_VEHICLE_TRIP_FILE",
                            resultPath + "Trip/vehicleTrip0000.txt");
    GVManager::setNewString("RESULT_VEHICLE_COUNT_FILE",
                            resultPath + "vehicleCount.txt");

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 定数の定義
    /*
     * 後でGVManager::setVariablesFromFile()によって上書きされる
     */

    /* シミュレーションの実行に関するもの */

    // 出力ファイルに各種コメントを付して出力する
    GVManager::setNewNumeric("OUTPUT_COMMENT_IN_FILE", 0);

    /* エージェントに関するもの */

    // 自動車の反応遅れ時間[sec]
    GVManager::setNewNumeric("REACTION_TIME_VEHICLE", 0.74);

    // 自動車が交差点で右折する場合などの
    // 対向車とのギャップアクセプタンス[sec]
    GVManager::setNewNumeric("GAP_ACCEPTANCE_VEHICLE_CROSS", 3.0);

    // 普通車(VehicleFamily::passenger())の最大加速度，減速度[m/(sec^2)]
    GVManager::setNewNumeric("MAX_ACCELERATION_PASSENGER", 3.0);
    GVManager::setNewNumeric("MAX_DECELERATION_PASSENGER", -5.0);

    // EV普通車(VehicleFamily::EV_passenger())の最大加速度，減速度[m/(sec^2)]
    GVManager::setNewNumeric("MAX_ACCELERATION_EV_PASSENGER", 3.0);
    GVManager::setNewNumeric("MAX_DECELERATION_EV_PASSENGER", -5.0);

    // バス(VehicleFamily::bus())の最大加速度，減速度[m/(sec^2)]
    GVManager::setNewNumeric("MAX_ACCELERATION_BUS", 3.0);
    GVManager::setNewNumeric("MAX_DECELERATION_BUS", -5.0);

    // 大型車(VehicleFamily::truck())の最大加速度，減速度[m/(sec^2)]
    GVManager::setNewNumeric("MAX_ACCELERATION_TRUCK", 3.0);
    GVManager::setNewNumeric("MAX_DECELERATION_TRUCK", -5.0);

    // 車線変更時に与える横向きの速度[km/h]
    GVManager::setNewNumeric("ERROR_VELOCITY", 7.5);

    // 発生端点からL[m]以内の車両は出力しない
    GVManager::setNewNumeric("NO_OUTPUT_LENGTH_FROM_ORIGIN_NODE", 0);

    // 車両ログの追加情報を出力する
    GVManager::setNewNumeric("OUTPUT_VEHICLE_EXTENSION", 0);

    // 普通車(VehicleFamily::passenger())のサイズ[m]
    GVManager::setNewNumeric("VEHICLE_LENGTH_PASSENGER", 4.400);
    GVManager::setNewNumeric("VEHICLE_WIDTH_PASSENGER",  1.830);
    GVManager::setNewNumeric("VEHICLE_HEIGHT_PASSENGER", 1.315);

    // EV普通車(VehicleFamily::EV_passenger())のサイズ[m]
    GVManager::setNewNumeric("VEHICLE_LENGTH_EV_PASSENGER", 4.400);
    GVManager::setNewNumeric("VEHICLE_WIDTH_EV_PASSENGER",  1.830);
    GVManager::setNewNumeric("VEHICLE_HEIGHT_EV_PASSENGER", 1.315);

    // バス(VehicleFamily::bus())のサイズ[m]
    GVManager::setNewNumeric("VEHICLE_LENGTH_BUS", 8.465);
    GVManager::setNewNumeric("VEHICLE_WIDTH_BUS",  2.230);
    GVManager::setNewNumeric("VEHICLE_HEIGHT_BUS", 3.420);

    // 大型車(VehicleFamily::truck())のサイズ[m]
    GVManager::setNewNumeric("VEHICLE_LENGTH_TRUCK", 8.465);
    GVManager::setNewNumeric("VEHICLE_WIDTH_TRUCK",  2.230);
    GVManager::setNewNumeric("VEHICLE_HEIGHT_TRUCK", 3.420);

    // ランダムに発生させる車両の交通量の係数
    GVManager::setNewNumeric("RANDOM_OD_FACTOR", 1.0);

    // generateTable, gefaultGenerateTableによって発生させる交通量の係数
    GVManager::setNewNumeric("TABLED_OD_FACTOR", 1.0);

    // 交錯を厳密に評価
    GVManager::setNewNumeric("STRICT_COLLISION_CHECK", 1);

    // 速度履歴を保存するか
    GVManager::setNewFlag("VEHICLE_VELOCITY_HISTORY_RECORD", true);
    // 速度履歴を保存するステップ数
    GVManager::setNewNumeric("VEHICLE_VELOCITY_HISTORY_SIZE", 180);
    // 速度履歴を保存するステップ間隔
    GVManager::setNewNumeric("VEHICLE_VELOCITY_HISTORY_INTERVAL", 10);

    /* 道路に関するもの */

    // 右折専用レーンの標準長さ[m]
    GVManager::setNewNumeric("RIGHT_TURN_LANE_LENGTH", 30);

    // 標準制限速度[km/h]
    /* ただしSPEED_LIMIT_INTERSECTIONが用いられることはほとんど無く，
     * 右左折時は下のVELOCITY_AT〜が使われ，
     * 直進時は次の単路でのSPEED_LIMITが参照される．*/
    GVManager::setNewNumeric("SPEED_LIMIT_SECTION", 60);
    GVManager::setNewNumeric("SPEED_LIMIT_INTERSECTION", 60);

    // 徐行速度[km/h]
    GVManager::setNewNumeric("VELOCITY_CRAWL", 10);

    // 右左折時の速度[km/h]
    GVManager::setNewNumeric("VELOCITY_AT_TURNING_RIGHT", 40);
    GVManager::setNewNumeric("VELOCITY_AT_TURNING_LEFT",  40);

    // 車両発生時の制限速度[km/h]、負ならなし
    GVManager::setNewNumeric("GENERATE_VELOCITY_LIMIT", -1);

    // 右左折時の最小ヘッドウェイ[秒]
    GVManager::setNewNumeric("MIN_HEADWAY_AT_TURNING", 1.7);

    // 車両発生量既定値[台/h]、入口レーン 3 以上 / 2 / 1、基本交通容量の10%
    GVManager::setNewNumeric("DEFAULT_TRAFFIC_VOLUME_WIDE",   660);
    GVManager::setNewNumeric("DEFAULT_TRAFFIC_VOLUME_NORMAL", 440);
    GVManager::setNewNumeric("DEFAULT_TRAFFIC_VOLUME_NARROW", 125);

    // 標準のレーン幅、歩道幅、横断歩道幅、路側幅
    GVManager::setNewNumeric("DEFAULT_LANE_WIDTH",      3.5);
    GVManager::setNewNumeric("DEFAULT_SIDEWALK_WIDTH",  5.0);
    GVManager::setNewNumeric("DEFAULT_CROSSWALK_WIDTH", 5.0);
    GVManager::setNewNumeric("DEFAULT_ROADSIDE_WIDTH",  1.0);

    // 単路の歩道を自動設定する際のレーン数、-1 なら自動設定なし
    /* 自動設定ありの場合、単路の全レーン数がこれ以上なら歩道を設定する
     * これ以下なら車道通行可能にする */
    GVManager::setNewNumeric("AUTO_SIDEWALK_SECTION_LANE", -1);

    // 道路エンティティの厳密な内部判定、1 ならあり
    /* 凹型の道路エンティティでも判定可能になる、ただし処理速度は遅い */
    GVManager::setNewNumeric("ROAD_ENTITY_STRICT_JUDGE_INSIDE", 1);

    // 交差点サイズ制限、中心からの距離[m]
    GVManager::setNewNumeric("INTERSECTION_SIZE_LIMIT", 20);

    // リンク旅行時間の更新間隔[秒]
    GVManager::setNewNumeric("INTERVAL_RENEW_LINK_TRAVEL_TIME", 300);

#ifdef _OPENMP
    /* マルチスレッドに関するもの */

    // スレッド数，0ならomp_get_num_procs()が返すプロセッサ数
    /*
     * 0の場合はAppMates::initの中でresetする
     */
    GVManager::setNewNumeric("NUM_THREAD", 1);//デバグのためシングルスレッド化（15.11.9）

#else //_OPENMP

    // シングルスレッドであればスレッド数は1
    GVManager::setNewNumeric("NUM_THREAD", 1);

#endif //_OPENMP
}
