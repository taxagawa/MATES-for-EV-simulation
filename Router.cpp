/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include <fstream>
#include <iostream>
#include "Router.h"
#include "AmuVector.h"
#include "NodeAStar.h"
#include "Route.h"
#include "RoadMap.h"
#include "Intersection.h"
#include "Section.h"
#include "OD.h"
#include "TimeManager.h"
#include "Tree.h"
#include "AmuVector.h"
#include "Random.h"
#include "Vehicle.h"
#include <cassert>
#include <cstdlib>

// #define DEBUG_ROUTER

using namespace std;

double Router::_vel = 120.0/60.0/60.0;

//======================================================================
Router::Router(Intersection* start, Intersection* goal, RoadMap* roadMap)
    :ARouter(start,goal,roadMap)
{
    _start = start;
    _goal = goal;
    _map = roadMap;
    _tree = new Tree();
    _counter = 0;
}

//======================================================================
Router::~Router()
{
    if(_tree != NULL)
    {
        delete _tree;
    }
}

//======================================================================
void Router::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;
}

//======================================================================
void Router::setTrip(OD* od, RoadMap* roadMap)
{
    _start = roadMap->intersection(od->start());
    _goal  = roadMap->intersection(od->goal());
    _map   = roadMap;
    _od    = *od;
}

//======================================================================
void Router::setParam(const vector<double>& rp)
{
    // 重みベクトルaの初期化
    _a.resize(6);
    for(int i = 0; i < static_cast<signed int>(_a.size());++i)
    {
        _a[i] = 0.0;
    }

    // 重みベクトルaの設定
    for(int i = 0; i < static_cast<signed int>(rp.size()) &&
            static_cast<signed int>(_a.size()) ; ++i)
    {
        _a[i] = rp[i];
    }

}

//======================================================================
void Router::initNode(const Intersection* start,
		      const Intersection* goal)
{
    if(_tree != NULL)
    {
        delete _tree;
    }
    _tree = new Tree();

    if(_a.size() != 0)
    {
        NodeAStar* startNode =
            new NodeAStar(const_cast<Intersection*>(start), NULL);
        // g = 0である．hは算出する．
        startNode->setGV(0.0);
        startNode->setHV(funcH(startNode, goal));
        _tree->push(startNode);

        _counter = 0;
    }
    else
    {
        cerr << " You have to call setParam() before calling initNode()."
             << endl;
        assert(0);
    }
}
//======================================================================
void Router::initNode(const Section* section,
		      const Intersection* start,
		      const Intersection* goal)
{
    if(_tree != NULL) delete _tree;
    _tree = new Tree();

    if(_a.size() != 0)
    {
        //スタートノードの作成および設定
        NodeAStar* startNode =
            new NodeAStar(const_cast<Intersection*>(start),NULL);
        // g = 0である．hは算出する．
        startNode->setGV(0.0);
        startNode->setHV(funcH(startNode,goal));
        _tree->push(startNode);

        _counter = 0;

        // sectionの端点である2つの交差点のうち、
        // startでない方を_treeに加える。
        Intersection* next;
        if(section->intersection(true) == start)
        {
            next = const_cast<Intersection*>(section->intersection(false));
        }
        else if(section->intersection(false) == start)
        {
            next = const_cast<Intersection*>(section->intersection(true));
        }
        else
        {
            cerr << " Wrong stree was given to Router::initNode(Street*)"
                 << endl;
            exit(1);
        }
        NodeAStar* nextNode =
            new NodeAStar(const_cast<Intersection*>(next),startNode);
        nextNode->setGV(funcG(nextNode));
        nextNode->setHV(funcH(nextNode,goal));
        _tree->push(nextNode);
        startNode->disable();

        ++_counter;
    }
    else
    {
        cerr << " You have to call setParam() before calling initNode() ."
             << endl;
        assert(0);
    }
}

//======================================================================
void Router::searchSegment(const Intersection* start,
               const Intersection* goal,
               const Intersection* past,
               const int step,
               vector<Route*>& result_routes)
{
    if (past==NULL)
    {
        initNode(start, goal);
    }
    else
    {
        initNode(past->nextSection(start), past, goal);
    }

#ifdef DEBUG_ROUTER
    ofstream rout("router_result.txt",ios::out);
    if(!rout)
    {
        cerr << "connot open file router_result.txt" << endl;
        assert(0);
    }
#endif

    bool canSolve = true;
    while (canSolve && !isEnd(goal) && step > _counter)
    {
#ifdef DEBUG_ROUTER
        cout << "_counter:" << _counter;
        cout << "     size of tree:" << _tree->size() << endl;

        // 展開状況をファイルに書き出す
        _tree->print(rout,_counter);
#endif
        // 末端のノードで、評価値が最小のものを取り出す。
        NodeAStar* minNode  = _tree->leafAble();
        if (minNode == NULL)
        {
            canSolve = false;
        }

        // ノードの展開、ツリーへの格納

        // 展開する
        vector<NodeAStar*> eNodes;
        while (canSolve
               && !isEnd(goal)
               && !expand(minNode, &eNodes, goal))
        {
            minNode->disable();
            minNode = _tree->leafAble();

            if(minNode == NULL)
            {
                canSolve = false;
            }
        }

        bool doSucceed = false;
        if(canSolve && !isEnd(goal))
        {
            // 展開されたものを_treeに加える。
            for(int i=0; i < static_cast<signed int>(eNodes.size()); ++i)
            {
                if(_tree->push(eNodes[i]))
                {
                    doSucceed = true;
                }
            }
        }

        if(canSolve && !isEnd(goal))
        {
            if(!doSucceed)
            {
                minNode->disable();
            }
        }

        _counter++;
    }

    if (canSolve && isEnd(goal))
    {
#ifdef DEBUG_ROUTER
        cout << "Yahoo!" << endl;
#endif
        //最適経路をセットする。
        /*
         * 最適経路をたどったゴールを取り出す。最適経路が見つかった時,
         * 探索木の葉の内,最小の評価値を持つものはゴールになっている。
         */
        NodeAStar* solNode = _tree->leafAble();

#ifdef DEBUG_ROUTER
        if(true)
        {
            NodeAStar* node = _tree->leafAble();
            assert(node->top()->id().compare(goal->id()) == 0);
        }
#endif
        result_routes.push_back(new Route());
        vector<Route*>::iterator it = result_routes.end();
        it--;
        (*it)->setCost(solNode->gv());

        vector<Intersection*> tmproute;

        if (past==NULL)
        {
            while(solNode != NULL)
            {
                tmproute.push_back(solNode->top());
                solNode = solNode -> parent();
            }
        }
        else
        {
            // pastを指定した場合には実際より一つ前の交差点を
            // スタート地点としているため，最初の一つは格納しない
            while(solNode->parent()!=NULL)
            {
                tmproute.push_back(solNode->top());
                solNode = solNode -> parent();
            }
        }

        for(int i = tmproute.size() -1 ;i >= 0;i--)
        {
            (*it)->push(tmproute[i]);
        }
#ifdef DEBUG_ROUTER
        cout << "routes.size():" << result_routes.size() << endl;
        assert(result_routes.size() == 1);
        assert(result_routes[0]->size() != 0);
        for(int i = 0; i < result_routes[0]->size();++i)
        {
            cout << result_routes[0]->inter(i) << " , ";
        }
        cout << endl;
#endif
    }
    else
    {
        cout << "Router cannot find route."<< endl
             << "  Origin: " << _start->id()
             << ", Destination: " << _goal->id() << endl
             << "     dump data:"<<endl;
        _tree->print(cout);
    }
}

//======================================================================
double Router::searchSegmentGV(const Intersection* start,
                               const Intersection* goal,
                               const Intersection* past,
                               const int step,
                               std::string stopCS)
{
    _stopCS = stopCS;

    if (past==NULL)
    {
        initNode(start, goal);
    }
    else
    {
        initNode(past->nextSection(start), past, goal);
    }

#ifdef DEBUG_ROUTER
    ofstream rout("router_result.txt",ios::out);
    if(!rout)
    {
        cerr << "connot open file router_result.txt" << endl;
        assert(0);
    }
#endif

    double GV;
    bool canSolve = true;
    while (canSolve && !isEnd(goal) && step > _counter)
    {
#ifdef DEBUG_ROUTER
        cout << "_counter:" << _counter;
        cout << "     size of tree:" << _tree->size() << endl;

        // 展開状況をファイルに書き出す
        _tree->print(rout,_counter);
#endif
        // 末端のノードで、評価値が最小のものを取り出す。
        NodeAStar* minNode  = _tree->leafAble();
        if (minNode == NULL)
        {
            canSolve = false;
        }

        // ノードの展開、ツリーへの格納

        // 展開する
        vector<NodeAStar*> eNodes;
        while (canSolve
               && !isEnd(goal)
               && !expand(minNode, &eNodes, goal))
        {
            minNode->disable();
            minNode = _tree->leafAble();

            if(minNode == NULL)
            {
                canSolve = false;
            }
        }

        bool doSucceed = false;
        if(canSolve && !isEnd(goal))
        {
            // 展開されたものを_treeに加える。
            for(int i=0; i < static_cast<signed int>(eNodes.size()); ++i)
            {
                if(_tree->push(eNodes[i]))
                {
                    doSucceed = true;
                }
            }
        }

        if(canSolve && !isEnd(goal))
        {
            if(!doSucceed)
            {
                minNode->disable();
            }
        }

        if (isEnd(goal))
        {
            GV = minNode->gv();
            return GV;
        }
        _counter++;
    }

}

// by uchida 2017/2/26
// Subnode.cppのnearestCS用経路探索
//======================================================================
double Router::searchSegmentGV(const Intersection* start,
                               const Intersection* goal,
                               const Intersection* past,
                               const int step)
{
    if (past==NULL)
    {
        initNode(start, goal);
    }
    else
    {
        initNode(past->nextSection(start), past, goal);
    }

#ifdef DEBUG_ROUTER
    ofstream rout("router_result.txt",ios::out);
    if(!rout)
    {
        cerr << "connot open file router_result.txt" << endl;
        assert(0);
    }
#endif

    double GV;
    bool canSolve = true;
    while (canSolve && !isEnd(goal) && step > _counter)
    {
#ifdef DEBUG_ROUTER
        cout << "_counter:" << _counter;
        cout << "     size of tree:" << _tree->size() << endl;

        // 展開状況をファイルに書き出す
        _tree->print(rout,_counter);
#endif
        // 末端のノードで、評価値が最小のものを取り出す。
        NodeAStar* minNode  = _tree->leafAble();
        if (minNode == NULL)
        {
            canSolve = false;
        }

        // ノードの展開、ツリーへの格納

        // 展開する
        vector<NodeAStar*> eNodes;
        while (canSolve
               && !isEnd(goal)
               && !expand_noV(minNode, &eNodes, goal))
        {
            minNode->disable();
            minNode = _tree->leafAble();

            if(minNode == NULL)
            {
                canSolve = false;
            }
        }

        bool doSucceed = false;
        if(canSolve && !isEnd(goal))
        {
            // 展開されたものを_treeに加える。
            for(int i=0; i < static_cast<signed int>(eNodes.size()); ++i)
            {
                if(_tree->push(eNodes[i]))
                {
                    doSucceed = true;
                }
            }
        }

        if(canSolve && !isEnd(goal))
        {
            if(!doSucceed)
            {
                minNode->disable();
            }
        }

        if (isEnd(goal))
        {
            GV = minNode->gv();
            return GV;
        }
        _counter++;
    }

}

//======================================================================
bool Router::search(const Intersection* start,
		    int step,
		    Route*& result_route)
{
    Intersection* past = NULL;
    Intersection* goal;
    vector<Route*> routes;
    assert(start);

    result_route = new Route();

    const vector<string>* stopPoints   = _od.stopPoints();
    vector<string>::const_iterator isp = stopPoints->begin();

    // 経由地を一つでも過ぎていれば
    // それ以降の経由地を目的地として探索する。
    if(_od.lastPassedStopPoint() >= 0)
    {
        isp  = _od.stopPoints()->begin() + _od.lastPassedStopPoint()+1;
    }

    while(isp != stopPoints->end())
    {
        goal = _map->intersection(*isp);
        searchSegment(start, goal, past, step, routes);
        if(routes.size() == 0)
        {
            // 探索失敗
            result_route = NULL;
            return false;
        }
        vector<Intersection*>* newRoute =
            const_cast< vector<Intersection*>* >(routes[0]->route());
        result_route->push(*newRoute);

        for(unsigned int i = 0; i < routes.size(); i++)
        {
            delete routes[i];
        }
        routes.clear();

        start = goal;
        if (result_route->size()>=2)
            past = result_route->inter(result_route->size()-2);
        isp++;
    }

    goal = _map->intersection(_od.goal());
    assert(goal == _goal);

    searchSegment(start, goal, past, step, routes);
    if(routes.size() == 0)
    {
        //探索失敗
        result_route = NULL;
        return false;
    }
    vector<Intersection*>* newRoute =
        const_cast< vector<Intersection*>* >(routes[0]->route());
    result_route->push(*newRoute);
    for(unsigned int i = 0; i < routes.size(); i++)
    {
        delete routes[i];
    }
    routes.clear();

    return true;
}

//======================================================================
bool Router::search(const Section* section,
		    const Intersection* start,
		    int step,
		    Route*& result_route)
{
    assert(section);
    assert(start);
    // 「startの次の交差点」(startとsectionから分かる)を始点として経路探索する
    // 結果をコピーする前に最初のstartをresult_routeに格納しておく
    Intersection* past = const_cast<Intersection*>(start);
    Intersection* goal;
    start = start->next(start->direction(section));
    vector<Route*> routes;

    result_route = new Route();
    result_route->push(past);
    result_route->setLastPassedIntersection(past);

    // もしstartがODNodeであれば，それ以上の経路を探索できない
    if (start->numNext()==1 && start!=_goal)
    {
        result_route = NULL;
        return false;
    }

    const vector<string>* stopPoints   = _od.stopPoints();
    vector<string>::const_iterator isp = stopPoints->begin();

    // 経由地を一つでも過ぎていれば、
    // それ以降の経由地を目的地として探索する。
    if(_od.lastPassedStopPoint() >= 0)
    {
        isp  = _od.stopPoints()->begin() + _od.lastPassedStopPoint() + 1;
    }

    while(isp != stopPoints->end())
    {
        goal = _map->intersection(*isp);
        searchSegment(start, goal, past, step, routes);

        if(routes.size() == 0)
        {
            // 探索失敗
            result_route = NULL;
            return false;
        }
        vector<Intersection*>* newRoute =
            const_cast< vector<Intersection*>* >(routes[0]->route());
        result_route->push(*newRoute);

        for(unsigned int i = 0; i < routes.size(); i++)
        {
            delete routes[i];
        }
        routes.clear();

        start = goal;
        if (result_route->size()>=2)
            past = result_route->inter(result_route->size()-2);
        isp++;
    }

    goal = _map->intersection(_od.goal());
    assert(goal == _goal);

    searchSegment(start, goal, past, step, routes);
    if(routes.size() == 0)
    {
        //探索失敗
        result_route = NULL;
        return false;
    }
    vector<Intersection*>* newRoute =
        const_cast< vector<Intersection*>* >(routes[0]->route());
    result_route->push(*newRoute);
    for(unsigned int i = 0; i < routes.size(); i++)
    {
        delete routes[i];
    }
    routes.clear();

    return true;
}

// by uchida 2016/5/12
// CS用に目的CSを指定した経路探索関数
//======================================================================
bool Router::CSsearch(const Section* section,
            const Intersection* start,
            int step,
            Route*& result_route,
            string stopCS)
{
    assert(section);
    assert(start);
    // 「startの次の交差点」(startとsectionから分かる)を始点として経路探索する
    // 結果をコピーする前に最初のstartをresult_routeに格納しておく
    Intersection* past = const_cast<Intersection*>(start);
    Intersection* goal;
    start = start->next(start->direction(section));

    vector<Route*> routes;

    result_route = new Route();
    result_route->push(past);
    result_route->setLastPassedIntersection(past);

    // もしstartがODNodeであれば，それ以上の経路を探索できない
    if (start->numNext()==1 && start!=_goal)
    {
        result_route = NULL;
        return false;
    }

    _stopCS = stopCS;
    // もしstartがCSNodeであれば,最終目的地を検索する
    if (start->id() == stopCS)
    {

        const vector<string>* stopPoints   = _od.stopPoints();
        vector<string>::const_iterator isp = stopPoints->begin();
        // by uchida 2016/5/20
        // pastをNULLにしてsearchSegment/initNodeすることで
        // 流入方向を考慮せずにCSを出入りできるはず
        past = NULL;

        // 経由地を一つでも過ぎていれば、
        // それ以降の経由地を目的地として探索する。
        if(_od.lastPassedStopPoint() >= 0)
        {
            isp  = _od.stopPoints()->begin() + _od.lastPassedStopPoint() + 1;
        }

        while(isp != stopPoints->end())
        {
            goal = _map->intersection(*isp);
            searchSegment(start, goal, past, step, routes);

            if(routes.size() == 0)
            {
                // 探索失敗
                result_route = NULL;
                return false;
            }
            vector<Intersection*>* newRoute =
                const_cast< vector<Intersection*>* >(routes[0]->route());
            result_route->push(*newRoute);

            for(unsigned int i = 0; i < routes.size(); i++)
            {
                delete routes[i];
            }
            routes.clear();

            start = goal;
            if (result_route->size()>=2)
                past = result_route->inter(result_route->size()-2);
            isp++;
        }

        goal = _map->intersection(_od.goal());
        assert(goal == _goal);

        searchSegment(start, goal, past, step, routes);
        if(routes.size() == 0)
        {
            //探索失敗
            result_route = NULL;
            return false;
        }
        vector<Intersection*>* newRoute =
            const_cast< vector<Intersection*>* >(routes[0]->route());
        result_route->push(*newRoute);
        for(unsigned int i = 0; i < routes.size(); i++)
        {
            delete routes[i];
        }
        routes.clear();

        return true;

    }
    // それ以外はCSを検索
    else
    {

        // 目的CSを割り込ませて最優先で探索
        // by uchida 2016/5/12
        goal = _map->intersection(_stopCS);
        searchSegment(start, goal, past, step, routes);

        if(routes.size() == 0)
        {
            //探索失敗
            result_route = NULL;
            cout << "CS cannot find" << endl;
            return false;
        }

        vector<Intersection*>* newCSRoute =
            const_cast< vector<Intersection*>* >(routes[0]->route());
        result_route->push(*newCSRoute);
        for(unsigned int i = 0; i < routes.size(); i++)
        {
            delete routes[i];
        }
        routes.clear();

        return true;

    }

}

//======================================================================
bool Router::expand(NodeAStar* node, vector<NodeAStar*>* expandedNodes,
		    const Intersection* goal)
{
    bool flag = false;
    Intersection* me = node->top();

    for(int i = 0; i < me->numNext() ; i++)
    {
        // 祖先のノードの側には展開させない．
        // また，到達不能なノードにも展開させない．
        bool reachableFlag = false;
        if( node->parent() == NULL)
        {
            reachableFlag = true;
        }
        else
        {
            // 到達不能なノード
            /*
             * @note
             * 経路選択時に交差点内のレーン接続まで考慮できるかどうかは
             * 本来はドライバーに依存する．
             */
            if (me->isReachable(node->bottom(), me->next(i)) == false)
            {
                reachableFlag = false;
            }
            // 接続先が通行禁止の単路
            else if (me->nextSection(i)
                     ->mayPassVehicleType(me, _vehicle->type()) == false)
            {
                reachableFlag = false;
            }
            // 祖先のノード
            else
            {
                NodeAStar* ac = node->parent();
                reachableFlag = true;
                while (ac->parent()!=NULL && reachableFlag)
                {
                    if (me==ac->bottom()
                        && me->next(i)==ac->top())
                    {
                        reachableFlag = false;
                    }
                    ac = ac->parent();
                }
            }
        }

        if (reachableFlag)
        {
            // NodeAStarのメインコンテナは_treeが持つのでdeleteはRouterの
            // 関数からは作用させないことに注意。
            NodeAStar* tmpNode =
                new NodeAStar(const_cast<Intersection*>(me->next(i)),node);
            // 選択率の評価
            if (Random::uniform(_vehicle->randomGenerator())
                < me->nextSection(i)->routingProbability(me, _vehicle->type(), _stopCS))
            {
                // 展開可能
                tmpNode->setGV(funcG(tmpNode));
                tmpNode->setHV(funcH(tmpNode, goal));
            }
            else
            {
                // 展開不可能
                tmpNode->disable();
                tmpNode->setGV(INFINITY);
                tmpNode->setHV(INFINITY);
            }
            expandedNodes->push_back(tmpNode);
            flag = true;
        }
    }
    return flag;
}

// by uchida 2017/2/26
// 車両から呼び出さなくてもよいようにした関数
//======================================================================
bool Router::expand_noV(NodeAStar* node, vector<NodeAStar*>* expandedNodes,
		    const Intersection* goal)
{
    bool flag = false;
    Intersection* me = node->top();

    for(int i = 0; i < me->numNext() ; i++)
    {
        // 祖先のノードの側には展開させない．
        // また，到達不能なノードにも展開させない．
        bool reachableFlag = false;
        if( node->parent() == NULL)
        {
            reachableFlag = true;
        }
        else
        {
            // 到達不能なノード
            /*
             * @note
             * 経路選択時に交差点内のレーン接続まで考慮できるかどうかは
             * 本来はドライバーに依存する．
             */
            if (me->isReachable(node->bottom(), me->next(i)) == false)
            {
                reachableFlag = false;
            }
            // 祖先のノード
            else
            {
                NodeAStar* ac = node->parent();
                reachableFlag = true;
                while (ac->parent()!=NULL && reachableFlag)
                {
                    if (me==ac->bottom()
                        && me->next(i)==ac->top())
                    {
                        reachableFlag = false;
                    }
                    ac = ac->parent();
                }
            }
        }

        if (reachableFlag)
        {
            // NodeAStarのメインコンテナは_treeが持つのでdeleteはRouterの
            // 関数からは作用させないことに注意。
            NodeAStar* tmpNode =
                new NodeAStar(const_cast<Intersection*>(me->next(i)),node);

            // 展開可能
            tmpNode->setGV(funcG(tmpNode));
            tmpNode->setHV(funcH(tmpNode, goal));

            expandedNodes->push_back(tmpNode);
            flag = true;
        }
    }
    return flag;
}

//======================================================================
bool Router::isEnd(const Intersection* goal)
{
    bool flag = false;
    /*
     * ゴール候補。探索木の葉の中で評価値が最小のものがゴールノードの時
     * 探索は終了
     */
    if (isGoal(_tree->leafAble(), goal))
    {
        flag = true;
    }
    return flag;
}

//======================================================================
bool Router::isGoal(NodeAStar* node, const Intersection* goal)
{
    bool flag = false;

    //ゴールのノード番号と比較すればよい
    if (node == NULL)
    {
        return flag;
    }

    Intersection* me = node->top();
    if(me->id().compare(goal->id()) == 0)
    {
        flag = true;
    }

    return flag;
}

//======================================================================
double Router::funcG(NodeAStar* node)
{
    double gValue = 0.0;

    if(node->parent() == NULL)
    {
        gValue = 0.0;
    }
    else if(node->parent()->parent() == NULL)
    {
        // 一つ前のノード
        NodeAStar* na1 = node->parent();
        // 現在のノードとna1を結ぶリンク
        Section* link1 = na1->top()->nextSection(node->top());

        double x[6];
        for(int i = 0;i < 6;i++)
        {
            x[i] = 0;
        }

        /*
         * cost = _a[i]*x[i]を一つ前のノードna1のgv()に加算したものが
         * ノードnodeの_gvとなる。
         */
        //x[0]の算出
        AmuVector tmpVec(na1->top()->center(), node->top()->center());
        x[0] = tmpVec.size();
        //x[1]の算出
        x[1] = na1->top()->averagePassTimeForGlobal
            (na1->top()->direction(node->top()));
        if (x[1]<1.0e-3)
        {
            x[1] = link1->length()/_vel;
        }
        //x[2]の導出
        x[2] = 0.0;
        //x[3]の導出
        x[3] = 0.0;
        //x[4]の導出
        x[4] = 0.0;
        //x[5]の導出
        x[5] = x[0] / link1->width();

        //gの値の算出
        for(int i = 0;i < 6;i++)
        {
            gValue += _a[i]*x[i];
        }
        //親のノードのg値との和になる。
        gValue += node->parent()->gv();
    }
    else
    {
        // 一つ前のノード
        NodeAStar* na1 = node->parent();
        /*
         * 二つ前のノード。
         * 何故かというと1つ前のノードna1から現在のノードnodeに
         * 到達するにはna1で直進,左折,右折のいずれをするのかは
         * node,na1,更に一つ前のノードna2の関係からでしか判断できない。
         */
        NodeAStar* na2 = na1->parent();
        // 現在のノードとna1を結ぶリンク
        Section* link1 = na1->top()->nextSection(node->top());

        double x[6];
        for(int i = 0;i < 6;i++)
        {
            x[i] = 0;
        }

        /*
         * cost = _a[i]*x[i]を一つ前のノードna1のgv()に加算したものが
         * ノードnodeの_gvとなる．
         */
        //x[0]の算出
        AmuVector tmpVec(na1->top()->center(), node->top()->center());
        x[0] = tmpVec.size();
        //x[1]の算出
        x[1] = na1->top()->averagePassTimeForGlobal
            (na1->top()->direction(node->top()));
        if (x[1]<1.0e-3)
        {
            x[1] = link1->length()/_vel;
        }
        /*
        cout << "from:" << na1->bottom()->id()
             << ", via:" << node->bottom()->id()
             << ", to:" << node->top()->id()
             << ", cost:" << x[1] << endl;
        */
        if(na1->top()->isStraight(na2->top(),node->top()))
        {
            //x[2]の導出
            x[2] = 10;
        }
        else if(na1->top()->isLeft(na2->top(),node->top()))
        {
            //x[3]の導出
            x[3] = 10;
        }
        else if(na1->top()->isRight(na2->top(),node->top()))
        {
            //x[4]の導出
            x[4] = 10;
        }
        //x[5]の導出
        x[5] = x[0] / link1->width();

        //gの値の算出
        for(int i = 0;i < 6;i++)
        {
            gValue += _a[i]*x[i];
        }
        //親のノードのg値との和になる。
        gValue += node->parent()->gv();
    }

    return gValue;
}

//======================================================================
double Router::funcH(NodeAStar* node, const Intersection* goal)
{
    double com[2];  //nodeの座標
    double gcom[2]; //goalの座標

    com[0] = node->top()->center().x();
    com[1] = node->top()->center().y();

    gcom[0] = goal->center().x();
    gcom[1] = goal->center().y();

    double len = sqrt((com[0] - gcom[0]) * (com[0] - gcom[0])
                      + (com[1] - gcom[1]) * (com[1] - gcom[1]));
    double h_value = _a[0] * len + _a[1] * len / _vel;

    return h_value;
}


//======================================================================
void Router::setLastPassedStopPoint(const string passedInter)
{
    _od.setLastPassedStopPoint(passedInter);
}

//======================================================================
double Router::funcF(NodeAStar* node, const Intersection* goal)
{
    return funcH(node, goal) + funcG(node);
}


//======================================================================
int Router::counter() const
{
    return _counter;
}

//======================================================================
void Router::printParam() const
{
    string names[6] = {"distance",
                       "time",
                       "straight",
                       "left",
                       "right",
                       "width"};

    cout << "Router parameter: ";

    for (int i=0; i<static_cast<signed int>(_a.size()); i++)
    {
        cout << _a[i] << "/" << names[i] << ", ";
    }
    cout << endl;
}

//======================================================================
const Intersection* Router::start() const
{
    return _start;
}

//======================================================================
const Intersection* Router::goal() const
{
    return _goal;
}

//======================================================================
const std::vector<double> Router::param() const
{
    return _a;
}

//======================================================================
OD* Router::od()
{
    return &_od;
}
