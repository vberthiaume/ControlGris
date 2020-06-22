/*
  ==============================================================================

    SourceLinkEnforcer.cpp
    Created: 16 Jun 2020 2:21:04pm
    Author:  samuel

  ==============================================================================
*/

#include "SourceLinkEnforcer.h"

#include "ControlGrisConstants.h"

class LinkStrategy
{
    bool mInitialized{ false };

public:
    LinkStrategy() noexcept = default;
    virtual ~LinkStrategy() noexcept = default;

    LinkStrategy(LinkStrategy const &) = default;
    LinkStrategy(LinkStrategy &&) noexcept = default;

    LinkStrategy & operator=(LinkStrategy const &) = default;
    LinkStrategy & operator=(LinkStrategy &&) = default;

    void calculateParams(SourceSnapshot const & primarySourceSnapshot, int const numberOfSources)
    {
        calculateParams_impl(primarySourceSnapshot, numberOfSources);
        mInitialized = true;
    }

    void apply(Array<SourceSnapshot> & secondarySnapshots) const
    {
        for (auto & snapshot : secondarySnapshots) { // TODO: this is applied to more sources than it should
            apply(snapshot);
        }
    }

    void apply(SourceSnapshot & snapshot) const
    {
        jassert(mInitialized);
        apply_impl(snapshot);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot(SourceSnapshot const & snapshot) const
    {
        jassert(mInitialized);
        return getInversedSnapshot_impl(snapshot);
    }

private:
    virtual void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot, int numberOfSources) = 0;
    virtual void apply_impl(SourceSnapshot & snapshot) const = 0;
    [[nodiscard]] virtual SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const = 0;
};

class CircularStrategy final : public LinkStrategy
{
private:
    Radians mRotation{};
    float mRadiusRatio{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        auto const notQuiteZero{ std::nextafter(0.0f, 1.0f) };
        mRotation = primarySourceSnapshot.source->getAzimuth() - primarySourceSnapshot.azimuth;
        auto const primarySourceInitialRadius{ primarySourceSnapshot.position.getDistanceFromOrigin() };
        auto const radius{ primarySourceInitialRadius == 0.0f
                               ? notQuiteZero
                               : primarySourceSnapshot.source->getPos().getDistanceFromOrigin()
                                     / primarySourceInitialRadius };
        mRadiusRatio = radius == 0.0f ? notQuiteZero : radius;
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const newPosition{ snapshot.position.rotatedAboutOrigin(mRotation.getAsRadians()) * mRadiusRatio };
        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        SourceSnapshot result{ snapshot };
        auto const newPosition{
            (snapshot.source->getPos() / mRadiusRatio).rotatedAboutOrigin(-mRotation.getAsRadians())
        };
        result.position = newPosition;
        return result;
    }
};

class CircularFixedRadiusStrategy : public LinkStrategy
{
private:
    Radians mRotation{};
    float mRadius{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mRotation = primarySourceSnapshot.source->getAzimuth() - primarySourceSnapshot.azimuth;
        mRadius = primarySourceSnapshot.source->getPos().getDistanceFromOrigin();
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        Radians const oldAngle{ std::atan2(snapshot.position.getY(), snapshot.position.getX()) };
        auto const newAngle{ (mRotation + oldAngle).getAsRadians() };
        Point<float> const newPosition{ std::cos(newAngle) * mRadius, std::sin(newAngle) * mRadius };

        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        auto const sourcePosition{ snapshot.source->getPos() };
        SourceSnapshot result{ snapshot };

        Radians const sourceAngle{ std::atan2(sourcePosition.getY(), sourcePosition.getX()) };
        auto const inversedAngle{ (sourceAngle - mRotation).getAsRadians() };
        Point<float> const newPosition{ std::cos(inversedAngle) * mRadius, std::sin(inversedAngle) * mRadius };

        result.position = newPosition;
        return result;
    }
};

class CircularFixedAngleStrategy : public LinkStrategy
{
private:
    Radians mDeviationPerSource{};
    Radians mPrimaySourceAngle{};
    float mRadiusRatio{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot, int const numberOfSources) final
    {
        auto const notQuiteZero{ std::nextafter(0.0f, 1.0f) };
        auto const initialRadius{ std::max(primarySourceSnapshot.position.getDistanceFromOrigin(), notQuiteZero) };
        mRadiusRatio
            = std::max(primarySourceSnapshot.source->getPos().getDistanceFromOrigin() / initialRadius, notQuiteZero);

        auto const sourcePosition{ primarySourceSnapshot.source->getPos() };
        mPrimaySourceAngle = Radians{ std::atan2(sourcePosition.getY(), sourcePosition.getX()) };
        mDeviationPerSource = Degrees{ 360 } / numberOfSources;
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const sourceIndex{ snapshot.source->getIndex() };
        auto const newAngle{ mPrimaySourceAngle + mDeviationPerSource * sourceIndex.toInt() };
        auto const initialRadius{ snapshot.position.getDistanceFromOrigin() };
        auto const newRadius{ initialRadius * mRadiusRatio };
        Point<float> const newPosition{ std::cos(newAngle.getAsRadians()) * newRadius,
                                        std::sin(newAngle.getAsRadians()) * newRadius };

        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        SourceSnapshot result{ snapshot };

        auto const newRadius{ snapshot.source->getPos().getDistanceFromOrigin() / mRadiusRatio };
        Point<float> const newPosition{ newRadius, 0.0f }; // we only care about changing the radius

        result.position = newPosition;

        return result;
    }
};

class CircularFullyFixedStrategy : public LinkStrategy
{
    Radians mDeviationPerSource{};
    Radians mPrimaySourceAngle{};
    float mRadius{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot, int const numberOfSources) final
    {
        mDeviationPerSource = Degrees{ 360.0f } / numberOfSources;
        auto const primarySourcePosition{ primarySourceSnapshot.source->getPos() };
        mPrimaySourceAngle = Radians{ std::atan2(primarySourcePosition.getY(), primarySourcePosition.getX()) };
        mRadius = primarySourcePosition.getDistanceFromOrigin();
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const secondaryIndex{ snapshot.source->getIndex() };
        auto const angle{ mPrimaySourceAngle + mDeviationPerSource * secondaryIndex.toInt() };
        Point<float> newPosition{ std::cos(angle.getAsRadians()) * mRadius, std::sin(angle.getAsRadians()) * mRadius };

        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl([[maybe_unused]] SourceSnapshot const & snapshot) const final
    {
        // nothing to do here!
        return snapshot;
    }
};

class LinkSymmetricXStrategy : public LinkStrategy
{
    Point<float> mPrimaryPosition;

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mPrimaryPosition = primarySourceSnapshot.source->getPos();
    }

    void apply_impl([[maybe_unused]] SourceSnapshot & snapshot) const final
    {
        Point<float> const newPosition{ -mPrimaryPosition.getX(), mPrimaryPosition.getY() };
        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl([[maybe_unused]] SourceSnapshot const & snapshot) const final
    {
        // nothing to do here!
        return snapshot;
    }
};

class LinkSymmetricYStrategy : public LinkStrategy
{
    Point<float> mPrimaryPosition;

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mPrimaryPosition = primarySourceSnapshot.source->getPos();
    }

    void apply_impl([[maybe_unused]] SourceSnapshot & snapshot) const final
    {
        Point<float> const newPosition{ mPrimaryPosition.getX(), -mPrimaryPosition.getY() };
        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl([[maybe_unused]] SourceSnapshot const & snapshot) const final
    {
        // nothing to do here!
        return snapshot;
    }
};

class DeltaLockStrategy : public LinkStrategy
{
    Point<float> mDelta;

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mDelta = primarySourceSnapshot.source->getPos() - primarySourceSnapshot.position;
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const newPosition{ snapshot.position + mDelta };
        snapshot.source->setPos(newPosition, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        SourceSnapshot result{ snapshot };

        auto const initialPos{ snapshot.source->getPos() - mDelta };
        result.position = initialPos;

        return result;
    }
};

class FixedElevationStrategy : public LinkStrategy
{
    Radians mElevation{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mElevation = primarySourceSnapshot.source->getElevation();
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        snapshot.source->setElevation(mElevation, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        return snapshot;
    }
};

class LinearMinElevationStrategy : public LinkStrategy
{
    static constexpr Radians ELEVATION_DIFF{ -MAX_ELEVATION / 3.0f * 2.0f };
    Radians mBaseElevation{};
    Radians mElevationPerSource{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mBaseElevation = primarySourceSnapshot.source->getElevation();
        mElevationPerSource = ELEVATION_DIFF / (numberOfSources - 1);
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const sourceIndex{ snapshot.source->getIndex().toInt() };
        auto const newElevation{ mBaseElevation + mElevationPerSource * sourceIndex };
        snapshot.source->setElevation(newElevation, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        return snapshot;
    }
};

class LinearMaxElevationStrategy : public LinkStrategy
{
    static constexpr Radians ELEVATION_DIFF{ MAX_ELEVATION / 3.0f * 2.0f };
    Radians mBaseElevation{};
    Radians mElevationPerSource{};

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mBaseElevation = primarySourceSnapshot.source->getElevation();
        mElevationPerSource = ELEVATION_DIFF / (numberOfSources - 1);
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const sourceIndex{ snapshot.source->getIndex().toInt() };
        auto const newElevation{ mBaseElevation + mElevationPerSource * sourceIndex };
        snapshot.source->setElevation(newElevation, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        return snapshot;
    }
};

class DeltaLockElevationStrategy : public LinkStrategy
{
    Radians mDelta;

    void calculateParams_impl(SourceSnapshot const & primarySourceSnapshot,
                              [[maybe_unused]] int const numberOfSources) final
    {
        mDelta = primarySourceSnapshot.source->getElevation() - primarySourceSnapshot.elevation;
    }

    void apply_impl(SourceSnapshot & snapshot) const final
    {
        auto const newElevation{ snapshot.elevation + mDelta };
        snapshot.source->setElevation(newElevation, SourceLinkNotification::silent);
    }

    [[nodiscard]] SourceSnapshot getInversedSnapshot_impl(SourceSnapshot const & snapshot) const final
    {
        SourceSnapshot result{ snapshot };

        auto const initialElevation{ snapshot.source->getElevation() - mDelta };
        result.elevation = initialElevation;

        return result;
    }
};

std::unique_ptr<LinkStrategy> getLinkStrategy(AnySourceLink const sourceLink)
{
    if (std::holds_alternative<PositionSourceLink>(sourceLink)) {
        switch (std::get<PositionSourceLink>(sourceLink)) {
        case PositionSourceLink::independent:
            return nullptr;
        case PositionSourceLink::circular:
            return std::make_unique<CircularStrategy>();
        case PositionSourceLink::circularFixedRadius:
            return std::make_unique<CircularFixedRadiusStrategy>();
        case PositionSourceLink::circularFixedAngle:
            return std::make_unique<CircularFixedAngleStrategy>();
        case PositionSourceLink::circularFullyFixed:
            return std::make_unique<CircularFullyFixedStrategy>();
        case PositionSourceLink::linkSymmetricX:
            return std::make_unique<LinkSymmetricXStrategy>();
        case PositionSourceLink::linkSymmetricY:
            return std::make_unique<LinkSymmetricYStrategy>();
        case PositionSourceLink::deltaLock:
            return std::make_unique<DeltaLockStrategy>();
        case PositionSourceLink::undefined:
        default:
            jassertfalse;
        }
    } else {
        jassert(std::holds_alternative<ElevationSourceLink>(sourceLink));
        switch (std::get<ElevationSourceLink>(sourceLink)) {
        case ElevationSourceLink::independent:
            return nullptr;
        case ElevationSourceLink::fixedElevation:
            return std::make_unique<FixedElevationStrategy>();
        case ElevationSourceLink::linearMin:
            return std::make_unique<LinearMinElevationStrategy>();
        case ElevationSourceLink::linearMax:
            return std::make_unique<LinearMaxElevationStrategy>();
        case ElevationSourceLink::deltaLock:
            return std::make_unique<DeltaLockElevationStrategy>();
        case ElevationSourceLink::undefined:
        default:
            jassertfalse;
        }
    }
    jassertfalse;
    return nullptr;
}

SourceLinkEnforcer::SourceLinkEnforcer(Sources & sources, AnySourceLink const sourceLink) noexcept
    : mSources(sources)
    , mSourceLink(sourceLink)
{
    reset();
}

SourceLinkEnforcer::~SourceLinkEnforcer() noexcept
{
    for (auto & source : mSources) {
        source.removeSourceLinkListener(this);
    }
}

void SourceLinkEnforcer::setSourceLink(AnySourceLink sourceLink)
{
    if (sourceLink != mSourceLink) {
        mSourceLink = sourceLink;
        snapAll();
    }
}

void SourceLinkEnforcer::numberOfSourcesChanged()
{
    reset();
}

void SourceLinkEnforcer::primarySourceMoved()
{
    auto strategy{ getLinkStrategy(mSourceLink) };

    if (strategy != nullptr) {
        strategy->calculateParams(mPrimarySourceSnapshot, mSources.size());
        strategy->apply(mSecondarySourcesSnapshots);
    }
}

void SourceLinkEnforcer::secondarySourceMoved(SourceIndex const sourceIndex)
{
    jassert(sourceIndex.toInt() > 0 && sourceIndex.toInt() < MAX_NUMBER_OF_SOURCES);
    auto const secondaryIndex{ sourceIndex.toInt() - 1 };
    auto & snapshot{ mSecondarySourcesSnapshots.getReference(secondaryIndex) };
    auto strategy{ getLinkStrategy(mSourceLink) };

    if (strategy != nullptr) {
        strategy->calculateParams(mPrimarySourceSnapshot, mSources.size());
        snapshot = strategy->getInversedSnapshot(snapshot);
    }

    primarySourceMoved(); // some positions are invalid - fix them right away
}

void SourceLinkEnforcer::snapAll()
{
    mPrimarySourceSnapshot.source = &mSources.getPrimarySource();
    mPrimarySourceSnapshot.takeSnapshot();
    mSecondarySourcesSnapshots.clear();
    for (auto & secondarySource : mSources.getSecondarySources()) {
        SourceSnapshot newItem;
        newItem.source = &secondarySource;
        newItem.takeSnapshot();
        mSecondarySourcesSnapshots.add(newItem);
    }
}

void SourceLinkEnforcer::reset()
{
    for (auto & source : mSources) {
        source.removeSourceLinkListener(this);
    }
    snapAll();
    for (auto & source : mSources) {
        source.addSourceLinkListener(this);
    }
}

void SourceLinkEnforcer::changeListenerCallback(ChangeBroadcaster * broadcaster)
{
    auto sourceChangeBroadcaster{ dynamic_cast<Source::SourceChangeBroadcaster *>(broadcaster) };
    jassert(sourceChangeBroadcaster != nullptr);
    if (sourceChangeBroadcaster != nullptr) {
        auto & source{ sourceChangeBroadcaster->getSource() };
        if (source.isPrimarySource()) {
            primarySourceMoved();
        } else {
            secondarySourceMoved(source.getIndex());
        }
    }
}
