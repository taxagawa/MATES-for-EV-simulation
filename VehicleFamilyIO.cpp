/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "VehicleFamilyIO.h"
#include "VehicleFamilyManager.h"
#include "GVManager.h"
#include "AmuStringOperator.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

using namespace std;

//======================================================================
void VehicleFamilyIO::getReadyVehicleFamily()
{
    string fVehicleFamily;
    GVManager::getVariable("VEHICLE_FAMILY_FILE", &fVehicleFamily);

    ifstream inVehicleFamilyFile(fVehicleFamily.c_str(), ios::in);

    if (!inVehicleFamilyFile.good())
    {
        cout << "no vehicle family file: "
             << fVehicleFamily << endl;
        return;
    }

    // ファイルの読み込み
    string str;
    while (inVehicleFamilyFile.good())
    {
        getline(inVehicleFamilyFile, str);
        AmuStringOperator::getAdjustString(&str);
        if (!str.empty())
        {
            vector<string> tokens;
            AmuStringOperator::getTokens(&tokens, str, ',');
            if (tokens.size()==10)
            {
                int id;
                double bodyLength, bodyWidth, bodyHeight;
                double maxAcceleration, maxDeceleration;
                double bodyColorR, bodyColorG, bodyColorB;

                id = atoi(tokens[0].c_str());
                bodyLength = atof(tokens[1].c_str());
                bodyWidth  = atof(tokens[2].c_str());
                bodyHeight = atof(tokens[3].c_str());

                maxAcceleration = atof(tokens[5].c_str());
                maxDeceleration = atof(tokens[6].c_str());

                bodyColorR = atof(tokens[7].c_str());
                bodyColorG = atof(tokens[8].c_str());
                bodyColorB = atof(tokens[9].c_str());

                VFAttribute vehicleFamily
                    (id,
                     bodyLength, bodyWidth, bodyHeight,
                     0.0,
                     maxAcceleration, maxDeceleration,
                     bodyColorR, bodyColorG, bodyColorB);
                VehicleFamilyManager::addVehicleFamily(vehicleFamily);
            }
        }
    }
}

//======================================================================
void VehicleFamilyIO::print()
{
    cout << endl
         << "*** Vehicle Family Parameters ***" << endl;

    map<int, VFAttribute>* families = VehicleFamilyManager::families();
    for (map<int, VFAttribute>::iterator itvf = families->begin();
         itvf!=families->end(); itvf++)
    {
        int id;
        double length, width, height;
        double weight = 0.0;
        double accel, decel;
        double r, g, b;

        id = itvf->second.id();
        itvf->second.getSize(&length, &width, &height);
        itvf->second.getPerformance(&accel, &decel);
        itvf->second.getBodyColor(&r, &g, &b);

        cout << id << ": "
             << "(" << length << ", " << width << ", " << height
             << "/" << weight << "), ("
             << accel << ", " << decel << "), ("
             << r << ", " << g << ", " << b << ")" << endl; 
    }
}
