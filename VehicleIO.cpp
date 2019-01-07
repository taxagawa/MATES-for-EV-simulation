/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "VehicleIO.h"
#include "Vehicle.h"
#include "ObjManager.h"
#include "Lane.h"
#include "LaneBundle.h"
#include "GVManager.h"
#include "FileManager.h"
#include "AmuConverter.h"
#include "AmuVector.h"
#include "Conf.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#ifdef USE_MINGW
#include <io.h>
#else // USE_MINGW
#include <sys/types.h>
#include <sys/stat.h>
#endif // USE_MINGW
#include <cassert>
#include <cmath>
#include <sstream>

#define NUM_FIGURE_FOR_TIMELINE_FILENAME 10

#ifdef USE_ZLIB
#include <zlib.h>
#define fzopen   gzopen
#define fzclose  gzclose
#define fzprintf gzprintf
#define fz_ok    Z_OK
#else // USE_ZLIB
#define fzopen   fopen
#define fzclose  fclose
#define fzprintf fprintf
#define fz_ok    0
#endif // USE_ZLIB

using namespace std;

const string VehicleIO::_timePrefix = "#Time=";

//======================================================================
VehicleIO::VehicleIO()
{
    // by takusagawa 2019/1/7
    // timeFile関連を追加
    string resultDir, attributeFile, tripFile, timeFile;
    GVManager::getVariable("RESULT_OUTPUT_DIRECTORY", &resultDir);
    GVManager::getVariable("RESULT_VEHICLE_ATTRIBUTE_FILE", &attributeFile);
    GVManager::getVariable("RESULT_VEHICLE_TRIP_FILE", &tripFile);
    GVManager::getVariable("RESULT_VEHICLE_TIME_FILE", &timeFile);
    _extension
        = static_cast<int>(
            GVManager::getNumeric("OUTPUT_VEHICLE_EXTENSION"));

    _attributeOutFileName = attributeFile;
    _tripOutFileName  = tripFile;
    _timeOutFileName  = timeFile;

    // 以前の結果を消去する
    _attributeOut.open(_attributeOutFileName.c_str(), ios::trunc);
    if (_attributeOut.good())
    {
        _attributeOut.close();
    }
    _tripOut.open(_tripOutFileName.c_str(), ios::trunc);
    if (_tripOut.good())
    {
        _tripOut.close();
    }
    // by takusagawa
    _timeOut.open(_timeOutFileName.c_str(), ios::trunc);
    if (_timeOut.good())
    {
        _timeOut.close();
    }
}

//======================================================================
VehicleIO& VehicleIO::instance()
{
    static VehicleIO instance;
    return instance;
}

//======================================================================
bool VehicleIO::writeVehiclesDynamicData(const ulint time,
					 vector<Vehicle*>* vehicles)
{
    // (可視)車両のカウンター
    int nVisible = 0;
    int nAll = 0;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 詳細データを出力する場合
    if (GVManager::getFlag("FLAG_OUTPUT_TIMELINE_D"))
    {
        //--------------------------------------------------------------
        // ファイル名を決定する
        vector<string> paths;
        string resultDir;

        GVManager::getVariable("RESULT_TIMELINE_DIRECTORY", &resultDir);
        paths.push_back(resultDir+"vehicle/");
        string strTime
            = AmuConverter::itos(time,
                                 NUM_FIGURE_FOR_TIMELINE_FILENAME);
        paths.push_back(strTime.substr(0,3)+"/");
        paths.push_back(strTime.substr(3,3)+"/");
#ifdef USE_ZLIB
        paths.push_back(strTime.substr(6,4)+".txt.gz");
#else // USE_ZLIB
        paths.push_back(strTime.substr(6,4)+".txt");
#endif // USE_ZLIB
        string dynamicPath = "";
        for (unsigned int i=0; i<paths.size(); i++)
        {
            dynamicPath += paths[i];
        }

        //--------------------------------------------------------------
        // ファイルを開く
#ifdef USE_ZLIB
        _dynamicOut = fzopen(dynamicPath.c_str(), "wb6f");
#else // USE_ZLIB
        _dynamicOut = fzopen(dynamicPath.c_str(), "w");
#endif // USE_ZLIB
        if (_dynamicOut==NULL)
        {
        // ディレクトリが準備されていない場合には作成を試みる
            paths.pop_back();
            if(_makeDirectories(paths))
            {
#ifdef USE_ZLIB
                _dynamicOut = fzopen(dynamicPath.c_str(), "wb6f");
#else // USE_ZLIB
                _dynamicOut = fzopen(dynamicPath.c_str(), "w");
#endif // USE_ZLIB
            }
        }
        // ディレクトリを作ろうとしてもファイルが開けない場合には
        // 諦めて落ちる。
        if (_dynamicOut==NULL)
        {
            cerr << "cannot open file:" << dynamicPath << endl;
            assert(0);
            exit(EXIT_FAILURE);
        }

        //--------------------------------------------------------------
        // ファイルの出力
        if (GVManager::getNumeric("OUTPUT_COMMENT_IN_FILE")!=0)
        {
            fzprintf(_dynamicOut, "%s\n", _timePrefix.c_str());
        }
        for (int i=0; i<static_cast<signed int>((*vehicles).size()); i++)
        {
            // 車両データの出力
            /*
             * 発生直後の自動車は速度が非現実的であると言う理由から
             * 出力されない場合がある．
             */
            if ((*vehicles)[i]->isAwayFromOriginNode())
            {
                _writeVehicleDynamicData((*vehicles)[i]);
                nVisible++;
            }
        }

        //--------------------------------------------------------------
        // ファイルを閉じる
        int fzcl = fzclose(_dynamicOut);
        if (fzcl != fz_ok)
        {
            cerr << "cannot close file:" << dynamicPath << endl;
            exit(EXIT_FAILURE);
        }

        //--------------------------------------------------------------
        // 統計データの書き出し
        if (GVManager::getFlag("FLAG_OUTPUT_TIMELINE_S"))
        {
            // vehicleCount.txtのオープン
            string fVehicleCount;
            GVManager::getVariable("RESULT_VEHICLE_COUNT_FILE",
                                   &fVehicleCount);
            ofstream& ofsVC = FileManager::getOFStream(fVehicleCount);
            // オープンに失敗した場合は関数内で落ちるはず

            // 車両台数等の動的グローバル情報の書き出し
            nAll = static_cast<signed int>((*vehicles).size());
            ofsVC << time << "," << nAll << "," << nVisible << endl;
        }
    }
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // 詳細データを出力しない場合
    else
    {
        //--------------------------------------------------------------
        // 可視車両数のカウント
        for (int i=0; i<static_cast<signed int>((*vehicles).size()); i++)
        {
            if ((*vehicles)[i]->isAwayFromOriginNode())
            {
                nVisible++;
            }
        }
        //--------------------------------------------------------------
        // 統計データの書き出し
        if (GVManager::getFlag("FLAG_OUTPUT_TIMELINE_S"))
        {
            // vehicleCount.txtのオープン
            string fVehicleCount;
            GVManager::getVariable("RESULT_VEHICLE_COUNT_FILE",
                                   &fVehicleCount);
            ofstream& ofsVC = FileManager::getOFStream(fVehicleCount);
            // オープンに失敗した場合は関数内で落ちるはず

            // 車両台数等の動的グローバル情報の書き出し
            nAll = static_cast<signed int>((*vehicles).size());
            ofsVC << time << "," << nAll << "," << nVisible << endl;
        }
    }

    return true;
}

//======================================================================
bool VehicleIO::_writeVehicleDynamicData(Vehicle* vehicle)
{
    assert(_dynamicOut);

    // 角度の計算
    /*
     * theta = xy平面に投影したときの(0,1,0)方向との間の角
     * (反時計回りを正とする)
     *
     * phi   = 勾配.xy平面となす角(上り坂が正，下り坂が負となる)
     */
    double theta = vehicle->directionVector().calcAngle
        (AmuVector(0,1,0))*180*M_1_PI;
    // -pi<=theta<piであるので，0<=theta<2piに変更
    if (theta<0)
        theta += 360;
    double phi = atan(vehicle->lane()->gradient()/100.0)*180*M_1_PI;

    stringstream ss;
    ss.str("");
    ss << vehicle->id() << ","
       << vehicle->type() << ","
       << vehicle->x() << ","
       << vehicle->y() << ","
       << vehicle->z() << ","
       << theta << ","
       << phi << ","
       << vehicle->velocity()*1000 << ","
       << vehicle->accel()*1.0e+6 << ",";
    if (vehicle->intersection()!=NULL)
    {
        ss << vehicle->intersection()->id() << ",";
    }
    else
    {
        ss << "NULL,";
    }
    if (vehicle->section()!=NULL)
    {
        ss << vehicle->section()->id() << ",";
    }
    else
    {
        ss << "NULL,";
    }

    // 追加情報出力、0:なし、1:ウィンカ、2:先行者ID
    if (_extension >= 1)
    {
        ss << vehicle->blinker().state() << ",";
        if (_extension >= 2)
        {
            int leaderSize = vehicle->virtualLeaders()->size();
            ss << leaderSize << ",";
            for (int i = 0; i < leaderSize ; i++)
            {
                ss << vehicle->virtualLeaders()->at(i)->id() << ",";
            }
        }
    }

    fzprintf(_dynamicOut, "%s\n", ss.str().c_str());

    return true;
}

//======================================================================
bool VehicleIO::_makeDirectories(vector<string> paths) const
{
    bool result = false;

    string tmpPath = "";
    for (unsigned int i=0; i<paths.size(); i++)
    {
        tmpPath += paths[i];
    }

#ifndef USE_MINGW
    // パーミッション設定
    mode_t mode = S_IRUSR | S_IRGRP | S_IXUSR
        | S_IXGRP | S_IWUSR | S_IWGRP;
#endif //USE_MINGW

    // ディレクトリ作成
#ifdef USE_MINGW
    if (_mkdir(tmpPath.c_str())!=0)
#else
        if (mkdir(tmpPath.c_str(), mode)!=0)
#endif
        {
            /*
             * mkdirは成功したときに0，失敗したときに-1を返し，
             * 失敗したときはエラーコードが変数errno(cerrnoで宣言)に
             * 格納される．
             * man 2 mkdir を参照．
             */
            if (errno==ENOENT && paths.size()>=2)
            {
                // tmpPathの中のどれかのディレクトリが存在しない場合には
                // 成功するまで(可能な限り)上の階層にもどる
                paths.pop_back();
#ifdef USE_MINGW
                if(_makeDirectories(paths)
                   && _mkdir(tmpPath.c_str())==0)
#else
                    if(_makeDirectories(paths)
                       && mkdir(tmpPath.c_str(), mode)==0)
#endif
                    {
                        result = true;
                    }
            }
            else
            {
                cerr << errno << endl;
                cerr << "cannot make directory: " << tmpPath << endl;
                assert(0);
            }
        }
        else {
            result = true;
        }
    return result;
}

//======================================================================
bool VehicleIO::writeVehicleStaticData(Vehicle* vehicle)
{
    bool result = false;

    _attributeOut.open(_attributeOutFileName.c_str(), ios::app);
    if (_attributeOut.good())
    {
        _attributeOut << vehicle->id() << ","
                      << vehicle->type() << ","
                      << vehicle->bodyLength() << ","
                      << vehicle->bodyWidth() << ","
                      << vehicle->bodyHeight() << endl;

        _attributeOut.close();
        result = true;
    }
    return result;
}

//======================================================================
bool VehicleIO::writeVehicleDistanceData(Vehicle* vehicle)
{
    // シミュレーション中にゴールに到達した車両
    bool result = false;

    _tripOut.open(_tripOutFileName.c_str(), ios::app);
    if (_tripOut.good())
    {
        _tripOut << vehicle->id() << ","
                 << vehicle->type() << ","
                 << vehicle->startTime() << ","
                 << TimeManager::time() << ","
                 // by uchida 2017/2/8
                 // CSでの充電時間を記録
                 // まずCSの待ち行列に追加された時刻
                 << vehicle->waitingLineEntryTime() << ","
                 // 充電開始時刻(上下の２つは同じ値になることもある)
                 << vehicle->startChargingTime() << ","
                 // CSの出庫時刻
                 << vehicle->restartTime() << ","
                 // それにあわせて全旅行時間から割り引く
                 << (TimeManager::time() - vehicle->startTime())
                    - (vehicle->restartTime() - vehicle->waitingLineEntryTime()) << ","
                 << vehicle->route()->start()->id() << ","
                 << vehicle->route()->goal()->id() << ","
                 << vehicle->tripLength() << ",";

        // by takusagawa 2018/11/10
        // 待ち時間情報を受け取れるかどうか.
        // 受け取れる場合は1,受け取れない場合は0
        if (vehicle->receiveWaitingInfo())
        {
            _tripOut << "1" << endl;
        }
        else
        {
            _tripOut << "0" << endl;
        }

        _tripOut.close();
        result = true;
    }

    // by takusagawa
    _timeOut.open(_timeOutFileName.c_str(), ios::app);
    if (_timeOut.good())
    {
        if (vehicle->type() >= 80)
        {
            _timeOut << vehicle->waitingLineEntryTime() << "," <<  vehicle->getChargeFlagTime() << "," << vehicle->getEstimatedArrivalTime() << endl;
        }
        _timeOut.close();
    }
    return result;
}

//======================================================================
bool VehicleIO::writeAllVehiclesDistanceData()
{
    // シミュレーション終了時に走行中の車両
    bool result = false;

    _tripOut.open(_tripOutFileName.c_str(), ios::app);
    if (_tripOut.good())
    {
        vector<Vehicle*>* vehicles = ObjManager::vehicles();

        for (unsigned int i=0; i<vehicles->size(); i++)
        {
            _tripOut << (*vehicles)[i]->id() << ","
                     << (*vehicles)[i]->type() << ","
                     << (*vehicles)[i]->startTime() << ","
                     << "******" << ","
                     // by uchida 2017/2/8
                     // CSでの充電時間を記録
                     << (*vehicles)[i]->waitingLineEntryTime() << ","
                     << (*vehicles)[i]->startChargingTime() << ","
                     << (*vehicles)[i]->restartTime() << ",";

            // by takusagawa 2018/11/10
            // 条件式の後半部分を追加
            // これはシミュレーション終了時に待ち行列中にいるが,充電中ではない車両の条件
            // この車両もrestartTimeが0なので,else以下の式にはならない.
            if ((*vehicles)[i]->onCharging() || (*vehicles)[i]->isWaiting())
            {
                // 充電中であれば
                _tripOut <<
                ((*vehicles)[i]->waitingLineEntryTime() - (*vehicles)[i]->startTime()) << ",";
            }
            else
            {
                // 充電中でなければ
                _tripOut <<
                (TimeManager::time() - (*vehicles)[i]->startTime()) - ((*vehicles)[i]->restartTime() - (*vehicles)[i]->waitingLineEntryTime()) << ",";
            }

            _tripOut << (*vehicles)[i]->route()->start()->id() << ","
                     << (*vehicles)[i]->route()->goal()->id() << ","
                     << (*vehicles)[i]->tripLength() << ",";

            // by takusagawa 2018/11/10
            // 待ち時間情報を受け取れるかどうか.
            // 受け取れる場合は1,受け取れない場合は0
            if ((*vehicles)[i]->receiveWaitingInfo())
            {
                _tripOut << "1" << endl;
            }
            else
            {
                _tripOut << "0" << endl;
            }
        }

        _tripOut.close();
        result = true;
    }

    // by takusagawa 2019/1/7
    _timeOut.open(_timeOutFileName.c_str(), ios::app);
    if (_timeOut.good())
    {
        vector<Vehicle*>* vehicles = ObjManager::vehicles();

        for (unsigned int i=0; i<vehicles->size(); i++)
        {
            if ((*vehicles)[i]->type() >= 80)
            {
                _timeOut << (*vehicles)[i]->waitingLineEntryTime() << "," << (*vehicles)[i]->getChargeFlagTime() << "," << (*vehicles)[i]->getEstimatedArrivalTime() << endl;
            }
        }
        _timeOut.close();
    }
    return result;
}
