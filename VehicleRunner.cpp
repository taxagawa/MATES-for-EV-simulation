/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "Vehicle.h"
#include "Lane.h"
#include "Section.h"
#include "Intersection.h"
#include "GVManager.h"
#include "Random.h"
#include "AmuPoint.h"
#include "NCSNode.h"
#include <cassert>

using namespace std;

//======================================================================
void Vehicle::run()
{
    _isLanePassed  = false;
    _isNotifySet   = false;
    _isUnnotifySet = false;

    // スリープタイマのカウントダウン
    if (_sleepTime>0)
    {
        _sleepTime -= TimeManager::unit();
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // レーン方向の位置（レーン始点からの距離）の更新
    double length = _velocity * TimeManager::unit();
    _oldLength   =  _length;
    _length      += length;
    _totalLength += length;
    _tripLength  += length;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 車線変更の処理
    /**
     * レーン垂直方向の位置の更新を含む
     */
    if (_laneShifter.isActive())
    {
        _error += _errorVelocity * TimeManager::unit();

        _laneShifter.proceedShift();

        // 十分に横に移動したら車線変更処理を終了する
        if (abs(_error) >= _section->laneWidth())
        {
            _laneShifter.endShift();
        }
    }
    else if (_section && _laneShifter.canShift())
    {
        _laneShifter.beginShift();
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // レーンへの登録処理
    if (_length < _lane->length())
    {
        //--------------------------------------------------------------
        // レーンの終点に達していない場合
        // 同一レーンに登録する
        _lane->putAgent(this);
        _lane->setUsed();

        // 単路にいる場合
        // 次の交差点まで50m以内で，次に右左折するときはウィンカーを点ける
        if (_section!=NULL)
        {
            if (_section->lengthToNext(_lane, _length)<=50
                && (_blinker.turning() != _localRoute.turning()))
            {
                // 交差点拡張方向を導入した場合は要修正
                if (_localRoute.turning()==RD_LEFT)
                {
                    _blinker.setLeft();
                }
                else if (_localRoute.turning()==RD_RIGHT)
                {
                    _blinker.setRight();
                }
                else
                {
                    _blinker.setNone();
                }
            }
        }
    }
    else
    {
        //--------------------------------------------------------------
        // 次のレーンに移る
        _lane->setUsed();
        _nextLane->setUsed();

        // 次に所属するレーンを基準にするので，oldLengthはマイナス
        _oldLength -= _lane->length();
        _length    -= _lane->length();

        // _lane, _intersection, _sectionの更新
        if (_section==NULL)
        {
            // 交差点を走行中の場合
            assert(_intersection!=NULL);

            // 一旦停止フラグの解除
            _hasPaused = false;

            if (_intersection->isMyLane(_nextLane))
            {
                // 交差点内の次のレーンへ
                _runIntersection2Intersection();
            }
            else
            {
                // 交差点から単路へ
                _totalLength = 0;
                _runIntersection2Section();

                // ウィンカーを消す
                if (_blinker.isLeft() || _blinker.isRight())
                {
                    _blinker.setNone();
                }
            }
        }
        else
        {
            // by uchida 2016/5/19
            // 次の交差点
            Intersection* frontIntersection =
                _section->intersection(_section->isUp(_lane));

            // 単路を走行中の場合
            if (_section->isMyLane(_nextLane))
            {
                // 単路内の次のレーンへ
                _runSection2Section();

            }
            else if (frontIntersection->id() == _stopCS
                        && _route->goal()->id() != _stopCS)
            {
                // 単路からCSへ
                _runSection2CS();
            }
            else
            {
                // 単路から交差点へ
                _totalLength = 0;
                _runSection2Intersection();
            }
        }
        _lane->putAgent(this);
        _lane->setLastArrivalTime(TimeManager::time());
    }
}

//======================================================================
void Vehicle::_runIntersection2Intersection()
{
    _lane = _nextLane;
    _decideNextLane(_intersection, _lane);
}

//======================================================================
void Vehicle::_runIntersection2Section()
{
    _section = _intersection->nextSection(_lane);
    assert(_section);

    // by uchida 2017/2/27
    if (_section->intersection(true) != _intersection)
    {
        _realPassedRoute.push_back(_section->intersection(true));
    }
    else if (_section->intersection(false) != _intersection)
    {
        _realPassedRoute.push_back(_section->intersection(false));
    }

    // _intersectionに交差点通過時間を通知
    int from = _intersection->direction(_prevIntersection);
    int to   = _intersection->direction(_section);
    assert(0<=from && from<_intersection->numNext());
    assert(0<=to && to<_intersection->numNext());

    _intersection
        ->addPassTime(from, to, TimeManager::time()-_entryTime);
    _entryTime = TimeManager::time();

    // _watchedVehiclesの登録状況の受渡し
    /*
     * 本来は車線変更中にレーン束を移ることはないはずだが...
     */
    if (_isNotifying)
    {
        _intersection->eraseWatchedVehicle(this);
        _section->addWatchedVehicle(this);
    }

    // commented by uchida 2016/5/25
    // _typeはなくてもいいはずだよな
    if (_isCharge && _type >= 80)
    {

        Intersection* nextIntersection;
        if (_section->intersection(true) == _intersection)
        {
            nextIntersection = _section->intersection(false);
        }
        else if (_section->intersection(false) == _intersection)
        {
            nextIntersection = _section->intersection(true);
        }
        assert(nextIntersection);

        if (nextIntersection->id() == _stopCS)
        {
            // by uchida 2017/2/14
            //これまでの経路を取得する
            if (GVManager::getFlag("FLAG_OUTPUT_SCORE"))
            {
                getSubNodes_re();
            }

            // by uchida 2016/5/19
            // 次の交差点がCSならば先んじてCSから先の経路を再検索
            // CS新入直後、どの方向の出口から出てくるかを決めたいので
            CSreroute(_section, _intersection, _stopCS);
        }
        else if (_stopCS == "")
        {

            // by uchida     2016/5/24
            // 0.ランダムor一定
            // 1.ユークリッド距離で最近傍
            // 2.経路選択の最短コスト
            // 3.現在位置→CS→Dを最小化する
            // 2.3は発生時から計算している

            // by uchida 2016/5/19
            // ここでCS経由するCSを設定
            // 今はランダムとしているが、今後変更予定
            // 0
            //_searchCSRand();
            //CSreroute(_section, _intersection, _stopCS);

            // 1
            //_searchCSEuclid();
            //CSreroute(_section, _intersection, _stopCS);

            // 2
            //_searchCSCost();
            //CSreroute(_section, _intersection, _stopCS);

            // 2017/5/23
            // ここをonすることでconventional・offとすることでghost
            // 3
            // by takusagawa 2018/9/25
            // 待ち時間情報の受け取りができる場合はその情報を考慮したCS選択を行う
            // フラグが立っていた場合、待ち時間予測による情報を使用する
            if (!_isReceiveWaitingInfo)
            {
                _searchCSSumCost();
            }
            else
            {
                if (GVManager::getFlag("FLAG_USE_FUTURE_WAITING_LINE"))
                {
                    _searchCSFutureWaitingTimeSumCost();
                }
                else
                {
                    _searchCSWaitingTimeSumCost();
                }
            }
            CSreroute(_section, _intersection, _stopCS);

        }
    }

    // 車線変更に失敗したとき場合などのために
    // 必要であればここでルートを再探索する。
    if(_prevIntersection != NULL)
    {
        Intersection* next
            = _intersection
            ->next(_intersection->direction(_section));
        if (dynamic_cast<ODNode*>(next)==NULL
            && _route ->next(_intersection, next)==NULL)
        {
            if (_isCharge && _type >= 80)
            {

                // by uchida 2016/5/25
                // CS検索
                // 0
                //_searchCSRand();
                //CSreroute(_section, _intersection, _stopCS);

                // 1
                //_searchCSEuclid();
                //CSreroute(_section, _intersection, _stopCS);

                // 2
                //_searchCSCost();
                //CSreroute(_section, _intersection, _stopCS);

                // 3
                // by takusagawa 2018/9/25
                // 待ち時間情報の受け取りができる場合はその情報を考慮したCS選択を行う
                // フラグが立っていた場合、待ち時間予測による情報を使用する
                if (!_isReceiveWaitingInfo)
                {
                    _searchCSSumCost();
                }
                else
                {
                    if (GVManager::getFlag("FLAG_USE_FUTURE_WAITING_LINE"))
                    {
                        _searchCSFutureWaitingTimeSumCost();
                    }
                    else
                    {
                        _searchCSWaitingTimeSumCost();
                    }
                }
                CSreroute(_section, _intersection, _stopCS);

            }
            else
            {
                reroute(_section, _intersection);
            }

        }
    }

    _prevIntersection = _intersection;
    _intersection=NULL;
    _lane = _nextLane;
    _localRouter.clear();

    _localRouter.localReroute(_section, _lane, _length);
    _decideNextLane(_section, _lane);

}

//======================================================================
void Vehicle::_runSection2CS()
{
    _intersection = _section->nextIntersection(_lane);
    assert(_intersection);
    //最後に通過した交差点として新しい交差点を登録。
    if(_prevIntersection == NULL)
    {
        _route
            ->setLastPassedIntersection(_intersection);
    }
    else
    {
        _route
            ->setLastPassedIntersection(_prevIntersection,
                                        _intersection);
    }
    //最後に通過した経由地として新しい交差点を登録。
    _router
        ->setLastPassedStopPoint(_intersection->id());

    // by takusagawa 2018/10/29
    // 経路選択の際に待ち時間を適切に考慮するために追加
    int from = _intersection->direction(_prevIntersection);
    int to   = _intersection->direction(_section);
    assert(0<=from && from<_intersection->numNext());
    assert(0<=to && to<_intersection->numNext());

    _intersection
        ->addPassTime(from, to, TimeManager::time()-_entryTime);

    // by uchida 2016/5/20
    // コピペしたけど不要な気はする
    //_watchedVehiclesの登録状況の受渡し
    /*
     * 本来は車線変更中にレーン束を移ることはないはずだが...
     */
    if (_isNotifying)
    {
        _section->eraseWatchedVehicle(this);
        if (!(dynamic_cast<ODNode*>(_intersection)))
        {
            // ODノードに入る(次のステップで消える)ときにはaddしない
            _intersection->addWatchedVehicle(this);
        }
    }
    _section = _intersection->nextSection(_route->inter(2));
    _lane = _section->lanesFrom(_intersection)[0];
    _stopCS = "";

    dynamic_cast<CSNode*>(_intersection)->addEV(this);

//    _runCS2Section();

}

// by uchida 2016/5/20
//======================================================================
void Vehicle::_runCS2Section()
{
    assert(_section);

    _length = 10.0;

    // by uchida 2017/2/27
    if (_section->intersection(true) != _intersection)
    {
        _realPassedRoute.push_back(_section->intersection(true));
    }
    else if (_section->intersection(false) != _intersection)
    {
        _realPassedRoute.push_back(_section->intersection(false));
    }

    // by takusagawa 2018/10/29
    // 経路選択の際, 待ち時間を二重に考慮していたため以下コメントアウト
    // _intersectionに交差点通過時間を通知
    // int from = _intersection->direction(_prevIntersection);
    // int to   = _intersection->direction(_section);
    // assert(0<=from && from<_intersection->numNext());
    // assert(0<=to && to<_intersection->numNext());

    // _intersection
    //     ->addPassTime(from, to, TimeManager::time()-_entryTime);
    // commented by uchida 2016/5/22
    // 関係ない気もするけど，ここをentryTimeにすると
    // 充電時間の分だけ過大評価になる
    // ->のでVehicleEV::run()内部に移動した
    // _entryTime = TimeManager::time();

    // _watchedVehiclesの登録状況の受渡し
    /*
     * 本来は車線変更中にレーン束を移ることはないはずだが...
     */
    if (_isNotifying)
    {
        _intersection->eraseWatchedVehicle(this);
        _section->addWatchedVehicle(this);
    }

    // 車線変更に失敗したとき場合などのために
    // 必要であればここでルートを再探索する。
    if(_prevIntersection != NULL)
    {
        Intersection* next
            = _intersection
            ->next(_intersection->direction(_section));
        if (dynamic_cast<ODNode*>(next)==NULL
            && _route ->next(_intersection, next)==NULL)
        {
            reroute(_section, _intersection);
        }
    }

    _prevIntersection = _intersection;

    _intersection = NULL;

    _localRouter.clear();
    _localRouter.localReroute(_section, _lane, _length);
    _decideNextLane(_section, _lane);

    // by uchida 2017/2/8
    // CS入庫時刻の登録
    setStartChargingTime(TimeManager::time());

}

//======================================================================
void Vehicle::_runSection2Section()
{
    _lane = _nextLane;
    if (!_localRouter.isSearched())
    {
        _localRouter.clear();
        _localRouter.localReroute(_section, _lane, _length);
    }

    _decideNextLane(_section, _lane);
}

//======================================================================
void Vehicle::_runSection2Intersection()
{
    _intersection = _section->nextIntersection(_lane);
    assert(_intersection);

    // 車線変更を行いながら交差点に入るとMATESが落ちるのでここでトラップ．
    // とりあえず車線変更を緊急中断
    if (_laneShifter.isActive())
    {
        /*
        cerr << "WARNING: would enter intersection while lane shifting."
             << endl
             << "vehicle:" << _id
             << " in section:" << _section->id() << endl;
        */
        _laneShifter.abortShift();
    }

    //最後に通過した交差点として新しい交差点を登録。
    if(_prevIntersection == NULL)
    {
        _route
            ->setLastPassedIntersection(_intersection);
    }
    else
    {
        _route
            ->setLastPassedIntersection(_prevIntersection,
                                        _intersection);
    }
    //最後に通過した経由地として新しい交差点を登録。
    _router
        ->setLastPassedStopPoint(_intersection->id());

    //_watchedVehiclesの登録状況の受渡し
    /*
     * 本来は車線変更中にレーン束を移ることはないはずだが...
     */
    if (_isNotifying)
    {
        _section->eraseWatchedVehicle(this);
        if (!(dynamic_cast<ODNode*>(_intersection)))
        {
            // ODノードに入る(次のステップで消える)ときにはaddしない
            _intersection->addWatchedVehicle(this);
        }
    }

    _section = NULL;
    _lane = _nextLane;
    // ODノードでなければ次のレーンを検索
    if (dynamic_cast<ODNode*>(_intersection)==NULL)
    {
        _decideNextLane(_intersection, _lane);
    }
    // // ガソリン車でODノードに入ったなら
    // else if (_type < 80)
    // {
    //     _leaders
    // }

    // NCSNode(=OD)に入るEVは必ず充電する
    if (dynamic_cast<NCSNode*>(_intersection) && _type >= 80)
    {
        dynamic_cast<NCSNode*>(_intersection)->addEV(this);
    }

}

//======================================================================
void Vehicle::_decideNextLane(Intersection* inter, Lane* lane)
{
    // 次のレーン候補の数
    int num = lane->nextLanes()->size();
    if (num)
    {
        /*
         * LocalRouteは交差点を出るまでの経路を持っている
         * 正確には，交差点を出て次の単路に入った
         * 最初のレーンまでを保持している
         */
        if (_localRouter.isSearched())
        {
            _nextLane = _localRoute.next(lane);
        }
        else
        {
            _nextLane = lane->nextStraightLane();
        }
        assert(_nextLane);
    }
    else
    {
        // この後のrerouteに任せる
    }
}

//======================================================================
void Vehicle::_decideNextLane(Section* section, Lane* lane)
{
    // 次のレーン候補の数
    int num = lane->nextLanes()->size();
    if (num!=0)
    {
        if(_localRouter.isSearched())
        {
            _nextLane = _localRoute.next(lane);
        }
        else
        {
            _nextLane = lane->nextStraightLane();
        }
        assert(_nextLane);
    }
}

//======================================================================
bool Vehicle::_setLane(Intersection* inter, Lane* lane, double length)
{
    bool result = false;
    assert(_roadMap!=NULL && inter!=NULL && lane!=NULL);
    if (inter->isMyLane(lane))
    {
        _lane = lane;
        _decideNextLane(inter, lane);
        result = true;
    }
    return result;
}

//======================================================================
bool Vehicle::_setLane(Section* section, Lane* lane, double length)
{
    bool result = false;
    assert(_roadMap!=NULL && section!=NULL && lane!=NULL);
    if (section->isMyLane(lane))
    {
        _localRouter.clear();
        _localRouter.localReroute(section, lane, length);
        _lane = lane;
        _decideNextLane(section, lane);
        assert(_nextLane);
        result = true;
    }
    return result;
}

//======================================================================
bool Vehicle::addToSection(RoadMap* roadMap,
                           Section* section,
                           Lane* lane,
                           double length)
{
    // 車が初めて登場したときにだけ呼ばれる
    bool check = false;
    if (roadMap!=NULL)
    {
        _roadMap = roadMap;
        _section = section;
        _prevIntersection = section->intersection(!(section->isUp(lane)));
        _length = length;
        _entryTime = TimeManager::time();

        check = _setLane(section, lane, length);

//#ifndef GENERATE_VELOCITY_0
//        if (lane->tailAgent())
//        {
//            _velocity = lane->tailAgent()->velocity();
//        }
//        else
//        {
//            _velocity = lane->speedLimit()/60/60;
//        }
//#endif //GENERATE_VELOCITY_0

        // とりあえず初速を0にしてみた
        // (15/12/7 by uchida)→もどした
        // _velocity = 0.0;

        double limit = GVManager::getNumeric("GENERATE_VELOCITY_LIMIT")/60/60;
        if (limit >= 0.0 && _velocity > limit)
        {
            _velocity = limit;
        }

    }
    assert(check);
    lane->setUsed();
    return check;
}
