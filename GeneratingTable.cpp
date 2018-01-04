/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "GeneratingTable.h"
#include "Conf.h"
#include "Router.h"
#include "TimeManager.h"
#include "GVManager.h"
#include "AmuStringOperator.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

using namespace std;

//######################################################################
// GTCellクラス
//======================================================================
GTCell::GTCell():_begin(0),_end(0), _volume(0){}

//======================================================================
GTCell::~GTCell(){}

//======================================================================
bool GTCell::setValue(ulint begin, ulint end, double volume,
		      const std::string& start, const std::string& goal)
{
    if (begin <= end && volume >= 0)
    {
        _begin = begin;
        _end = end;
        _volume = volume;
        _generatedVolume = 0;
        // 背景交通にEV等発生させようとする場合にはここを変更する必要がある
        // 2015/10/28 by uchida
        _vehicleType = 20;
    }
    else
    {
        _begin = 0;
        _end = 0;
        _od.clear();
        return false;
    }
  
    if(!_od.setValue(start, goal))
    {
        return false;
    }
  
    return true;
}

//======================================================================
bool GTCell::setValue(ulint begin, ulint end, double volume, int vehicleType,
		      const std::string& start, const std::string& goal,
		      const std::vector<std::string>& stopPoints)
{
    if (begin <= end && volume >= 0)
    {
        _begin = begin;
        _end = end;
        _volume = volume;
        _vehicleType = vehicleType;
    }
    else
    {
        _begin = 0;
        _end = 0;
        _od.clear();
        return false;
    }
  
    if(!_od.setValue(start, goal, stopPoints))
    {
        return false;
    }

    return true;
}

//======================================================================
ulint GTCell::begin() const
{
    return _begin;
}

//======================================================================
ulint GTCell::end() const
{
    return _end;
}

//======================================================================
double GTCell::volume() const
{
    return _volume;
}

//======================================================================
unsigned int GTCell::generatedVolume() const
{
    return _generatedVolume;
}

//======================================================================
void GTCell::incrementGeneratedVolume()
{
    _generatedVolume++;
}

//======================================================================
int GTCell::vehicleType() const
{
    return _vehicleType;
}

//======================================================================
const string& GTCell::start() const
{
    return _od.start();
}

//======================================================================
const string& GTCell::goal() const
{
    return _od.goal();
}

//======================================================================
const vector<string>* GTCell::stopPoints() const
{
    return _od.stopPoints();
}

//======================================================================
const OD* GTCell::od() const
{
    return &_od;
}

//======================================================================
void GTCell::print() const
{
    cout << "beginTime: " << _begin
         << ", endTime: " << _end
         << ", volume: " << _volume
         << ", origin: " << _od.start()
         << ", destination: " << _od.goal()
         << ", stopPoints: ";

    const vector<string>* stopPoints = _od.stopPoints();
    if (stopPoints->empty())
    {
        cout << "none." << endl;
        return;
    }
    for (unsigned int i=0; i<stopPoints->size(); i++)
    {
        cout << (*stopPoints)[i] << " ";
    }
    cout << endl;
}

//######################################################################
// GTCellのソートに用いるクラス
class GTCellLess
{
public:
    /// GTCellの比較関数
    /**
     * 左辺のbegin（開始時刻）が右辺より小さければ真を返す．
     * 即ち，GTCellは開始時刻が早い順に整列される．
     */
    bool operator()(const GTCell* rl, const GTCell* rr) const
    {
        return (rl->begin() < rr->begin());
    }
};

//######################################################################
// GeneratingTableクラス
//======================================================================
GeneratingTable::GeneratingTable()
{
    _table.clear();
    _activatedTable.clear();
}

//======================================================================
GeneratingTable::~GeneratingTable()
{
    for (unsigned int i=0; i<_table.size(); i++)
    {
        delete _table[i];
    }
    _table.clear();
    for (unsigned int i=0; i<_activatedTable.size(); i++)
    {
        delete _activatedTable[i];
    }
    _activatedTable.clear();
}

//======================================================================
void GeneratingTable::createGTCell(
    ulint begin, ulint end, double volume,
    const std::string& start, const std::string& goal)
{
    GTCell* cell = new GTCell();
    cell->setValue(begin, end, volume, start, goal);
    _table.push_back(cell);
}

//======================================================================
void GeneratingTable::createGTCell(
    ulint begin, ulint end, double volume, int vehicleType,
    const std::string& start, const std::string& goal,
    const std::vector<std::string>& stopPoints)
{
    GTCell* cell = new GTCell;
    cell->setValue(begin, end, vehicleType,
                   volume, start, goal, stopPoints);
    _table.push_back(cell);
}

//======================================================================
void GeneratingTable::extractActiveGTCells(
    std::vector<const GTCell*>* result_GTCells)
{
    // _tableは開始時刻でソートされているので，先頭から要素を調査する
    while (_table.size()>0)
    {
        if (_table.front()->begin() <= TimeManager::time())
        {
            // _tableの先頭から要素を取り除き_activatedTableに追加する
            GTCell* activatedGTCell = _table.front();
            _activatedTable.push_back(activatedGTCell);
            result_GTCells->push_back(activatedGTCell);
            _table.pop_front();
        }
        else
        {
            break;
        }
    } 
}

//======================================================================
void GeneratingTable::extractActiveGTCellsAllAtOnce(
    std::vector<const GTCell*>* result_GTCells)
{
    // _tableは開始時刻でソートされているので，先頭から要素を調査する
    while (_table.size()>0)
    {
        // _tableの先頭から要素を取り除き_activatedTableに追加する
        GTCell* activatedGTCell = _table.front();
        _activatedTable.push_back(activatedGTCell);
        result_GTCells->push_back(activatedGTCell);
        _table.pop_front();
    } 
}

//======================================================================
void GeneratingTable::getValidGTCells(
    const std::string& intersectionId,
    std::vector<const GTCell*>* result_GTCells) const
{
    for(deque<GTCell*>::const_iterator where=_table.begin();
        where!=_table.end();
        where++)
    {
        if((*where)->start().compare(intersectionId)==0)
        {
            (*result_GTCells).push_back((*where));
        }
    }
}

//======================================================================
const GTCell* GeneratingTable::validGTCell(
    const std::string& intersectionId) const
{
    for(deque<GTCell*>::const_iterator where=_table.begin();
        where!=_table.end();
        where++)
    {
        if((*where)->start().compare(intersectionId)==0)
        {
            return (*where);
        }
    }
    return NULL;
}

//======================================================================
bool GeneratingTable::init(const std::string& fileName)
{
    bool result = false;
    fstream fin;
    fin.open(fileName.c_str(), ios::in);
    if(!fin.good())
    {
        cout << "no vehicle generate table file: "
             << fileName << endl;
        return false;
    }
    else
    {
        result = true;
    }
    vector<string> tokens;
    string str;
    while(fin.good())
    {
        getline(fin, str);
        //文字列の整形
        AmuStringOperator::getAdjustString(&str);
        if(!str.empty())
        {
            AmuStringOperator::getTokens(&tokens, str, ',');
        }
        if(tokens.size() >= 6)
        {
            int i;
            int curIndex;
            GTCell* cell = new GTCell();
            ulint begin;
            ulint end;
            double volume;
            int vehicleType;
            string start;
            string goal;
            int numStoppingInters;
            vector<string> stopPoints;
            vector<string>::iterator it;

            // 発生開始、終了時刻
            begin = atoi(tokens[0].c_str());
            end = atoi(tokens[1].c_str());

            // 出発地、目的地
            ostringstream ost0, ost1;
            ost0.width(NUM_FIGURE_FOR_INTERSECTION);
            ost0.fill('0');
            ost1.width(NUM_FIGURE_FOR_INTERSECTION);
            ost1.fill('0');
            ost0 << (tokens[2]);
            ost1 << (tokens[3]);

            start = ost0.str();
            goal = ost1.str();

            // 発生量
            volume
                = atof(tokens[4].c_str())
                * GVManager::getNumeric("TABLED_OD_FACTOR");

            // 車種ID
            vehicleType = atoi(tokens[5].c_str());

            // 経由地
            curIndex = 6;
            numStoppingInters = atoi(tokens[curIndex].c_str());
            curIndex++;
            stopPoints.reserve(numStoppingInters);
            for(i = 0; i < numStoppingInters; i++)
            {
                ostringstream ost;
                string stopPoint;
                ost.width(NUM_FIGURE_FOR_INTERSECTION);ost.fill('0');
                ost << (tokens[curIndex]);
                curIndex++;
                stopPoint = ost.str();
                stopPoints.push_back(stopPoint);
            }

            if(cell->setValue(begin, end, volume, vehicleType, 
                             start, goal, stopPoints))
            {
                _table.push_back(cell);
            }
            else
            {
                cerr << "Unknown error occured in GeneratingTable.\n"
                     << "begin, end, traffic volume, start, goal\n"
                     << begin << "\t" << end << "\t" << volume
                     << "\t" << start << "\t" << goal << endl;
                delete cell;
            }
            tokens.clear();
        }
    }
    fin.close();

    // _tableをソートする
    sort(_table.begin(), _table.end(), GTCellLess());

    return result;
}

//======================================================================
bool GeneratingTable::initFixedTable(const std::string& fileName)
{
    bool result = false;
    fstream fin;
    fin.open(fileName.c_str(), ios::in);
    if(!fin.good())
    {
        cout << "no vehicle generate table file: "
             << fileName << endl;
        return false;
    }
    else
    {
        result = true;
    }
    vector<string> tokens;
    string str;
    while(fin.good())
    {
        getline(fin, str);
        //文字列の整形
        AmuStringOperator::getAdjustString(&str);
        if(!str.empty())
        {
            AmuStringOperator::getTokens(&tokens, str, ',');
        }
        if(tokens.size() >= 4)
        {
            int i;
            int curIndex;
            GTCell* cell = new GTCell();
            ulint begin;
            int vehicleType;
            string start;
            string goal;
            int numStoppingInters;
            vector<string> stopPoints;
            vector<string>::iterator it;

            // 発生時刻
            begin = atoi(tokens[0].c_str());

            // 出発地、目的地
            ostringstream ost0, ost1;
            ost0.width(NUM_FIGURE_FOR_INTERSECTION);
            ost0.fill('0');
            ost1.width(NUM_FIGURE_FOR_INTERSECTION);
            ost1.fill('0');
            ost0 << (tokens[1]);
            ost1 << (tokens[2]);

            start = ost0.str();
            goal = ost1.str();

            // 車種ID
            vehicleType = atoi(tokens[3].c_str());

            // 経由地
            curIndex = 4;
            numStoppingInters = atoi(tokens[curIndex].c_str());
            curIndex++;
            stopPoints.reserve(numStoppingInters);
            for(i = 0; i < numStoppingInters; i++)
            {
                ostringstream ost;
                string stopPoint;
                ost.width(NUM_FIGURE_FOR_INTERSECTION);
                ost.fill('0');
                ost << (tokens[curIndex]);
                curIndex++;
                stopPoint = ost.str();
                stopPoints.push_back(stopPoint);
            }

            if(cell->setValue(begin, begin, 1, vehicleType, 
                              start, goal, stopPoints))
            {
                _table.push_back(cell);
            }
            else
            {
                cerr << "Unknown error occured in GeneratingTable.\n"
                     << "begin, start, goal\n"
                     << begin << "\t" 
                     << "\t" << start << "\t" << goal << endl;
                delete cell;
            }
            tokens.clear();
        }
    }
    fin.close();

    // _tableをソートする
    sort(_table.begin(), _table.end(), GTCellLess());
    
    return result;
}

//======================================================================
void GeneratingTable::print() const
{
    cout << "Table Unused:" << endl;
    for (unsigned int i=0; i<_table.size(); i++)
    {
        _table[i]->print();
    }
    cout << "Table Activated:" << endl;
    for (unsigned int i=0; i<_activatedTable.size(); i++)
    {
        _activatedTable[i]->print();
    }
}
