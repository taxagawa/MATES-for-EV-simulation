/* **************************************************
 * Copyright (C) 2014 ADVENTURE Project
 * All Rights Reserved
 **************************************************** */
#ifndef __DRAWER_H__
#define __DRAWER_H__

class RoadEntity;
class Lane;
class Signal;
class Intersection;
class Section;
class RoadMap;

class DetectorUnit;
class Vehicle;

/**
 * @addtogroup Drawing
 * @brief AutoGLを用いた描画
 * @ingroup Visualization
 */

//######################################################################
/// RoadEntity（サブセクション）を描画するクラス
/**
 * @ingroup Drawing
 */
class RoadEntityDrawer
{
public:
    static RoadEntityDrawer& instance();
    virtual void draw(const RoadEntity& entity) const;
protected:
    RoadEntityDrawer(){};
    RoadEntityDrawer(const RoadEntityDrawer&){};
    virtual ~RoadEntityDrawer(){};

    /// サブセクションの形状を描画する
    virtual void drawShape(const RoadEntity& entity) const;

    /// サブネットワークを表示する
    virtual void drawSubnetwork(const RoadEntity& entity,
                                double height,
                                double width) const;

    /// 信号を描画する
    virtual void drawSignals(const RoadEntity& entity) const;

    /// IDを表示する
    virtual void drawId(const RoadEntity& entity) const;
};

//######################################################################
/// Laneを描画するクラス
/**
 * @ingroup Drawing
 */
class LaneDrawer
{
public:
    static LaneDrawer& instance();
    virtual void draw(const Lane& lane, double height, double width) const;
    virtual void drawId(const Lane& lane) const;
protected:
    LaneDrawer(){};
    LaneDrawer(const LaneDrawer&){};
    virtual ~LaneDrawer(){};
};

//######################################################################
/// Signalを描画するクラス
/**
 * @ingroup Drawing
 */
class SignalDrawer
{
public:
    static SignalDrawer& instance();
    virtual void draw(const Intersection& inter) const;
protected:
    SignalDrawer(){};
    SignalDrawer(const SignalDrawer&){};
    virtual ~SignalDrawer(){};
};

//######################################################################
/// Intersectionを描画するクラス
/**
 * @ingroup Drawing
 */
class IntersectionDrawer
{
public:
    static IntersectionDrawer& instance();
    virtual void draw(const Intersection& inter) const;
    virtual void drawSimple(const Intersection& inter,
                            double radius) const;

protected:
    IntersectionDrawer(){};
    IntersectionDrawer(const IntersectionDrawer&){};
    virtual ~IntersectionDrawer(){};

    /// サブセクションを描画する
    virtual void drawSubsections(const Intersection& inter) const;
    /// 内部レーンを描画する
    virtual void drawLanes(const Intersection& inter) const;

    /// コネクタを描画する
    virtual void drawConnectors(const Intersection& inter) const;

    /// 信号を描画する
    virtual void drawSignals(const Intersection& inter) const;
    /// IDを表示する
    virtual void drawId(const Intersection& inter) const;

    // by uchida 2016/5/23
    /// CSのIDを表示する
    virtual void drawCSId(const Intersection& inter) const;

    // by uchida 2016/5/23
    /// CSの充電電力量を表示する
    virtual void drawChargeValue(const Intersection& inter) const;
};

//######################################################################
/// Sectionを描画するクラス
/**
 * @ingroup Drawing
 */
class SectionDrawer
{
public:
    static SectionDrawer& instance();
    virtual void draw(const Section& section) const;
    virtual void drawSimple(const Section& section,
                            double width) const;
protected:
    SectionDrawer(){};
    SectionDrawer(const SectionDrawer&){};
    virtual ~SectionDrawer(){};

    /// サブセクションを描画する
    virtual void drawSubsections(const Section& section) const;
    /// 内部レーンを描画する
    virtual void drawLanes(const Section& section) const;

    /// コネクタを描画する
    virtual void drawConnectors(const Section& section) const;
};

//######################################################################
/// RoadMapを描画するクラス
/**
 * @ingroup Drawing
 */
class RoadMapDrawer
{
public:
    static RoadMapDrawer& instance();
    virtual void draw(const RoadMap& roadMap) const;

protected:
    RoadMapDrawer();
    RoadMapDrawer(const RoadMapDrawer&){};
    virtual ~RoadMapDrawer(){};

    const IntersectionDrawer* _intersectionDrawer;
    const SectionDrawer* _sectionDrawer;
};

//######################################################################
/// Detectorを描画するクラス
/**
 * @ingroup Drawing
 */
class DetectorDrawer
{
public:
    static DetectorDrawer& instance();
    virtual void draw(const DetectorUnit& unit) const;

protected:
    DetectorDrawer(){};
    DetectorDrawer(const DetectorDrawer&){};
    virtual ~DetectorDrawer(){};
};

//######################################################################
/// Vehicleを描画するクラス
/**
 * @ingroup Drawing
 */
class VehicleDrawer
{
public:
    static VehicleDrawer& instance();
    virtual void draw(const Vehicle& vehicle) const;
    virtual void drawSimple(const Vehicle& vehicle,
                            double size) const;
protected:
    VehicleDrawer(){};
    VehicleDrawer(const VehicleDrawer&){};
    virtual ~VehicleDrawer(){};
};

#endif //__DRAWER_H__
