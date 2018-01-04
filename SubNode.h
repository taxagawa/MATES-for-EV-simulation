/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __SUBNODE_H__
#define __SUBNODE_H__

#include <vector>
#include <map>
#include <string>
#include <list>
#include "AmuVector.h"
#include "AmuPoint.h"
#include "LaneBundle.h"
#include "VehicleFamily.h"

#include <iostream>

class Intersection;
class Section;
class RoadMap;

using namespace std;

/// Section内部のスコアリング用ノードクラス

class SubNode
{
public:
    SubNode(AmuPoint ptSubNode);
    ~SubNode();

    // CS配置用のスコア
    double _score;

    // SubNodeの情報を出力する
    void print();

    AmuPoint point();

    void score(double s);
    double score();

    void nearestCS(RoadMap* _map);
    void setNearestCS(double distance);
    double distanceCS();

    double _distanceCS;

    void setIntersection(Intersection* intersection);
    void setSection(Section* section);

    // 自分の所属
    Intersection* _intersection;
    Section* _section;

    bool isIntersection();

private:
    // 点の実体
    AmuPoint _point;
};

#endif //__SUBNODE_H__
