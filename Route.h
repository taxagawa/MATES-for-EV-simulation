/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __ROUTE_H__
#define __ROUTE_H__
#include <iostream>
#include <vector>

class Intersection;

/// 大域的経路を表すクラス
/**
 * 交差点の順序列で表現される。
 *  ARouter で探索した経路を1つ格納する。
 *
 *  @ingroup Routing
 */
class Route
{
public:
    Route();
    ~Route();
    /// 最後尾に交差点を追加
    void push(Intersection*);
    /// 最後尾に複数の交差点を追加
    void push(std::vector<Intersection*>& isec_vec);

    /// 先頭の交差点を削除
    Intersection* pop();

    /// 最後尾の交差点を取得する
    Intersection* back();

    /// 最後に通りすぎた交差点を設定
    /**
     * 最後に通りすぎた交差点を最初のpassedに設定。
     * 指定された交差点がなければ何もしない。
     * (passeddを通り過ぎるときに呼ばれる。)
     */
    void setLastPassedIntersection(Intersection* passed);

    /// 最後に通りすぎた交差点を設定
    /**
     * 最後に通りすぎた交差点をprevの次に來るpassedに設定。
     * 指定された交差点がなければ何もしない。
     * (passeddを通り過ぎるときに呼ばれる。)
     */
    void setLastPassedIntersection(Intersection* prev, Intersection* passed);

    /// 最後に通り過ぎた交差点インデックスを取得する
    int lastPassedIntersectionIndex() const;

    /// 前の交差点を返す
    Intersection* prev(Intersection*) const;

    /// 次の交差点を返す
    /**
     * 次の交差点を返す
     * 同じ交差点を複数回通る場合、この関数では
     * 次の交差点は一意に決まらないので
     * できるだけ next(Intersection*, Intersection*)
     * を使うこと。
     */
    Intersection* next(Intersection*) const;

    /// 次の交差点を返す
    /**
     * バスルートなど、
     * 同じ交差点を２度通る可能性があるルートの場合、
     * next(Intersection*) では不足で
     * 直前の交差点の情報も与えないとルートが確定しない。
     * @note S.B. fixed this function. 2008/08/27
     */
    Intersection* next(Intersection*, Intersection*) const;

    /// 出発地を返す
    Intersection* start() const;

    /// 目的地を返す
    Intersection* goal()  const;

    /// 含まれる交差点を返す
    const std::vector<Intersection*>* route()  const;

    /// 含まれる交差点の個数を返す
    int size() const;

    /// @p i 番目の交差点を返す
    Intersection* inter(int) const;

    /// この経路のコストを返す
    double cost() const;

    /// この経路のコストを設定する
    void setCost(double);

    /// ルートを出力する
    void print(std::ostream& out) const;
  
protected:
    /// コスト。経路探索での価値判断に用いる
    double _cost;
    /// 経路本体
    std::vector<Intersection*> _route;
    /// 最後に通過した交差点のindex
    int _lastPassedIntersection;
};
#endif

