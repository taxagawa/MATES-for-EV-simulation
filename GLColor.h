/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __GL_COLOR_H__
#define __GL_COLOR_H__

/// autogl に描画をさせる時の色の管理を行うクラス
/**
 * このクラスはインスタンスを作成しない
 *
 * @ingroup Drawing
 */
class GLColor
{
public:

    //==================================================================
    // 背景、地面など全般的なもの
    static void setBackground();
    static void setGround();
    static void setSimpleNetwork();

    //==================================================================
    // 道路構造

    // サブセクション
    // 道路表面の描画色
    static void setSubsection();
    static void setCrossWalk();
    static void setSideWalk();

    // 通行権による色分け
    static void setVehicleAccessible();
    static void setPedestrianAccessible();
    static void setAnyTrafficAccessible();
    static void setAnyTrafficInaccessible();

    // サブネットワーク
    static void setSubnetwork();
    static void setSubsectionEdge();
    static void setWalkerGateway();

    // レーン
    static void setLane();
    static void setStraightLane();
    static void setLeftLane();
    static void setRightLane();
    static void setUpLane();
    static void setDownLane();

    // ボーダー、コネクタ他
    static void setBorder();
    static void setInPoint();
    static void setOutPoint();
    static void setDetector();
    static void setInterId();
    static void setCSId();
    static void setCSValue();
    static void setLaneId();
    static void setSubsectionId();
    static void setRoadsideUnitId();

    //==================================================================
    // 信号
    static void setBlueSignal();
    static void setRedSignal();
    static void setYellowSignal();
    static void setNoneSignal();
    static void setAllSignal();
    static void setStraightSignal();
    static void setLeftSignal();
    static void setRightSignal();
    static void setStraightLeftSignal();
    static void setStraightRightSignal();
    static void setLeftRightSignal();

    //====================================================================
    // エージェント
    static void setVehicle();
    static void setSleepingVehicle();
    static void setTruck();
    static void setBus();
    static void setVehicleId();
  
private:
    GLColor();
    ~GLColor();
};

#endif //__GL_COLOR_H__
