/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "Route.h"
#include "Intersection.h"
#include <iostream>
#include <algorithm>
#include <cassert>

using namespace std;

//======================================================================
Route::Route()
{
    _cost = 0;
    _lastPassedIntersection = -1;
}

//======================================================================
Route::~Route(){}

//======================================================================
void Route::push(Intersection* isec)
{
    if(isec != NULL)
    {
        _route.push_back(isec);
    }
}

//======================================================================
void Route::push(std::vector<Intersection*>& isec_vec)
{
    if(isec_vec.size() == 0)
    {
        return;
    }
    
    // 最後の交差点が重複しないように気を付けながら末尾に追加
    if(_route.size() == 0)
    {
        _route.insert(_route.end(), isec_vec.begin(), isec_vec.end());
    }
    else
    {
        vector<Intersection*>::iterator it = _route.end();
        vector<Intersection*>::iterator it_in = isec_vec.begin();

        it--; // 最後尾の交差点を取得
        // 最後尾と先頭を比較して同一だったら追加開始位置をひとつずらす。
        if((*it)->id().compare((*it_in)->id()) == 0)
        {
            it_in++;
        }
        _route.insert(_route.end(), it_in, isec_vec.end());
    }
}

//======================================================================
Intersection* Route::pop()
{
    Intersection* result = NULL;
    if(! _route.empty())
    {
        result = _route.back();
        _route.pop_back();
    }
    return result;
}

//======================================================================
Intersection* Route::back()
{
    return _route.back();
}

//======================================================================
void Route::setLastPassedIntersection(Intersection* passed)
{
    assert(passed);

    for(unsigned int i = _lastPassedIntersection + 1;
        i < _route.size();
        i++)
    {
        if(_route[i] == passed)
        {
            _lastPassedIntersection = i;
            break;
        }
    }
}

//======================================================================
void Route::setLastPassedIntersection(Intersection* prev,
                                      Intersection* passed)
{
    if(prev == NULL || _lastPassedIntersection < 0)
    {
        setLastPassedIntersection(passed);
        return;
    }

    assert(prev);
    assert(passed);

    for (unsigned int i = _lastPassedIntersection+1;
         i < _route.size();
         i++)
    {
        if (_route[i-1] == prev && _route[i] == passed)
        {
            _lastPassedIntersection = i;
            break;
        }
    }
}

//======================================================================
int Route::lastPassedIntersectionIndex() const
{
    return _lastPassedIntersection;
}

//======================================================================
Intersection* Route::prev(Intersection* isec) const
{
    Intersection* result = NULL;
    vector<Intersection*>::const_iterator where;
    where = find(_route.begin(), _route.end(), isec);
    if (where != _route.begin())
    {
        where--;
        result = (*where);
    }

    return result;
}
//======================================================================
Intersection* Route::next(Intersection* isec) const
{
    Intersection* result = NULL;
    vector<Intersection*>::const_iterator offset;
    vector<Intersection*>::const_iterator where;

    //最後に通過した交差点以降のルートでisecを探す
    if(_lastPassedIntersection < 0)
    {
        offset = _route.begin();
    }
    else if(_lastPassedIntersection
            < static_cast<signed int>(_route.size()))
    {
        offset = _route.begin() + _lastPassedIntersection;
    }
    else
    {
        //ここに到達することはないはずだが。。。
        offset = _route.begin();
    }

    where = find(offset, _route.end(), isec);
    if(where != _route.end())
    {
        where++;
        if(where != _route.end())
        {
            result = (*where);
        }
    }
  
    return result;
}

//======================================================================
Intersection* Route::next(Intersection* is1,Intersection* is2) const
{
    assert(is1);
    assert(is2);
    unsigned int i;
    int offset;
    Intersection* result = NULL;

    //最後に通過した交差点以降のルートでisecを探す
    if(_lastPassedIntersection < 1)
    {
        offset = 0;
    }
    else if(_lastPassedIntersection
            < static_cast<signed int>(_route.size()))
    {
        offset = _lastPassedIntersection - 1;
    }
    else
    {
        //ここに到達することはないはずだが。。。
        offset = 0;
    }

    for (i = offset; i < _route.size()-2; i++)
    {
        if(_route[i] == is1 && _route[i+1] == is2) break;
    }
    if (i < _route.size()-2)
    {
        result = _route[i+2];
    }
    return result;
}

//======================================================================
Intersection* Route::start() const
{
    assert(!_route.empty());
    return _route[0];
}

//======================================================================
Intersection* Route::goal()  const
{
    assert(!_route.empty());
    return _route[_route.size()-1];
}

//======================================================================
const vector<Intersection*>* Route::route() const
{
    return &_route;
}

//======================================================================
Intersection* Route::inter(int num) const
{
    Intersection* result = NULL;
  
    if(num < static_cast<signed int>(_route.size()))
    {
        result = _route[num];
    }

    return result;
}

//======================================================================
double Route::cost() const
{
    return _cost;
}

//======================================================================
void Route::setCost(double c)
{
    if(c >= 0.0)
    {
        _cost = c;
    }
}

//======================================================================
int Route::size() const
{
    return _route.size();
}

//======================================================================
void Route::print(std::ostream& out) const
{
    out << "Route: lastly passed intersection index = "
        << _lastPassedIntersection << endl;
    for(int i = 0; i < static_cast<signed int>(_route.size()); i++)
    {

//        if(i != _lastPassedIntersection)
//        {
//            out << "\t" << _route[i]->id() << endl;
//        }
//        else
//        {
//            out << "\t" << _route[i]->id() << "<< " << endl;
//        }

        // by uchida 2016/5/23
        // isCSが機能しているかどうかの確認
        if (_route[i]->isCS())
        {

            if(i != _lastPassedIntersection)
            {
                out << "\t" << _route[i]->id() << "   CS" << endl;
            }
            else
            {
                out << "\t" << _route[i]->id() << "<< CS" << endl;
            }

        }
        else
        {

            if(i != _lastPassedIntersection)
            {
                out << "\t" << _route[i]->id() << endl;
            }
            else
            {
                out << "\t" << _route[i]->id() << "<< " << endl;
            }

        }

    }
}
