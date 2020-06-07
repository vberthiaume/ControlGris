/**************************************************************************
 * Copyright 2018 UdeM - GRIS - Olivier Belanger                          *
 *                                                                        *
 * This file is part of ControlGris, a multi-mSource spatialization plugin *
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
#include "AutomationManager.h"

#include <array>

#include <algorithm>

constexpr auto MAGIC_4 = 200;
constexpr auto MAGIC_6 = 199.0f;

AutomationManager::AutomationManager()
{
    mCurrentTrajectoryPoint = Point<float>{ mFieldWidth / 2.0f, mFieldWidth / 2.0f };
    mSource.setX(0.0f);
    mSource.setY(0.0f);
}

void AutomationManager::setFieldWidth(float const newFieldWidth)
{
    float const factor{ newFieldWidth / mFieldWidth };
    mFieldWidth = newFieldWidth;
    AffineTransform const t{ AffineTransform::scale(factor) };
    for (auto & p : mTrajectoryPoints) {
        p.applyTransform(t);
    }
}

void AutomationManager::setPositionActivateState(bool const newState)
{
    mActivateState = newState;
    if (newState == false) {
        mPlaybackPosition = Point<float>{ INVALID_POSITION };
    } else {
        mTrajectoryDeltaTime = 0.0;
        mLastTrajectoryDeltaTime = 0.0;
        mBackAndForthDirection = Direction::forward;
        mDampeningCycleCount = 0;
        mDampeningLastDelta = 0.0;
        mCurrentPlaybackDuration = mPlaybackDuration;
        mCurrentDegreeOfDeviation = 0.0;
        mDeviationCycleCount = 0;
    }
}

void AutomationManager::resetRecordingTrajectory(Point<float> const currentPosition)
{
    mPlaybackPosition = Point<float>{ INVALID_POSITION };
    mTrajectoryPoints.clear();
    mTrajectoryPoints.add(currentPosition);
    mLastRecordingPoint = currentPosition;
    setSourcePosition(Point<float>{ currentPosition.x / mFieldWidth, 1.0f - currentPosition.y / mFieldWidth });
}

Point<float> AutomationManager::smoothRecordingPosition(Point<float> const & pos)
{
    constexpr auto smoothingFactor = 0.8f;

    mLastRecordingPoint = (mLastRecordingPoint - pos) * smoothingFactor + pos;
    return mLastRecordingPoint;
}

void AutomationManager::createRecordingPath(Path & path)
{
    path.startNewSubPath(mTrajectoryPoints[0]);
    for (int i{ 1 }; i < mTrajectoryPoints.size(); ++i) {
        path.lineTo(mTrajectoryPoints[i]);
    }
}

void AutomationManager::setTrajectoryDeltaTime(double const relativeTimeFromPlay)
{
    mTrajectoryDeltaTime = relativeTimeFromPlay / mCurrentPlaybackDuration;
    mTrajectoryDeltaTime = std::fmod(mTrajectoryDeltaTime, 1.0f);
    computeCurrentTrajectoryPoint();
}

void AutomationManager::compressTrajectoryXValues(int maxValue) // TODO: make this function useless asap
{
    auto const offset{ static_cast<int>(10.0f + SOURCE_FIELD_COMPONENT_RADIUS) };
    maxValue -= offset;
    int const size{ getRecordingTrajectorySize() };
    auto const delta{ static_cast<float>(maxValue) / (static_cast<float>(size) + 1.0f) };
    for (int i{}; i < size; ++i) {
        mTrajectoryPoints.data()[i].setX(static_cast<float>(i) * delta + offset);
    }
}

void AutomationManager::computeCurrentTrajectoryPoint()
{
    int const dampeningCyclesTimes2{ mDampeningCycles * 2 };
    double currentScaleMin{ 0.0 };
    double currentScaleMax{ 0.0 };

    if (mTrajectoryPoints.size() > 0) {
        if (mTrajectoryDeltaTime < mLastTrajectoryDeltaTime) {
            if (mIsBackAndForth) {
                this->invertBackAndForthDirection();
                mDampeningCycleCount++;
                if (mDampeningCycleCount >= dampeningCyclesTimes2) {
                    mDampeningCycleCount = dampeningCyclesTimes2;
                }
            }
            mDeviationCycleCount++;
        }
        mLastTrajectoryDeltaTime = mTrajectoryDeltaTime;

        double trajectoryPhase;
        if (mIsBackAndForth && mDampeningCycles > 0) {
            if (mTrajectoryDeltaTime <= 0.5) {
                trajectoryPhase = std::pow(mTrajectoryDeltaTime * 2.0, 2.0) * 0.5;
            } else {
                trajectoryPhase = 1.0 - std::pow(1.0 - ((mTrajectoryDeltaTime - 0.5) * 2.0), 2.0) * 0.5;
            }
        } else {
            trajectoryPhase = mTrajectoryDeltaTime;
        }

        double delta{ trajectoryPhase * mTrajectoryPoints.size() };

        if (mBackAndForthDirection == Direction::backward) {
            delta = mTrajectoryPoints.size() - delta;
        }

        delta = std::clamp(delta, 0.0, static_cast<double>(mTrajectoryPoints.size()));

        if (mIsBackAndForth && mDampeningCycles > 0) {
            if (mDampeningCycleCount < dampeningCyclesTimes2) {
                double const relativeDeltaTime{ (mDampeningCycleCount + mTrajectoryDeltaTime) / dampeningCyclesTimes2 };
                mCurrentPlaybackDuration
                    = mPlaybackDuration - (std::pow(relativeDeltaTime, 2.0) * mPlaybackDuration * 0.25);
                currentScaleMin = relativeDeltaTime * mTrajectoryPoints.size() * 0.5;
                currentScaleMax = mTrajectoryPoints.size() - currentScaleMin;
                double const currentScale{ (currentScaleMax - currentScaleMin) / mTrajectoryPoints.size() };
                delta = delta * currentScale + currentScaleMin;
                mDampeningLastDelta = delta;
            } else {
                delta = mDampeningLastDelta;
            }
        } else {
            mDampeningLastDelta = delta;
        }

        double const deltaRatio{ static_cast<double>(mTrajectoryPoints.size() - 1) / mTrajectoryPoints.size() };
        delta *= deltaRatio;
        auto index = static_cast<int>(delta);
        if (index + 1 < mTrajectoryPoints.size()) {
            auto const frac{ static_cast<float>(delta) - static_cast<float>(index) };
            auto const & p1{ mTrajectoryPoints.getReference(index) };
            auto const & p2{ mTrajectoryPoints.getReference(index + 1) };
            mCurrentTrajectoryPoint = p1 + (p2 - p1) * frac;
        } else {
            mCurrentTrajectoryPoint = mTrajectoryPoints.getLast();
        }
    }

    if (mDegreeOfDeviationPerCycle != 0) {
        bool deviationFlag{ true };
        if (mIsBackAndForth && mDampeningCycles > 0) {
            if (approximatelyEqual(currentScaleMin, currentScaleMax)) {
                deviationFlag = false;
            }
        }
        if (deviationFlag) {
            mCurrentDegreeOfDeviation
                = static_cast<float>(mDeviationCycleCount + mTrajectoryDeltaTime) * mDegreeOfDeviationPerCycle;
            if (mCurrentDegreeOfDeviation >= 360.0f) {
                mCurrentDegreeOfDeviation -= 360.0f;
            }
        }
        AffineTransform const t{ AffineTransform::rotation(mCurrentDegreeOfDeviation / 360.0f
                                                               * MathConstants<float>::twoPi,
                                                           (mFieldWidth / 2.0f),
                                                           (mFieldWidth / 2.0f)) };
        mCurrentTrajectoryPoint.applyTransform(t);
    }

    if (mActivateState) {
        ;
        setSourcePosition(
            Point<float>{ mCurrentTrajectoryPoint.x / mFieldWidth, 1.0f - mCurrentTrajectoryPoint.y / mFieldWidth });
        sendTrajectoryPositionChangedEvent();
    }
}

Point<float> AutomationManager::getCurrentTrajectoryPoint() const
{
    if (mActivateState) {
        return mCurrentTrajectoryPoint;
    } else {
        return Point<float>{ getSourcePosition().x * mFieldWidth, (1.0f - getSourcePosition().y) * mFieldWidth };
    }
}

void AutomationManager::setSourceAndPlaybackPosition(Point<float> const pos)
{
    setSourcePosition(pos);
    setPlaybackPosition(pos);
}

void PositionAutomationManager::sendTrajectoryPositionChangedEvent()
{
    if (mActivateState || mTrajectoryType == PositionTrajectoryType::realtime) {
        mListeners.call([&](Listener & l) { l.trajectoryPositionChanged(this, mSource.getPos()); });
    }
}

void ElevationAutomationManager::sendTrajectoryPositionChangedEvent()
{
    if (mActivateState || mTrajectoryType == ElevationTrajectoryType::realtime) {
        mListeners.call([&](Listener & l) { l.trajectoryPositionChanged(this, mSource.getPos()); });
    }
}

void AutomationManager::fixSourcePosition()
{
    bool const shouldBeFixed{ mSourceLink != PositionSourceLink::independent };
    mSource.fixSourcePosition(shouldBeFixed);
}

void PositionAutomationManager::setTrajectoryType(PositionTrajectoryType const type, Point<float> const & startPosition)
{
    mTrajectoryType = type;

    mTrajectoryPoints.clear();

    auto const halfFieldSize{ mFieldWidth / 2.0f };
    constexpr auto TRAJECTORY_PADDING{ 8.0f };
    Range<float> const trajectoryLimits{ TRAJECTORY_PADDING, mFieldWidth - TRAJECTORY_PADDING };
    float const trajectoryMagnitude{ std::hypot(startPosition.x, startPosition.y)
                                     * ((mFieldWidth - SOURCE_FIELD_COMPONENT_DIAMETER) / 2.0f) };
    Radians const startPositionAngle{ std::atan2(startPosition.y, startPosition.x) };

    int const fSizeOver3{ static_cast<int>(mFieldWidth) / 3 };
    int const fSizeOver4{ static_cast<int>(mFieldWidth) / 4 };
    float x;
    float y;
    float transX{};
    float transY{};
    switch (mTrajectoryType) {
    case PositionTrajectoryType::realtime:
    case PositionTrajectoryType::drawing:
        mPlaybackPosition = Point<float>{ -1.0f, -1.0f };
        break;
    case PositionTrajectoryType::circleClockwise: {
        constexpr auto NB_POINTS = 200;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * i / (NB_POINTS - 1) - startPositionAngle };
            x = std::cos(adjustedAngle.getAsRadians()) * trajectoryMagnitude + halfFieldSize;
            y = std::sin(adjustedAngle.getAsRadians()) * trajectoryMagnitude + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::circleCounterClockwise: {
        constexpr auto NB_POINTS = 200;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * -i / (NB_POINTS - 1) - startPositionAngle };
            x = std::cos(adjustedAngle.getAsRadians()) * trajectoryMagnitude + halfFieldSize;
            y = std::sin(adjustedAngle.getAsRadians()) * trajectoryMagnitude + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::ellipseClockwise: {
        constexpr auto NB_POINTS = 200;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * i / (NB_POINTS - 1) };
            x = std::cos(adjustedAngle.getAsRadians()) * trajectoryMagnitude;
            y = std::sin(adjustedAngle.getAsRadians()) * trajectoryMagnitude * 0.5f;
            auto const mag{ std::hypot(x, y) };
            auto const ang{ std::atan2(y, x) };
            x = mag * std::cos(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            y = mag * std::sin(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::ellipseCounterClockwise: {
        constexpr auto NB_POINTS = 200;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * -i / (NB_POINTS - 1) };
            x = std::cos(adjustedAngle.getAsRadians()) * trajectoryMagnitude * 0.5f;
            y = std::sin(adjustedAngle.getAsRadians()) * trajectoryMagnitude;
            auto const mag{ std::hypot(x, y) };
            auto const ang{ std::atan2(y, x) };
            x = mag * std::cos(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            y = mag * std::sin(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::spiralClockwiseOutIn: {
        constexpr auto NB_POINTS = 300;
        constexpr auto NB_POINTS_PER_REVOLUTION = NB_POINTS / 3;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * i / NB_POINTS_PER_REVOLUTION };
            auto const adjustedMagnitude{ trajectoryMagnitude * (1.0f - static_cast<float>(i) / NB_POINTS) };
            x = std::cos(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            y = std::sin(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            auto const mag{ std::hypot(x, y) };
            auto const ang{ std::atan2(y, x) };
            x = mag * std::cos(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            y = mag * std::sin(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::spiralCounterClockwiseOutIn: {
        constexpr auto NB_POINTS = 300;
        constexpr auto NB_POINTS_PER_REVOLUTION = NB_POINTS / 3;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * -i / NB_POINTS_PER_REVOLUTION };
            auto const adjustedMagnitude{ trajectoryMagnitude * (1.0f - static_cast<float>(i) / NB_POINTS) };
            x = std::cos(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            y = std::sin(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            auto const mag{ std::hypot(x, y) };
            auto const ang{ std::atan2(y, x) };
            x = mag * std::cos(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            y = mag * std::sin(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::spiralClockwiseInOut: {
        constexpr auto NB_POINTS = 300;
        constexpr auto NB_POINTS_PER_REVOLUTION = NB_POINTS / 3;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * i / NB_POINTS_PER_REVOLUTION };
            auto const adjustedMagnitude{ trajectoryMagnitude * (static_cast<float>(i) / NB_POINTS) };
            x = std::cos(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            y = std::sin(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            float const mag{ std::hypot(x, y) };
            float const ang{ std::atan2(y, x) };
            x = mag * std::cos(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            y = mag * std::sin(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::spiralCounterClockwiseInOut: {
        constexpr auto NB_POINTS = 300;
        constexpr auto NB_POINTS_PER_REVOLUTION = NB_POINTS / 3;
        for (int i{}; i < NB_POINTS; ++i) {
            auto const adjustedAngle{ twoPi * -i / NB_POINTS_PER_REVOLUTION };
            auto const adjustedMagnitude{ trajectoryMagnitude * (static_cast<float>(i) / NB_POINTS) };
            x = std::cos(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            y = std::sin(adjustedAngle.getAsRadians()) * adjustedMagnitude;
            float const mag{ std::hypot(x, y) };
            float const ang{ std::atan2(y, x) };
            x = mag * std::cos(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            y = mag * std::sin(ang + startPositionAngle.getAsRadians()) + halfFieldSize;
            x = trajectoryLimits.clipValue(x);
            y = trajectoryLimits.clipValue(y);
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    }
    case PositionTrajectoryType::squareClockwise:
    case PositionTrajectoryType::squareCounterClockwise: {
        constexpr auto NB_POINTS = 300;
        static_assert(NB_POINTS % 4 == 0); // much simpler that way

        constexpr auto sideLength{ MathConstants<float>::sqrt2 };
        constexpr auto step{ sideLength / (static_cast<float>(NB_POINTS) / 4.0f) };

        auto currentPosition{ Point<float>{ -sideLength / 2.0f, -sideLength / 2.0f } };

        // top side
        for (int i{}; i < NB_POINTS / 4; ++i) {
            mTrajectoryPoints.add(currentPosition);
            currentPosition.setX(currentPosition.getX() + step);
        }
        // right side
        for (int i{}; i < NB_POINTS / 4; ++i) {
            mTrajectoryPoints.add(currentPosition);
            currentPosition.setY(currentPosition.getY() + step);
        }
        // bottom side
        for (int i{}; i < NB_POINTS / 4; ++i) {
            mTrajectoryPoints.add(currentPosition);
            currentPosition.setX(currentPosition.getX() - step);
        }
        // left side
        for (int i{}; i < NB_POINTS / 4; ++i) {
            mTrajectoryPoints.add(currentPosition);
            currentPosition.setY(currentPosition.getY() - step);
        }

        // rotate, scale, translate and clip
        auto const rotationAngle{ (startPositionAngle - Degrees{ 45.0f }).getAsRadians() };
        for (auto & point : mTrajectoryPoints) {
            auto const rotated{ point.rotatedAboutOrigin(rotationAngle) };
            auto const scaled{ rotated * trajectoryMagnitude };
            auto const translated{ scaled + Point<float>{ halfFieldSize, halfFieldSize } };
            Point<float> const clipped{ trajectoryLimits.clipValue(translated.getX()),
                                        trajectoryLimits.clipValue(translated.getY()) };
            point = clipped;
        }
        break;
    }
    case PositionTrajectoryType::triangleClockwise:
    case PositionTrajectoryType::triangleCounterClockwise: {
        Point<float> const p1{ 0.0f, -1.0f };
        Point<float> p2{};
        Point<float> p3{};
        if (mTrajectoryType == PositionTrajectoryType::triangleClockwise) {
            p2 = Point<float>{ 1.0f, 1.0f };
            p3 = Point<float>{ -1.0f, 1.0f };
        } else {
            p2 = Point<float>{ -1.0f, 1.0f };
            p3 = Point<float>{ 1.0f, 1.0f };
        }
        auto const step{ 1.0f / (mFieldWidth / 3.0f) };
        transX = startPosition.x * ((mFieldWidth / 2.0f - SOURCE_FIELD_COMPONENT_RADIUS));
        transY = startPosition.y * ((mFieldWidth / 2.0f - SOURCE_FIELD_COMPONENT_RADIUS));
        auto const magnitude{ std::hypot(transX, transY) };
        for (int i{}; i < static_cast<int>(mFieldWidth); ++i) {
            if (i < (fSizeOver3)) {
                x = (p2.x - p1.x) * static_cast<float>(i) * step + p1.x;
                y = (p2.y - p1.y) * static_cast<float>(i) * step + p1.y;
            } else if (i < (fSizeOver3 * 2)) {
                x = (p3.x - p2.x) * static_cast<float>(i - fSizeOver3) * step + p2.x;
                y = (p3.y - p2.y) * static_cast<float>(i - fSizeOver3) * step + p2.y;
            } else {
                x = (p1.x - p3.x) * static_cast<float>(i - fSizeOver3 * 2) * step + p3.x;
                y = (p1.y - p3.y) * static_cast<float>(i - fSizeOver3 * 2) * step + p3.y;
            }
            Point<float> p(x * magnitude + (mFieldWidth / 2.0f), y * magnitude + (mFieldWidth / 2.0f));
            p.applyTransform(AffineTransform::rotation(-startPositionAngle.getAsRadians(),
                                                       (mFieldWidth / 2.0f),
                                                       (mFieldWidth / 2.0f)));
            p.x = trajectoryLimits.clipValue(p.x);
            p.y = trajectoryLimits.clipValue(p.y);
            mTrajectoryPoints.add(p);
        }
    } break;
    case PositionTrajectoryType::undefined:
        jassertfalse;
    }

    if (mTrajectoryType > PositionTrajectoryType::drawing) {
        setSourcePosition(
            Point<float>{ mTrajectoryPoints[0].x / mFieldWidth, 1.0f - mTrajectoryPoints[0].y / mFieldWidth });
    } else {
        setSourcePosition(Point<float>{ 0.5f, 0.5f });
    }
}

void ElevationAutomationManager::setTrajectoryType(ElevationTrajectoryType const type)
{
    mTrajectoryType = type;

    mTrajectoryPoints.clear();

    auto constexpr offset{ 10.0f + SOURCE_FIELD_COMPONENT_RADIUS };
    auto const width{ mFieldWidth - offset };
    auto constexpr minPos{ 15.0f };
    auto const maxPos{ mFieldWidth - 20.0f };

    switch (type) {
    case ElevationTrajectoryType::realtime:
    case ElevationTrajectoryType::drawing:
        mPlaybackPosition = Point<float>{ -1.0f, -1.0f };
        break;
    case ElevationTrajectoryType::downUp:
        for (int i{}; i < MAGIC_4; ++i) {
            float const x = (static_cast<float>(i) / MAGIC_6) * width + offset;
            float const y = (static_cast<float>(i) / MAGIC_6) * (maxPos - minPos) + minPos;
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    case ElevationTrajectoryType::upDown:
        for (int i{}; i < MAGIC_4; ++i) {
            float const x = (static_cast<float>(i) / MAGIC_6) * width + offset;
            float const y = (1.0f - static_cast<float>(i) / MAGIC_6) * (maxPos - minPos) + minPos;
            mTrajectoryPoints.add(Point<float>{ x, y });
        }
        break;
    case ElevationTrajectoryType::undefined:
        jassertfalse;
    }

    if (type > ElevationTrajectoryType::drawing) {
        setSourcePosition(
            Point<float>{ mTrajectoryPoints[0].x / mFieldWidth, 1.0f - mTrajectoryPoints[0].y / mFieldWidth });
    } else {
        setSourcePosition(Point<float>{ 0.5f, 0.5f });
    }
}
