/**************************************************************************
 * Copyright 2018 UdeM - GRIS - Olivier Belanger                          *
 *                                                                        *
 * This file is part of ControlGris, a multi-source spatialization plugin *
 *                                                                        *
 * ControlGris is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * ControlGris is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with ControlGris.  If not, see                           *
 * <http://www.gnu.org/licenses/>.                                        *
 *************************************************************************/
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "AutomationManager.h"
#include "GrisLookAndFeel.h"
#include "SettingsBoxComponent.h"
#include "Source.h"

// This file defines the classes that implement the 2D view (azimuth-elevation
// or azimuth-distance) and the elevation view.
//
// Classes:
//   FieldComponent : The base class. Implements the common attributes and
//                    methods of the two views.
//   MainFieldComponent : The 2D view as a cartesian plane. It is used to show
//                        and control two parameters, azimuth-elevation for the
//                        VBAP algorithm and azimuth-distance for the LBAP
//                        algorithm.
//   ElevationFieldComponent : The 1D view used to show and control the elevation
//                             parameter for the LBAP algorithm.

//==============================================================================
class FieldComponent : public Component
{
public:
    FieldComponent() = default;
    ~FieldComponent() override { setLookAndFeel(nullptr); }

    void drawFieldBackground(Graphics & g, bool isMainField, SpatMode spatMode = SpatMode::VBAP) const;

    Point<float> posToXy(Point<float> const & p, int p_iFieldWidth) const;
    Point<float> xyToPos(Point<float> const & p, int p_iFieldWidth) const;

    void setSources(Source * sources, int numberOfSources);
    void setSelectedSource(int selectedId);

    void setIsPlaying(bool const state) { mIsPlaying = state; }

    struct Listener {
        virtual ~Listener() = default;

        virtual void fieldSourcePositionChanged(int sourceId, int whichField) = 0;
        virtual void fieldTrajectoryHandleClicked(int whichField) = 0;
    };

    void addListener(Listener * l) { listeners.add(l); }
    void removeListener(Listener * l) { listeners.remove(l); }

    ListenerList<Listener> listeners{};

protected:
    static constexpr Point<float> INVALID_POINT{ -1.f, -1.f };
    static constexpr int NO_SELECTION_SOURCE_ID = -2;

    Source * mSources{};

    bool mIsPlaying{ false };
    int mNumberOfSources{};
    int mSelectedSourceId{};
    int mOldSelectedSourceId{};

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldComponent)
};

//==============================================================================
class MainFieldComponent final : public FieldComponent
{
public:
    MainFieldComponent(AutomationManager & automan);
    ~MainFieldComponent() final = default;

    void createSpanPathVBAP(Graphics & g, int i) const;
    void createSpanPathLBAP(Graphics & g, int i) const;
    void drawTrajectoryHandle(Graphics &) const;
    void paint(Graphics & g) final;

    bool isTrajectoryHandleClicked(MouseEvent const & event); // TODO: this should be const
    void mouseDown(MouseEvent const & event) final;
    void mouseDrag(MouseEvent const & event) final;
    void mouseMove(MouseEvent const & event) final;
    void mouseUp(MouseEvent const & event) final;

    void setSpatMode(SpatMode spatMode);

private:
    AutomationManager & mAutomationManager;

    Point<float> degreeToXy(Point<float> const & p, int p_iFieldWidth) const;
    Point<float> xyToDegree(Point<float> const & p, int p_iFieldWidth) const;

    Point<int> clipRecordingPosition(Point<int> const & pos);

    bool hasValidLineDrawingAnchor1() const { return mLineDrawingAnchor1 != INVALID_POINT; }
    bool hasValidLineDrawingAnchor2() const { return mLineDrawingAnchor2 != INVALID_POINT; }

    SpatMode mSpatMode;

    bool mShowCircularSourceSelectionWarning{ false };

    Point<float> mLineDrawingAnchor1;
    Point<float> mLineDrawingAnchor2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainFieldComponent)
};

//==============================================================================
class ElevationFieldComponent final : public FieldComponent
{
public:
    ElevationFieldComponent(AutomationManager & automationManager) : mAutomationManager(automationManager) {}
    ~ElevationFieldComponent() final = default;

    void paint(Graphics & g) final;

    void mouseDown(MouseEvent const & event) final;
    void mouseDrag(MouseEvent const & event) final;
    void mouseUp(MouseEvent const & event) final;

private:
    AutomationManager & mAutomationManager;
    int mCurrentRecordingPositionX{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElevationFieldComponent)
};
