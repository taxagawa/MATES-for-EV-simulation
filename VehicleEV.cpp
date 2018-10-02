#include "VehicleEV.h"
#include "VehicleRunningMode.h"
#include "Random.h"
#include "SubNode.h"
#include "GVManager.h"
#include "Lane.h"
//#include "ObjManager.h"
#include <cassert>

class VehicleRunningMode;
class Intersection;

//======================================================================
VehicleEV::VehicleEV():_parent(){

  // 重量[kg]（車体重量+乗員重量）
  // 55kg/人×2名=110kgが標準の重量の模様

  // 諸々の出典
  // 道路運送車両の保安基準の細目を定める告示【2012.03.30】 別添42（軽・中量車排出ガスの測定方法）
  // http://www.mlit.go.jp/common/001056718.pdf
  _mass = 1180.0;

  // 全面投影面積[m2]
  _fronralProjectedArea = 2.14;

  // 電池容量[Wsec]
  // [W]=[kg * m2 / s3]
  // ( 1kWh = 1,000 * 3,600 * Wsec )
  _batteryCapacity = 10.5 * 1000.0 * 3600.0;// revised by uchida 2017/1/18

  // debug by uchida 2016/5/22
  // 充電残量は乱数で変化させる設定
  // double rnd = 0;
  _initialSoC = 0.0;

  // by uchida 2017/1/26
  // 0.3 < rnd < 0.8
  // by takusagawa 2018/01/07
  // OD距離に応じて初期SoCを変化させるようにした
  /*
  rnd = Random::uniform(_rnd);
  rnd = (rnd * 0.5) + 0.8 * (_parent->getOdDistance(this) / 35000); //35000は最大値を超えないように適当に与えた
  cout << _parent->id(this) << endl;
  if (rnd > 0.85)
  {
    _initialSoC = 0.85;
  }
  else if (rnd < 0.2)
  {
    _initialSoC = 0.2;
  }
  else
  {
    _initialSoC = rnd;
  }
  */

  _batteryRemain = 0.0;
//  _batteryRemain = _batteryCapacity;
  instantaneousValue = 0.0;
  accessory = 200.0;//[Wsec/100msec]

  // by takusagawa 2018/10/2
  _maxOD = 0.0;

  //充電走行開始時の閾値の設定
  _threshold = 0.3;

}

//======================================================================
VehicleEV::~VehicleEV(){
}

//======================================================================
void VehicleEV::run(){

  // 充電中の処理
  // 最終的には_onChargingをfalseに戻して再出発させないと
  // あと、sectionに車配置しとくのもよくない気はする
  if (_onCharging)
  {
    if (_swapTime >= 1)
    {
        _swapTime--;
        _sleepTime = TimeManager::unit() * 2;
    }
    else
    {
        charge();
    }
  }
  else if (_waiting)
  {
      _sleepTime = TimeManager::unit() * 2;
  }

  // 充電残量が直前ステップで負になった場合
  // 充電切れとしてレーンを閉塞させるため，消去する
  // 充電切れ判定のタイミングは再考の余地あり
  // (15/11/17 by uchida)
  if (_batteryRemain <= 0)
  {
    // by uchida 2017/2/15
    //これまでの経路を取得する
    if (GVManager::getFlag("FLAG_OUTPUT_SCORE"))
    {
        getSubNodes_del();
        scoringSubNodes();
    }

    _parent->addStrandedEVs(this);
  }
  else
  {
    // comented by uchida 2016/5/22
    // なんで_batteryRemain <= 1なの？
    // SoCと混同してる？
    assert(_batteryRemain > 0 || _batteryRemain <= 1);
    Vehicle::run();

    // commented by takusagawa 2018/09/22
    // runningModeはおそらく走行試験用のモードで、
    // ある時刻のときの速度を正確に定めてある
    if (_runningMode->isExist())
    {
        ulint elapsedTime = TimeManager::time() - _startTime;

        // 出力される速度はkm/hなのでm/sに変換
        double vel_1
            = _runningMode->velocity(elapsedTime % _runningMode->cycle()) / 3.6;
        double vel_2
            = _runningMode->velocity((elapsedTime - TimeManager::unit())
                % _runningMode->cycle()) / 3.6;

        // このタイミングで_velocity, _accelは[m/sec], [m/sec2]
        _velocity = vel_1;
        _accel = (vel_1 - vel_2) / (TimeManager::unit() / 1000.0);
    }

        double Froll
            = (RROLL * _mass * GRA)
            * _velocity * (TimeManager::unit() / 1000.0);

        // 空気抵抗
        double Faero
            = (RHO * CD * _fronralProjectedArea * pow(_velocity, 2.0) / 2.0)
            * _velocity * (TimeManager::unit() / 1000.0);

        // 勾配抵抗
        double Fhill
            = (_mass * GRA * phi())
            * _velocity * (TimeManager::unit() / 1000.0);

        // 加速抵抗
        double Faccel
            = ((1.0 + KROT) * _mass * _accel)
            * _velocity * (TimeManager::unit() / 1000.0);

        instantaneousValue = Froll + Faero + Fhill + Faccel;

//        double e = 0.546;
        double e = 1.0;
        instantaneousValue = instantaneousValue * e;

    if (instantaneousValue >= 0)
    {
        // 消費電力のSOC依存性
        // 現段階ではECR=-SOC + 2とする
        // (15/11/25 by uchida)
        _batteryRemain -= (instantaneousValue / ETA) * BC();

//if (_id == "000000")
//{
//        cout << (instantaneousValue / ETA) * BC() << ","
//             << 0 << endl;
//}
    }
    else
    {
        // 消費電力が負の場合は回生率をかけてエネルギーを回収
        // 回生率は速度依存性を考慮すべき
        // またSOCへの依存性があるかは要サーベイ
        // Yaoの論文からはSOCとt-1要素のどちらが結果の精度向上につながったのか不明
        // (15/11/25 by uchida)
        _batteryRemain -= (instantaneousValue * ETA) * regenerativeRate();

//if (_id == "000000")
//{
//        cout << 0 << ","
//             << (instantaneousValue * ETA) * regenerativeRate() << endl;
//}
    }

    // 消費電力に関係なく電装品分の電力は消費される
    _batteryRemain -= accessory;
  }

    // 電池残量は既定値内となる
    // SOCが一定値以下ならば経路探索のフラグを立てる
    // by uchida 2016/5/10
    // ODNode.cppのrerouteの際にも
    // このコードをコピペして利用している
    // by uchida 2016/5/30
  if (threshold() && !_isCharge)
  {
    onChargeflag();
  }

  if (_runningMode->isExist())
  {
    // このタイミングで_velocity, _accelは[m/msec], [m/msec2]
    _velocity = _velocity / 1000.0;
    _accel = _accel / pow(1000.0 ,2.0);
  }

  if (isSleep())
  {
    _velocity = 0;
    _accel = 0;
  }

}

//======================================================================
double VehicleEV::batteryRemain(){
  return _batteryRemain;
}

//======================================================================
double VehicleEV::SOC(){
  _stateOfCharge = _batteryRemain / _batteryCapacity;
  if (_stateOfCharge > 1)
  {
    _batteryRemain = _batteryCapacity;
    _stateOfCharge = 1;
  }
  else if (_stateOfCharge < 0)
  {
    _batteryRemain = 0;
    _stateOfCharge = 0;
  }

  // 2016/5/22
  // これは完全にVisualize用
  _SOC = _stateOfCharge;

  return _stateOfCharge;
}

// by uchida 2017/6/27
//======================================================================
double VehicleEV::initialSoC(){
  return _initialSoC;
}

// by uchida 2016/5/22
//======================================================================
double VehicleEV::BC(){
  return -1.0 * SOC() + 2.0;
}

//======================================================================
double VehicleEV::regenerativeRate(){
  assert(_velocity >= 0);

  // Yao論文を参考に速度依存の回生率を設定
  if (_velocity >= 5.0)
  {
    _regenerativeRate = 0.3 * (_velocity - 5.0) / 20.0 + 0.5;
  }
  else
  {
    _regenerativeRate = 0.5 * _velocity / 5.0;
  }

  // 138km/h（38.3m/s）以上で回生率1を上回るため
  if (_regenerativeRate > 1.0) _regenerativeRate = 1.0;

  return _regenerativeRate;
}

//by uchida 2017/2/2
//====================================================================
bool VehicleEV::threshold()
{

    if (SOC() < _threshold)
        return true;
    else
        return false;

}

//======================================================================
void VehicleEV::charge()
{

    // by uchida 2017/6/23
    // とりあえず元に戻しとく
    _swapTime = 0;

    // by uchida 2016/5/23
    CSNode* _csNode   = dynamic_cast<CSNode*>(_intersection);
    NCSNode* _ncsNode = dynamic_cast<NCSNode*>(_intersection);
    if (_csNode)
    {
        _csNode->sumCharge(_chargingValue);

        /* debug by takusagawa 2018/3/26
        if (_csNode->id() == "900106")
        {
          cout << "instantaneousCharge_" << _csNode->id() << " : "
          << _csNode->instantaneousCharge() << endl;
        }
        */

        // これよりちょっと小さいはずだけどとりあえず検証は後で
        // データによると急速CSにおける最大は50kW
        // 家庭用は2kW程度の模様
        // その場合以下の式は25　*　1000になるか
        _chargingValue = _csNode->outPower() * 1000.0 * (TimeManager::unit() / 1000.0);
        _batteryRemain += _chargingValue;

        // SOCが0.8以上であれば出発する
        // 急速充電でこれ以上にすることないようなので
        if (SOC() >= 0.8)
        {
            offChargeflag();
            _onCharging = false;
            _csNode->removeEV();
            Vehicle::_runCS2Section();
            // 充電時間の分だけ滞在時間が過大評価になるのを回避
            // (追記)　by uchida ここで回避しているのはあくまでSectionの滞在時間
            // 旅行時間全体を適正化するために_restartTimeを設定
            _entryTime = TimeManager::time();

            // by uchida 2017/2/8
            // CS出庫時刻の登録
            setRestartTime(TimeManager::time());

        }
        else
        {
            _sleepTime = TimeManager::unit() * 2;
        }
    }
    else if (_ncsNode)
    {
        _ncsNode->sumCharge(_chargingValue);

        // debug by takusagawa 2018/3/26
        //if (_ncsNode->id() == "000232")
        //{
        //  cout << "_chargingValue" << _ncsNode->id() << " : "
        //  << _chargingValue << endl;
        //}


        _chargingValue = _ncsNode->outPower() * 1000.0 * (TimeManager::unit() / 1000.0);
        _batteryRemain += _chargingValue;
        if (SOC() >= 0.8)
        {
            offChargeflag();
            _onCharging = false;
            _ncsNode->removeEV();
        }
        else
        {
            _sleepTime = TimeManager::unit() * 2;
        }
    }

}

// by takusagawa 2018/01/12
//====================================================================
void VehicleEV::setInitSoC()
{
  double rnd = 0.0;

  // by uchida 2017/1/26
  // 0.3 < rnd < 0.8
  // by takusagawa 2018/01/07
  // OD距離に応じて初期SoCを変化させるようにした
  rnd = Random::uniform(_rnd);
  rnd = (rnd * 0.5) + 0.6 * (getOdDistance() / _maxOD);
  //35000は最大値を超えないように適当に与えた
  //TODO いずれはマップ内の全交差点間の最長距離を設定したい

  if (rnd > 0.85)
  {
    _initialSoC = 0.85;
  }
  else if (rnd < 0.2)
  {
    _initialSoC = 0.2;
  }
  else
  {
    _initialSoC = rnd;
  }
  
  _batteryRemain = _batteryCapacity * _initialSoC;
}

// by takusagawa 2018/9/27
//====================================================================
double VehicleEV::getBatteryCapacity() const
{
    return _batteryCapacity;
}


void VehicleEV::setMaxOdDistance(double max)
{
    _maxOD = max;
}
