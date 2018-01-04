/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __AROUTER_H__
#define __AROUTER_H__

#include <string>
#include <vector>

class RoadMap;
class Intersection;
class Route;
class Section;
class OD;
class Vehicle;

/// 大域的経路探索を行うクラスの抽象基底クラス
/**
 * @sa Route
 * @ingroup Routing
 */
class ARouter
{
public:
    ARouter(Intersection*, Intersection*, RoadMap*) {};
    virtual ~ARouter(){};

    /// 担当する車両を設定する
    virtual void setVehicle(Vehicle* vehicle) = 0;

    /// 始点と終点および経由地を設定する
    virtual void setTrip(OD* od, RoadMap* roadMap) = 0;

    /// 選好を設定する
    /**
     * 重み付けに用いられるため，要素間の相対値が意味を持つ
     * それぞれの数値の意味はサブクラスを参照
     */
    virtual void setParam(const std::vector<double>&) = 0;

    /// 探索を行い，見つかった経路を@p result_routeに格納する
    /**
     *  @param start 探索を開始する交差点
     *  @param step 処理を打ち切る上限のステップ数
     *  @param[out] result_route 得られた経路を格納する
     *
     *  @note 探索失敗すると@p result_routeにはNULLが与えられる
     *  @warning 非NULLの@p result_routeは呼び出し側でdeleteすること
     */
    virtual bool search(const Intersection* start,
                        int step,
                        Route*& result_route) = 0;

    /// searchの@p section指定版
    // by uchida 2016/5/10
    // EV用に充電フラグを追加
    virtual bool search(const Section* section,
                        const Intersection* start,
                        int step,
                        Route*& result_route) = 0;

    virtual bool CSsearch(const Section* section,
                        const Intersection* start,
                        int step,
                        Route*& result_route,
                        std::string stopCS) = 0;

    // by uchida 2016/5/27
    /// 経路のGvalueを返す
    virtual double searchSegmentGV(const Intersection *start,
                                   const Intersection *goal,
                                   const Intersection *past,
                                   int step,
                                   std::string stopCS) = 0;

   //
   // by uchida 2017/2/26
   /// 経路のGvalueを返す(SubNode用)
   virtual double searchSegmentGV(const Intersection *start,
                                  const Intersection *goal,
                                  const Intersection *past,
                                  int step) = 0;

    /// 最後に通過した経由地を指定する
    /**
     * 指定された交差点が経由地リストになければ何もしない
     */
    virtual void setLastPassedStopPoint(const std::string passedInter) = 0;

    /// 経路探索の現在のステップ数を返す
    virtual int counter() const = 0;

    /// このクラスに設定された情報を返す
    virtual void printParam() const = 0;

    /// 始点を返す
    virtual const Intersection* start() const = 0;

    /// 終点を返す
    virtual const Intersection* goal() const = 0;

    /// 経路探索パラメータを返す
    virtual const std::vector<double> param() const = 0;

    /// ODデータを返す
    virtual OD* od() = 0;
};
#endif
