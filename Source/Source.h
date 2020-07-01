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

#include <optional>

#include "../JuceLibraryCode/JuceHeader.h"

#include "ConstrainedStrongTypes.h"
#include "ControlGrisConstants.h"

//==============================================================================
enum class SourceParameter { azimuth, elevation, distance, x, y, azimuthSpan, elevationSpan };
enum class SourceLinkNotification { notify, silent };
//==============================================================================
class Source
{
public:
    //==============================================================================
    class SourceChangeBroadcaster : public juce::ChangeBroadcaster
    {
    private:
        Source & mSource;

    public:
        SourceChangeBroadcaster(Source & source) noexcept : mSource(source) {}
        ~SourceChangeBroadcaster() noexcept = default;

        Source & getSource() { return mSource; }
        Source const & getSource() const { return mSource; }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceChangeBroadcaster);
    };

private:
    //==============================================================================
    SourceChangeBroadcaster mGuiChangeBroadcaster{ *this };
    SourceChangeBroadcaster mSourceLinkChangeBroadcaster{ *this };
    SourceIndex mIndex{};
    SourceId mId{ 1 };
    SpatMode mSpatMode{ SpatMode::cube };

    Radians mAzimuth{};
    Radians mElevation{};
    float mDistance{ 1.0f };

    Point<float> mPosition{};

    Normalized mAzimuthSpan{};
    Normalized mElevationSpan{};

    Colour mColour{ Colours::black };

public:
    //==============================================================================
    Source() noexcept = default;
    ~Source() noexcept = default;
    //==============================================================================
    void setIndex(SourceIndex const index) { mIndex = index; }
    SourceIndex getIndex() const { return mIndex; }

    void setId(SourceId const id) { mId = id; }
    SourceId getId() const { return mId; }

    void setSpatMode(SpatMode const spatMode) { mSpatMode = spatMode; }
    SpatMode getSpatMode() const { return mSpatMode; }

    void setAzimuth(Radians azimuth, SourceLinkNotification sourceLinkNotification);
    void setAzimuth(Normalized azimuth, SourceLinkNotification sourceLinkNotification);
    Radians getAzimuth() const { return mAzimuth; }
    Normalized getNormalizedAzimuth() const;

    void setElevation(Radians elevation, SourceLinkNotification sourceLinkNotification);
    void setElevation(Normalized elevation, SourceLinkNotification sourceLinkNotification);
    Radians getElevation() const { return mElevation; }
    Normalized getNormalizedElevation() const;

    void setDistance(float distance, SourceLinkNotification sourceLinkNotification);
    float getDistance() const { return mDistance; }
    void setAzimuthSpan(Normalized azimuthSpan);
    Normalized getAzimuthSpan() const { return mAzimuthSpan; }
    void setElevationSpan(Normalized elevationSpan);
    Normalized getElevationSpan() const { return mElevationSpan; }

    void setCoordinates(Radians azimuth,
                        Radians elevation,
                        float distance,
                        SourceLinkNotification sourceLinkNotification);
    bool isPrimarySource() const { return mIndex == SourceIndex{ 0 }; }

    void setX(float x, SourceLinkNotification sourceLinkNotification);
    void setX(Normalized x, SourceLinkNotification sourceLinkNotification);
    void setY(Normalized y, SourceLinkNotification sourceLinkNotification);
    float getX() const { return mPosition.getX(); }
    void setY(float y, SourceLinkNotification sourceLinkNotification);
    float getY() const { return mPosition.getY(); }
    Point<float> const & getPos() const { return mPosition; }
    void setPos(Point<float> const & pos, SourceLinkNotification sourceLinkNotification);

    void computeXY();
    void computeAzimuthElevation();

    void setColorFromIndex(int numTotalSources);
    Colour getColour() const { return mColour; }

    void addGuiChangeListener(ChangeListener * listener) { mGuiChangeBroadcaster.addChangeListener(listener); }
    void removeGuiChangeListener(ChangeListener * listener) { mGuiChangeBroadcaster.removeChangeListener(listener); }
    void addSourceLinkListener(ChangeListener * listener) { mSourceLinkChangeBroadcaster.addChangeListener(listener); }
    void removeSourceLinkListener(ChangeListener * listener)
    {
        mSourceLinkChangeBroadcaster.removeChangeListener(listener);
    }

    static Point<float> getPositionFromAngle(Radians const angle, float radius);
    static Radians getAngleFromPosition(Point<float> const & position);

    static Point<float> clipPosition(Point<float> const & position, SpatMode spatMode);
    static Point<float> clipDomePosition(Point<float> const & position);
    static Point<float> clipCubePosition(Point<float> const & position);

private:
    void sendNotifications(SourceLinkNotification sourceLinkNotification);
    static Radians clipElevation(Radians elevation);
    static float clipCoordinate(float coord);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Source);
};

class Sources
{
private:
    struct Iterator {
        Sources * sources;
        int index;

        bool operator!=(Iterator const & other) const { return index != other.index; }
        Iterator & operator++()
        {
            ++index;
            return *this;
        }
        Source & operator*() { return sources->get(index); }
        Source const & operator*() const { return sources->get(index); }
    };
    struct ConstIterator {
        Sources const * sources;
        int index;

        bool operator!=(ConstIterator const & other) const { return index != other.index; }
        ConstIterator & operator++()
        {
            ++index;
            return *this;
        }
        Source const & operator*() const { return sources->get(index); }
    };

    int mSize{ 2 };
    Source mPrimarySource{};
    std::array<Source, MAX_NUMBER_OF_SOURCES - 1> mSecondarySources{};

public:
    Sources() noexcept { initIndexes(); }
    ~Sources() noexcept = default;

    int size() const { return mSize; }
    void setSize(int const size);

    Source & get(int const index)
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[index - 1];
    }
    Source const & get(int const index) const
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[index - 1];
    }
    Source & get(SourceIndex const index) { return get(index.toInt()); }
    Source const & get(SourceIndex const index) const { return get(index.toInt()); }
    Source & operator[](int const index)
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[index - 1];
    }
    Source const & operator[](int const index) const
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[index - 1];
    }
    Source & operator[](SourceIndex const index) { return (*this)[index.toInt()]; }
    Source const & operator[](SourceIndex const index) const { return (*this)[index.toInt()]; }

    void initIndexes()
    {
        SourceIndex currentIndex{};
        mPrimarySource.setIndex(currentIndex++);
        for (auto & secondarySource : mSecondarySources) {
            secondarySource.setIndex(currentIndex++);
        }
    }

    Source & getPrimarySource() { return mPrimarySource; }
    Source const & getPrimarySource() const { return mPrimarySource; }
    auto & getSecondarySources() { return mSecondarySources; }
    auto const & getSecondarySources() const { return mSecondarySources; }

    Iterator begin() { return Iterator{ this, 0 }; }
    ConstIterator begin() const { return ConstIterator{ this, 0 }; }
    Iterator end() { return Iterator{ this, mSize }; }
    ConstIterator end() const { return ConstIterator{ this, mSize }; }

    // private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sources);
};