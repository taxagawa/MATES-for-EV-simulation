/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __ROUTER_H__
#define __ROUTER_H__

#include <string>
#include <vector>
#include <map>
#include "ARouter.h"
#include "OD.h"

class NodeAStar;
class Route;
class Intersection;
class Section;
class Tree;
class RoadMap;
class Section;
class Vehicle;

/**
 * @addtogroup Routing
 * @brief 経路と経路探索機能を定義するモジュール
 */

/// 大域的経路探索を行うクラスの実装
/**
 * @ingroup Routing
 */
class Router : public ARouter
{
public:
    Router(Intersection* start = NULL,
           Intersection* goal = NULL,
           RoadMap* = NULL);
    virtual ~Router();

    /// 担当する車両を設定する
    virtual void setVehicle(Vehicle* vehicle);

    /// 始点と終点および経由地を設定する
    /**
     * 始点・終点・経由地情報はODが持っている
     */
    virtual void setTrip(OD* od, RoadMap* roadMap);

    /// 経路選択パラメータの設定
    /**
     * このクラスではこのコンテナの要素数は6個まで考慮される。
     * 0番目は距離に関するコスト(大きいと最短経路の方が効用が高くなる)
     * 1番目は時間に関するコスト(大きいと最短時間の方が効用が高くなる)
     * 2番目は交差点での直進に関するコスト(大きいと直進が少ない方が効用が高くなる)
     * 3番目は交差点での左折に関するコスト(大きいと左折が少ない方が効用が高くなる)
     * 4番目は交差点での右折に関するコスト(大きいと右折が少ない方が効用が高くなる)
     * 5番目は道路の広さに関するコスト(大きいと道路が大きいほど効用が高くなる)
     */
    virtual void setParam(const std::vector<double>&);

    /// 探索を行い,見つかった経路を@p result_routesに格納する
    /**
     *  @param start 探索を開始する交差点
     *  @param step 処理を打ち切る上限のステップ数
     *  @param[out] result_routes 得られた経路を格納する
     *
     *  @note 探索失敗すると@p result_routesにはNULLが与えられる
     *  @warning 非NULLの@p result_routesは呼び出し側でdeleteすること
     */
    virtual bool search(const Intersection* start,
                        int step,
                        Route*& result_routes);

    /// searchの@p section指定版
    virtual bool search(const Section* section,
                        const Intersection* start,
                        int step,
                        Route*& route);

    /// searchの@p section指定版
    virtual bool CSsearch(const Section* section,
                        const Intersection* start,
                        int step,
                        Route*& route,
                        std::string stopCS);//EV

    /// 最後に通過した経由地を指定する
    /**
     * 指定された交差点が経由地リストになければ何もしない
     */
    virtual void setLastPassedStopPoint(const std::string passedInter);

    /// 経路探索の現在のステップ数を返す
    virtual int counter() const;

    /// このクラスに設定された情報を返す
    virtual void printParam() const;

    /// 始点を返す
    virtual const Intersection* start() const;

    /// 終点を返す
    virtual const Intersection* goal() const;

    /// 経路探索用パラメータを返す
    virtual const std::vector<double> param() const;

    virtual OD* od();

protected:
    /// 担当する車両
    Vehicle* _vehicle;

    /// 経路選択パラメータ
    std::vector<double> _a;

    /// 全てのリンクにおける車の平均速度の最高値
    static double _vel;

    /// 探索の対象となる道路地図
    RoadMap* _map;

    /// OD:出発地、目的地、経由地情報
    OD _od;

    /// 出発地
    Intersection* _start;

    /// 目的地
    Intersection* _goal;

    /// Tree構造
    Tree* _tree;

    /// 目的CS
    std::string _stopCS;

    /// いくつのステップ進んでいるかを示す
    int _counter;

    /** @name search() から呼び出されるユーティリティ関数群 */
    //@{

    /// スタートノードの作成および設定
    /**
     * 指定されたstartを用いてスタートノードを作成する
     *
     * @attention 先にsetParamが実行されていなければならない
     */
    virtual void initNode(const Intersection* start,
                          const Intersection* goal);

    /// スタートノードの作成および設定
    virtual void initNode(const Section* section,
                          const Intersection* start,
                          const Intersection* goal);

    /// 探索を行い,見つかった複数の経路を@p result_routesに格納する
    /**
     * startからgoalまでの経路を探索し，
     * 見つかった経路を@p result_routesに格納する．
     * 経由地を考慮した経路探索に用いる．
     */
    virtual void searchSegment(const Intersection *start,
                               const Intersection *goal,
                               const Intersection *past,
                               int step,
                               std::vector<Route*>& result_routes);

    /// 経路のGvalueを返す
    // expand関連などでstopCSは重要だった…忘れてた
    // commented by uchida 2016/5/31
    virtual double searchSegmentGV(const Intersection *start,
                                   const Intersection *goal,
                                   const Intersection *past,
                                   int step,
                                   std::string stopCS);

   // by uchida 2017/2/26
   // Subnode.cppのnearestCS用経路探索
   virtual double searchSegmentGV(const Intersection *start,
                                  const Intersection *goal,
                                  const Intersection *past,
                                  int step);

    /// ノードを展開する
    bool expand(NodeAStar*, std::vector<NodeAStar*>*, const Intersection*);
    bool expand_noV(NodeAStar*, std::vector<NodeAStar*>*, const Intersection*);

    /// 目的地ノードかどうかを判定する
    bool isGoal(NodeAStar*, const Intersection*);

    /// A*アルゴリズムを終了させて良いか判断する
    bool isEnd(const Intersection* goal);

    /// 評価関数値gを算出する
    double funcG(NodeAStar*);

    /// 評価関数値hを算出する
    double funcH(NodeAStar*, const Intersection*);

    /// 評価関数値fを算出する
    double funcF(NodeAStar*, const Intersection*);

    //@}
};

#endif
