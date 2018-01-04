/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "SubNode.h"
#include "RoadMap.h"
#include "Intersection.h"
#include "Lane.h"
#include "RoadEntity.h"
#include "RoadOccupant.h"
#include "Vehicle.h"
#include "GVManager.h"
#include "AmuLineSegment.h"
#include "Conf.h"
#include "CSNode.h"
#include <cassert>
#include <sstream>
#include <algorithm>
#include <vector>
#include <math.h>

using namespace std;

//======================================================================
SubNode::SubNode(AmuPoint ptSubNode)
{
    _score = 0;
    _point = ptSubNode;
}

//======================================================================
SubNode::~SubNode()
{

}

//======================================================================
void SubNode::setSection(Section* section)
{
    _section = section;
    _intersection = NULL;
}

//======================================================================
void SubNode::setIntersection(Intersection* intersection)
{
    _section = NULL;
    _intersection = intersection;
}

//======================================================================
bool SubNode::isIntersection()
{
    if (_intersection != NULL)
        return true;
    else
        return false;
}

//======================================================================
void SubNode::print()
{
    cout << "SN: " << _point.x() << " " << _point.y() << " " << _point.z() << " "
         << _score * _distanceCS / 10000 << " "
         << _score << " " << _score * _distanceCS << endl;
}

//======================================================================
AmuPoint SubNode::point()
{
    return _point;
}

//======================================================================
double SubNode::score()
{
    return _score;
}

//======================================================================
void SubNode::score(double s)
{
    _score += s;
}

//======================================================================
void SubNode::nearestCS(RoadMap* _map)
{
    vector<CSNode*> vec = _map->csNodes();
    double min =         100000000;
    double min_another = 100000000;
    bool isUp = true;
    int min_index = 0;

// by uchida 2017/3/14
// ここを利用し，途中でreturnしてしまえばユークリッド距離のコードに書き換えられる
    for (int i = 0; i < vec.size(); i++)
    {
          // ユークリッド距離をスコアに積算する場合
        double distance
            = vec[i]->center().distance(_point.x(), _point.y(), _point.z());
        if (min > distance)
        {
            min = distance;
            min_index = i;
        }
    }
    _distanceCS = min;
    // ここでreturnするとユークリッド距離格納
    // return;

// by uchida 2017/10/12
// なぜ1にしたのか意図不明
// _distanceCS = 1;
// return;

    ARouter* router = new Router();

    vector<double> tmp;
    tmp.push_back(1);
    for (int i = 0; i < 5; i++)
    {
        tmp.push_back(0);
    }
    const vector<double> p = tmp;

    router->setParam(p);

    // 最短経路長をスコアに積算する場合
    // intersectionであれば1箇所から計算
    if (_intersection != NULL)
    {
        for (int i = 0; i < vec.size(); i++)
        {
            // commented by uchida 2017/2/26
            // 最後のCS指定は不要だとは思うが念のため…
            double GV =
                router->searchSegmentGV(_intersection, vec[i], NULL, 10000);

            if (GV < min)
            {
                min = GV;
                min_index = i;
            }
        }
        _distanceCS = min;
    }
    // sectionの場合は両端2箇所のintersectionから計算
    else if (_section != NULL)
    {
        for (int i = 0; i < vec.size(); i++)
        {
            double GV_0 =
                router->searchSegmentGV(_section->intersection(false), vec[i], NULL, 10000);
            double GV_1 =
                router->searchSegmentGV(_section->intersection(true), vec[i], NULL, 10000);

            if (std::min(GV_0, GV_1) < min)
            {
                min = std::min(GV_0, GV_1);
                min_another = std::max(GV_0, GV_1);
                min_index = i;
                if (GV_0 < GV_1)
                {
                    isUp = false;
                }
                else
                {
                    isUp = true;
                }
            }
        }
        // 遠いほうのintersectionからの最短経路はこの_sectionを通ることを前提とした実装
        // 本来は、お互いの最短経路が重なり合わない可能性があり、その場合はより大きなコストとなるはず
        _distanceCS =
            (min_another - min) * (_point.distance(_section->intersection(isUp)->center()) / _section->length()) + min;
    }

}

//======================================================================
void SubNode::setNearestCS(double distance)
{
    _distanceCS = distance;
}

//======================================================================
double SubNode::distanceCS()
{
    return _distanceCS;
}
