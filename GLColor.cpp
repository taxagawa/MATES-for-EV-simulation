/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#include "GLColor.h"
#include <autogl.h>
#include <cassert>

//######################################################################
void GLColor::setBackground()
{
  //AutoGL_SetBackgroundColor(0.75, 0.8, 1.0);
  AutoGL_SetBackgroundColor(1.0, 1.0, 1.0);
}
//----------------------------------------------------------------------
void GLColor::setGround()
{
  AutoGL_SetColor(0.8, 0.7, 0.5);
}
//----------------------------------------------------------------------
void GLColor::setSimpleNetwork()
{
  //AutoGL_SetColor(1.0, 1.0, 1.0);
  AutoGL_SetColor(0, 0, 0);
}

//######################################################################
void GLColor::setSubsection()
{
  AutoGL_SetColor(0.8, 0.8, 0.9);
}
//----------------------------------------------------------------------
void GLColor::setCrossWalk()
{
  AutoGL_SetColor(0.9, 0.9, 0.9);
}
//----------------------------------------------------------------------
void GLColor::setSideWalk()
{
  AutoGL_SetColor(0.9, 0.9, 0.7);
}
//----------------------------------------------------------------------
void GLColor::setVehicleAccessible()
{
  AutoGL_SetColor(1, 0.7, 0.8);
}
//----------------------------------------------------------------------
void GLColor::setPedestrianAccessible()
{
  AutoGL_SetColor(0.7, 0.8, 1);
}
//----------------------------------------------------------------------
void GLColor::setAnyTrafficAccessible()
{
  AutoGL_SetColor(0.8, 0.8, 0.5);
}
//----------------------------------------------------------------------
void GLColor::setAnyTrafficInaccessible()
{
  AutoGL_SetColor(0.3, 0.3, 0.3);
}
//======================================================================
void GLColor::setSubnetwork()
{
  AutoGL_SetColor(1,1,1);
}
//----------------------------------------------------------------------
void GLColor::setSubsectionEdge()
{
  AutoGL_SetColor(0.8,0.6,0.6);
}
//----------------------------------------------------------------------
void GLColor::setWalkerGateway()
{
  AutoGL_SetColor(0.3,0.3,0.5);
}

//======================================================================
void GLColor::setLane()
{
  AutoGL_SetColor(1,1,1);
}
//----------------------------------------------------------------------
void GLColor::setStraightLane()
{
  AutoGL_SetColor(0.7, 1, 0.2);
}
//----------------------------------------------------------------------
void GLColor::setLeftLane()
{
  AutoGL_SetColor(1, 0.5, 0);
}
//----------------------------------------------------------------------
void GLColor::setRightLane()
{
  AutoGL_SetColor(0, 1, 1);
}
//----------------------------------------------------------------------
void GLColor::setUpLane()
{
  AutoGL_SetColor(1, 1, 0);
}
//----------------------------------------------------------------------
void GLColor::setDownLane()
{
  AutoGL_SetColor(1, 0.8, 0);
}

//======================================================================
void GLColor::setBorder()
{
  AutoGL_SetColor(1, 1, 1);
}
//----------------------------------------------------------------------
void GLColor::setInPoint()
{
  AutoGL_SetColor(0, 0.5, 1);
}
//----------------------------------------------------------------------
void GLColor::setOutPoint()
{
  AutoGL_SetColor(0, 1, 0.5);
}
//----------------------------------------------------------------------
void GLColor::setDetector()
{
  AutoGL_SetColor(0, 0.5, 0.7);
}
//----------------------------------------------------------------------
void GLColor::setInterId()
{
  AutoGL_SetColor(0, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setCSId()
{
  AutoGL_SetColor(1, 1, 0);
}
//----------------------------------------------------------------------
void GLColor::setCSValue()
{
  AutoGL_SetColor(1.0, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setLaneId()
{
  AutoGL_SetColor(1, 0.5, 0.5);
}
//----------------------------------------------------------------------
void GLColor::setSubsectionId()
{
  AutoGL_SetColor(1, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setRoadsideUnitId()
{
  AutoGL_SetColor(0, 0, 1);
}

//######################################################################
void GLColor::setBlueSignal()
{
  AutoGL_SetColor(0, 0, 1);
}
//----------------------------------------------------------------------
void GLColor::setRedSignal()
{
  AutoGL_SetColor(1, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setYellowSignal()
{
  AutoGL_SetColor(1, 1, 0);
}
//----------------------------------------------------------------------
void GLColor::setNoneSignal()
{
  AutoGL_SetColor(0, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setAllSignal()
{
  AutoGL_SetColor(1, 1, 1);
}
//----------------------------------------------------------------------
void GLColor::setStraightSignal()
{
  AutoGL_SetColor(1, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setLeftSignal()
{
  AutoGL_SetColor(0, 1, 0);
}
//----------------------------------------------------------------------
void GLColor::setRightSignal()
{
  AutoGL_SetColor(0, 0, 1);
}
//----------------------------------------------------------------------
void GLColor::setStraightLeftSignal()
{
  AutoGL_SetColor(1, 1, 0);
}
//----------------------------------------------------------------------
void GLColor::setStraightRightSignal()
{
  AutoGL_SetColor(1, 0, 1);
}
//----------------------------------------------------------------------
void GLColor::setLeftRightSignal()
{
  AutoGL_SetColor(0, 1, 1);
}

//######################################################################
void GLColor::setVehicle()
{
  AutoGL_SetColor(1, 0, 0);
}
//----------------------------------------------------------------------
void GLColor::setSleepingVehicle()
{
  AutoGL_SetColor(1, 1, 1);
}
//----------------------------------------------------------------------
void GLColor::setTruck()
{
  AutoGL_SetColor(0.3, 0.7, 1);
}
//----------------------------------------------------------------------
void GLColor::setBus()
{
  AutoGL_SetColor(0, 0.8, 0.5);
}
//----------------------------------------------------------------------
void GLColor::setVehicleId(){
  AutoGL_SetColor(0, 0, 0);
}

