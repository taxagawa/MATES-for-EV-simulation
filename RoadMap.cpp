/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <typeinfo>
#include "RoadMap.h"
#include "AmuPoint.h"
#include "AmuConverter.h"
#include "LaneBundle.h"
#include "Intersection.h"
#include "ODNode.h"
#include "CSNode.h"
#include "RoadEntity.h"
#include "Section.h"
#include "Signal.h"
#include "GVManager.h"
#include "Vehicle.h"
#include "GenerateVehicleIO.h"
#include "VehicleIO.h"
#include "ObjManager.h"
#include "Lane.h"

using namespace std;

//======================================================================
RoadMap::RoadMap(){}

//======================================================================
RoadMap::~RoadMap()
{
    //Intersectionをdelete&eraseする
    ITRMAPI iti = _intersections.begin();
    while (iti!= _intersections.end())
    {
        delete (*iti).second;
        iti++;
    }
    _intersections.erase(_intersections.begin(),
                         _intersections.begin());

    //Sectionをdelete&eraseする
    ITRMAPS its = _sections.begin();
    while (its!=_sections.end())
    {
        delete (*its).second;
        its++;
    }
    _sections.erase(_sections.begin(), _sections.begin());

    //Signalをdelete&eraseする
    ITRMAPSI itsi = _signals.begin();
    while (itsi!=_signals.end())
    {
        delete (*itsi).second;
        itsi++;
    }
    _signals.erase(_signals.begin(), _signals.end());
}

//======================================================================
const RMAPI* RoadMap::intersections() const
{
    return &_intersections;
}

//======================================================================
vector<ODNode*> RoadMap::odNodes()
{
    vector<ODNode*> vec;
    ITRMAPI iti = _intersections.begin();
    while (iti!=_intersections.end())
    {
        // (*iti).secondの型がODNodeであるかどうかチェックする
        // by uchida ここもいじっていたのか忘れていた
        if (dynamic_cast<ODNode*>((*iti).second) && !dynamic_cast<CSNode*>((*iti).second))
        {
            vec.push_back(dynamic_cast<ODNode*>((*iti).second));
        }
        iti++;
    }
    return vec;
}

// by uchida 2016/5/12
//======================================================================
vector<CSNode*> RoadMap::csNodes()
{
    vector<CSNode*> vec;
    ITRMAPI iti = _intersections.begin();
    while (iti!=_intersections.end())
    {
        // (*iti).secondの型がCSNodeであるかどうかチェックする
        if (dynamic_cast<CSNode*>((*iti).second))
        {
            vec.push_back(dynamic_cast<CSNode*>((*iti).second));
        }
        iti++;
    }
    return vec;
}

//======================================================================
Intersection* RoadMap::intersection(const string& id) const
{
    CITRMAPI iti = _intersections.find(id);
    if (iti != _intersections.end())
    {
        return (*iti).second;
    }
    else
    {
        return 0;
    }
}

//======================================================================
void RoadMap::addIntersection(Intersection* ptInter)
{
    //同一idがないか検査し、重複がなければ追加する
    string id = ptInter->id();
    ITRMAPI iti = _intersections.find(id);
    if (iti == _intersections.end())
    {
        _intersections[id] = ptInter;
        _bundles.push_back(ptInter);
    }
}

//======================================================================
bool RoadMap::checkIntersectionLane()
{
    bool check, result;

    cout << endl << "*** Check Intersection Lane ***" << endl;

    // 全交差点をチェック後、画面を見て修正、ODNode は除く
    result = true;
    ITRMAPI iti = _intersections.begin();
    while (iti!=_intersections.end())
    {
        if (!dynamic_cast<ODNode*>((*iti).second))
        {
            // 2016/10/27
            // by uchida
            // IntersectionのうちCSNode由来のものは除く
            // checkに引っかかりrunが止まるので
            if((*iti).second->numNext() != 1)
            {
                check = (*iti).second->checkLane();
                if (!check)
                {
                    cerr << "intersection " << (*iti).second->id()
                         << ": lane connection error." << endl;
                    result = false;
                }
            }
        }
        iti++;
    }
    cout << endl;
    return result;
}

//======================================================================
const RMAPS* RoadMap::sections() const
{
    return &_sections;
}

//======================================================================
Section* RoadMap::section(const string& id) const
{
    CITRMAPS its = _sections.find(id);
    if (its != _sections.end())
    {
        return (*its).second;
    }
    else
    {
        return 0;
    }
}

//======================================================================
void RoadMap::addSection(Section* ptSection)
{
    //同一idがないか検査し、重複があればコメントする
    string id = ptSection->id();
    ITRMAPS its= _sections.find(id);
    if (its != _sections.end())
    {
        cout << "Caution: section:" << id << " is duplicated." << endl;
    }
    _sections[id] = ptSection;
    _bundles.push_back(ptSection);
}

//======================================================================
void RoadMap::addSubNode(vector<SubNode*> SubNodes)
{
    _SubNodes.insert(_SubNodes.end(), SubNodes.begin(), SubNodes.end());
    // debug by uchida 2017/2/7
//    cout << "size of SNs : " << _SubNodes.size() << endl;
}


//======================================================================
vector<SubNode*> RoadMap::SubNodes()
{
    return _SubNodes;
}

//======================================================================
const vector<LaneBundle*>* RoadMap::laneBundles() const
{
    return &_bundles;
}

//======================================================================
void RoadMap::addUsedLaneBundle(LaneBundle* laneBundle)
{
#ifdef _OPENMP
#pragma omp critical (addUsedLaneBundle)
    {
#endif //_OPENMP

        _usedBundles.push_back(laneBundle);

#ifdef _OPENMP
    }
#endif //_OPENMP
}

//======================================================================
const RMAPSI* RoadMap::signals() const
{
    return &_signals;
}

//======================================================================
Signal* RoadMap::signal(const string& id) const
{
    CITRMAPSI itsi = _signals.find(id);
    if (itsi != _signals.end())
    {
        return (*itsi).second;
    }
    else
    {
        return 0;
    }
}

//======================================================================
void RoadMap::addSignal(Signal* ptSignal)
{
    //同一idがないか検査し、重複がなければ追加する
    string id = ptSignal->id();
    ITRMAPSI itsi = _signals.find(id);
    if (itsi == _signals.end())
    {
        _signals[id] = ptSignal;
    }
}

//======================================================================
void RoadMap::renewAgentLine()
{
    for (unsigned int i=0; i<_usedBundles.size(); i++)
    {
        _usedBundles[i]->renewAgentLine();
        if (typeid(*_usedBundles[i])==typeid(ODNode))
        {
            // ODNodeであればdeleteArrivedAgentを予約
            _usedODNodes.push_back(
                dynamic_cast<ODNode*>(_usedBundles[i]));
        }
    }
    _usedBundles.clear();
}

//======================================================================
void RoadMap::region(double& xmin, double& xmax,
		     double& ymin, double& ymax) const
{
    AmuPoint c;
    CITRMAPI iti = _intersections.begin();
    c = (*iti).second->center();
    xmin = c.x();
    xmax = c.x();
    ymin = c.y();
    ymax = c.y();

    while (iti != _intersections.end())
    {
        c = (*iti).second->center();
        if (c.x()<xmin) xmin = c.x();
        if (c.x()>xmax) xmax = c.x();
        if (c.y()<ymin) ymin = c.y();
        if (c.y()>ymax) ymax = c.y();
        iti++;
    }
}

//======================================================================
void RoadMap::deleteArrivedAgents()
{
    // ODノードのレーン上のエージェントを消去する
    for (unsigned int i=0; i<_usedODNodes.size(); i++)
    {
        _usedODNodes[i]->deleteAgent();
    }
    _usedODNodes.clear();
}

//======================================================================
void RoadMap::addStrandedVehicles(Vehicle* strandedVehicle)
{
    _strandedVehicles.push_back(strandedVehicle);
}

//======================================================================
void RoadMap::deleteStrandedAgents()
{
    // 充電切れのエージェントを消去する
    for (unsigned int i=0; i<_strandedVehicles.size(); i++)
    {
        if (GVManager::getFlag("FLAG_OUTPUT_TRIP_INFO"))
        {
            // 消去する前にトリップ長の出力
            VehicleIO::instance().writeVehicleDistanceData
                (dynamic_cast<Vehicle*>(_strandedVehicles[i]));
        }

//        _strandedVehicles[i]->lane()
//            ->outputAgents();

        cout << "strandedVehicle is deleted: (id)" << _strandedVehicles[i]->id()
             << " " << _strandedVehicles[i]->tripLength()

             << " " << _strandedVehicles[i]->route()->start()->id()
             << " " << _strandedVehicles[i]->route()->goal()->id()
             << " ";

        if (_strandedVehicles[i]->section() != NULL)
        {
            cout << _strandedVehicles[i]->section()->id();
        }
        else
        {
            cout << "*";
        }

        cout << " ";

        if (_strandedVehicles[i]->intersection() != NULL)
        {
            cout << _strandedVehicles[i]->intersection()->id();
        }
        else
        {
            cout << "*";
        }

        if (_strandedVehicles[i]->target() != NULL)
        {
            cout << " " << _strandedVehicles[i]->target()->id();
        }

        cout << endl;


        // Laneの_agentsからも削除しておく
        dynamic_cast<Vehicle*>(_strandedVehicles[i])->lane()
            ->eraseAgent(dynamic_cast<Vehicle*>(_strandedVehicles[i]));

        // ここでObjManagerから消去される
        ObjManager::deleteVehicle
            (dynamic_cast<Vehicle*>(_strandedVehicles[i]));
    }
    _strandedVehicles.clear();
}

//======================================================================
std::vector<std::string> RoadMap::ghostList()
{
    return _ghostList;
}

//======================================================================
void RoadMap::setGhostFlag(std::string id)
{
    _ghostList.push_back(id);
}

//======================================================================
void RoadMap::writeMapInfo() const
{
    string fNode, fLink;
    GVManager::getVariable("RESULT_NODE_SHAPE_FILE", &fNode);
    GVManager::getVariable("RESULT_LINK_SHAPE_FILE", &fLink);

    ofstream ofsNode(fNode.c_str(), ios::out);
    if (!ofsNode.fail())
    {
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // 交差点について
        ofsNode<< _intersections.size() << endl;
        CITRMAPI iti = _intersections.begin();
        while (iti!=_intersections.end())
        {
            // 交差点ID
            ofsNode << (*iti).second->id() << endl;

            // 中心点の座標
            ofsNode << (*iti).second->center().x() << ","
                    << (*iti).second->center().y() << ","
                    << (*iti).second->center().z() << endl;

            // 頂点の個数
            ofsNode << (*iti).second->numVertices() << endl;
            // 頂点の座標
            for (int i=0; i<(*iti).second->numVertices(); i++) {
                const AmuPoint tmpP = (*iti).second->vertex(i);
                ofsNode << tmpP.x() << ","
                        << tmpP.y() << ","
                        << tmpP.z() << endl;
            }
            iti++;
        }
        ofsNode.close();
    }

    ofstream ofsLink(fLink.c_str(), ios::out);
    if (!ofsLink.fail())
    {
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // 単路について
        ofsLink << _sections.size() << endl;
        CITRMAPS its = _sections.begin();
        while (its!=_sections.end())
        {
            // 単路ID
            ofsLink << (*its).second->id() << endl;

            // 右レーン数（終点ノードからの流入レーン数）
            // 交差点から見た流出レーン数
            Intersection* inter0 = (*its).second->intersection(false);
            const Border* border0
                = inter0->border(inter0->direction((*its).second));
            ofsLink << border0->numOut();
            ofsLink << ",";
            // 左レーン数（始点ノードからの流入レーン数）
            // 交差点から見た流出レーン数
            Intersection* inter1 = (*its).second->intersection(true);
            const Border* border1
                = inter1->border(inter1->direction((*its).second));
            ofsLink << border1->numOut();
            ofsLink << endl;

            // 始点ID, 終点ID
            ofsLink << inter0->id() << "," << inter1->id() << endl;

            // 始点，終点の信号の有無
            if (inter0->signal()) ofsLink << "1";
            else ofsLink << "0";
            ofsLink << ",";
            if (inter1->signal()) ofsLink << "1";
            else ofsLink << "0";
            ofsLink << endl;

            // 中心点(個数は2に固定)
            /* vertices(int)でも良いと思うが，順番が心配なので．*/
            ofsLink << "2" << endl;
            AmuPoint tmpP;
            tmpP = border0->lineSegment()->createInteriorPoint(1,1);
            ofsLink << tmpP.x() << ","
                    << tmpP.y() << ","
                    << tmpP.z() << endl;
            tmpP = border1->lineSegment()->createInteriorPoint(1,1);
            ofsLink << tmpP.x() << ","
                    << tmpP.y() << ","
                    << tmpP.z() << endl;

            // 右境界点(個数は2に固定)
            ofsLink << "2" << endl;
            tmpP = border0->lineSegment()->pointBegin();
            ofsLink << tmpP.x() << ","
                    << tmpP.y() << ","
                    << tmpP.z() << endl;
            tmpP = border1->lineSegment()->pointEnd();
            ofsLink << tmpP.x() << ","
                    << tmpP.y() << ","
                    << tmpP.z() << endl;

            // 左境界点(個数は2に固定)
            ofsLink << "2" << endl;
            tmpP = border0->lineSegment()->pointEnd();
            ofsLink << tmpP.x() << ","
                    << tmpP.y() << ","
                    << tmpP.z() << endl;
            tmpP = border1->lineSegment()->pointBegin();
            ofsLink << tmpP.x() << ","
                    << tmpP.y() << ","
                    << tmpP.z() << endl;

            its++;
        }
        ofsLink.close();
    }
}

//======================================================================
void RoadMap::dispIntersections() const
{
    CITRMAPI iti = _intersections.begin();
    cout << "*** RoadMap Information ***" << endl;
    while (iti!=_intersections.end())
    {
        (*iti).second->dispMapInfo();
        iti++;
    }
    /*
    CITRMAPS its = _sections.begin();
    while (its!=_sections.end())
    {
        (*its).second->print();
        its++;
    }
    */
    cout << endl;
}

// by uchida 2017/3/6
// スコアを時系列で出力する
//======================================================================
void RoadMap::writeScoreData(int timeStep)
{
    string outputDir, prefixD;
    GVManager::getVariable("RESULT_OUTPUT_DIRECTORY", &outputDir);
    GVManager::getVariable("RESULT_SCORE_PREFIX", &prefixD);

    if (!outputDir.empty()
        && GVManager::getFlag("FLAG_OUTPUT_SCORE"))
    {

        string fScore = outputDir + prefixD
            + ".csv." + AmuConverter::itos(timeStep/60000, 4);

        ofstream outScoreFile(fScore.c_str(), ios::app);

        if (!outScoreFile.fail())
        {
            outScoreFile << "x,y,z,score,score_d,z_dummy," << endl;

            for (int i = 0; i < _SubNodes.size(); i++)
            {
                // 出力を交差点のみに限定する場合
//                if (_SubNodes[i]->isIntersection())
//                {
                    outScoreFile << fixed << setprecision(5)
                    << _SubNodes[i]->point().x() << ","
                    << _SubNodes[i]->point().y() << ","
                    << _SubNodes[i]->point().z() << ","
                    << _SubNodes[i]->score() << ","
                    << _SubNodes[i]->score() * _SubNodes[i]->distanceCS() << ","
                    << _SubNodes[i]->score() * _SubNodes[i]->distanceCS() / 1000 << ","
                    << endl;
//                }
//                else
//                {
//                    outScoreFile
//                    << _SubNodes[i]->point().x() << ","
//                    << _SubNodes[i]->point().y() << ","
//                    << _SubNodes[i]->point().z() << ","
//                    << 0 << ","
//                    << 0 << ","
//                    << 0 << ","
//                    << endl;
//                }
            }
        }
        outScoreFile.close();
    }
}
