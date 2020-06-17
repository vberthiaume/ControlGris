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
#include "PluginEditor.h"

#include "ControlGrisConstants.h"
#include "PluginProcessor.h"

ControlGrisAudioProcessorEditor::ControlGrisAudioProcessorEditor(
    ControlGrisAudioProcessor & p,
    AudioProcessorValueTreeState & vts,
    PositionAutomationManager & positionAutomationManager,
    ElevationAutomationManager & elevationAutomationManager)
    : AudioProcessorEditor(&p)
    , mProcessor(p)
    , mAudioProcessorValueTreeState(vts)
    , mPositionAutomationManager(positionAutomationManager)
    , mElevationAutomationManager(elevationAutomationManager)
    , mPositionField(positionAutomationManager)
    , mElevationField(elevationAutomationManager)
{
    setLookAndFeel(&mGrisLookAndFeel);

    mIsInsideSetPluginState = false;
    mSelectedSource = {};

    // Set up the interface.
    //----------------------
    mMainBanner.setLookAndFeel(&mGrisLookAndFeel);
    mMainBanner.setText("Azimuth - Elevation", NotificationType::dontSendNotification);
    addAndMakeVisible(&mMainBanner);

    mElevationBanner.setLookAndFeel(&mGrisLookAndFeel);
    mElevationBanner.setText("Elevation", NotificationType::dontSendNotification);
    addAndMakeVisible(&mElevationBanner);

    mTrajectoryBanner.setLookAndFeel(&mGrisLookAndFeel);
    mTrajectoryBanner.setText("Trajectories", NotificationType::dontSendNotification);
    addAndMakeVisible(&mTrajectoryBanner);

    mSettingsBanner.setLookAndFeel(&mGrisLookAndFeel);
    mSettingsBanner.setText("Configuration", NotificationType::dontSendNotification);
    addAndMakeVisible(&mSettingsBanner);

    mPositionPresetBanner.setLookAndFeel(&mGrisLookAndFeel);
    mPositionPresetBanner.setText("Preset", NotificationType::dontSendNotification);
    addAndMakeVisible(&mPositionPresetBanner);

    mPositionField.setLookAndFeel(&mGrisLookAndFeel);
    mPositionField.addListener(this);
    addAndMakeVisible(&mPositionField);

    mElevationField.setLookAndFeel(&mGrisLookAndFeel);
    mElevationField.addListener(this);
    addAndMakeVisible(&mElevationField);

    mParametersBox.setLookAndFeel(&mGrisLookAndFeel);
    mParametersBox.addListener(this);
    addAndMakeVisible(&mParametersBox);

    mTrajectoryBox.setLookAndFeel(&mGrisLookAndFeel);
    mTrajectoryBox.addListener(this);
    addAndMakeVisible(mTrajectoryBox);
    mTrajectoryBox.setPositionSourceLink(mPositionAutomationManager.getSourceLink());
    mTrajectoryBox.setElevationSourceLink(
        static_cast<ElevationSourceLink>(mElevationAutomationManager.getSourceLink()));

    mSettingsBox.setLookAndFeel(&mGrisLookAndFeel);
    mSettingsBox.addListener(this);

    mSourceBox.setLookAndFeel(&mGrisLookAndFeel);
    mSourceBox.addListener(this);

    mInterfaceBox.setLookAndFeel(&mGrisLookAndFeel);
    mInterfaceBox.addListener(this);

    Colour bg = mGrisLookAndFeel.findColour(ResizableWindow::backgroundColourId);

    mConfigurationComponent.setLookAndFeel(&mGrisLookAndFeel);
    mConfigurationComponent.setColour(TabbedComponent::backgroundColourId, bg);
    mConfigurationComponent.addTab("Settings", bg, &mSettingsBox, false);
    mConfigurationComponent.addTab("Source", bg, &mSourceBox, false);
    mConfigurationComponent.addTab("Controllers", bg, &mInterfaceBox, false);
    addAndMakeVisible(mConfigurationComponent);

    mPositionPresetBox.setLookAndFeel(&mGrisLookAndFeel);
    mPositionPresetBox.addListener(this);
    addAndMakeVisible(&mPositionPresetBox);

    // Add sources to the fields.
    //---------------------------
    mPositionField.setSources(mProcessor.getSources());
    mElevationField.setSources(mProcessor.getSources());

    mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
    mProcessor.setSelectedSource(mSelectedSource);

    // Manage dynamic window size of the plugin.
    //------------------------------------------
    setResizeLimits(MIN_FIELD_WIDTH + 50, MIN_FIELD_WIDTH + 20, 1800, 1300);

    mLastUIWidth.referTo(mProcessor.mParameters.state.getChildWithName("uiState").getPropertyAsValue("width", nullptr));
    mLastUIHeight.referTo(
        mProcessor.mParameters.state.getChildWithName("uiState").getPropertyAsValue("height", nullptr));

    // set our component's initial size to be the last one that was stored in the filter's settings
    setSize(mLastUIWidth.getValue(), mLastUIHeight.getValue());

    mLastUIWidth.addListener(this);
    mLastUIHeight.addListener(this);

    // Load the last saved state of the plugin.
    //-----------------------------------------
    setPluginState();
}

ControlGrisAudioProcessorEditor::~ControlGrisAudioProcessorEditor()
{
    mConfigurationComponent.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void ControlGrisAudioProcessorEditor::setPluginState()
{
    mIsInsideSetPluginState = true;

    // Set global settings values.
    //----------------------------
    settingsBoxOscFormatChanged(mProcessor.getOscFormat());
    settingsBoxOscPortNumberChanged(mProcessor.getOscPortNumber());
    settingsBoxOscActivated(mProcessor.getOscConnected());
    settingsBoxFirstSourceIdChanged(mProcessor.getFirstSourceId());
    settingsBoxNumberOfSourcesChanged(mProcessor.getNumberOfSources());

    mInterfaceBox.setOscOutputPluginId(mAudioProcessorValueTreeState.state.getProperty("oscOutputPluginId", 1));
    mInterfaceBox.setOscReceiveToggleState(mAudioProcessorValueTreeState.state.getProperty("oscInputConnected", false));
    mInterfaceBox.setOscReceiveInputPort(mAudioProcessorValueTreeState.state.getProperty("oscInputPortNumber", 9000));

    mInterfaceBox.setOscSendToggleState(mAudioProcessorValueTreeState.state.getProperty("oscOutputConnected", false));
    mInterfaceBox.setOscSendOutputAddress(
        mAudioProcessorValueTreeState.state.getProperty("oscOutputAddress", "192.168.1.100"));
    mInterfaceBox.setOscSendOutputPort(mAudioProcessorValueTreeState.state.getProperty("oscOutputPortNumber", 8000));

    // Set state for trajectory box persistent values.
    //------------------------------------------------
    mTrajectoryBox.setTrajectoryType(mAudioProcessorValueTreeState.state.getProperty("trajectoryType", 1));
    mTrajectoryBox.setElevationTrajectoryType(mAudioProcessorValueTreeState.state.getProperty("trajectoryTypeAlt", 1));
    mTrajectoryBox.setPositionBackAndForth(mAudioProcessorValueTreeState.state.getProperty("backAndForth", false));
    mTrajectoryBox.setElevationBackAndForth(mAudioProcessorValueTreeState.state.getProperty("backAndForthAlt", false));
    mTrajectoryBox.setPositionDampeningCycles(mAudioProcessorValueTreeState.state.getProperty("dampeningCycles", 0));
    mPositionAutomationManager.setPositionDampeningCycles(
        mAudioProcessorValueTreeState.state.getProperty("dampeningCycles", 0));
    mTrajectoryBox.setElevationDampeningCycles(
        mAudioProcessorValueTreeState.state.getProperty("dampeningCyclesAlt", 0));
    mElevationAutomationManager.setPositionDampeningCycles(
        mAudioProcessorValueTreeState.state.getProperty("dampeningCyclesAlt", 0));
    mTrajectoryBox.setDeviationPerCycle(mAudioProcessorValueTreeState.state.getProperty("deviationPerCycle", 0));
    mPositionAutomationManager.setDeviationPerCycle(
        Degrees{ mAudioProcessorValueTreeState.state.getProperty("deviationPerCycle", 0) });
    mTrajectoryBox.setCycleDuration(mAudioProcessorValueTreeState.state.getProperty("cycleDuration", 5.0));
    mTrajectoryBox.setDurationUnit(mAudioProcessorValueTreeState.state.getProperty("durationUnit", 1));

    // Update the position preset box.
    //--------------------------------
    for (int i{}; i < NUMBER_OF_POSITION_PRESETS; ++i) {
        mPositionPresetBox.presetSaved(i + 1, false);
    }
    XmlElement * positionData = mProcessor.getFixedPositionData();
    XmlElement * fpos = positionData->getFirstChildElement();
    while (fpos) {
        mPositionPresetBox.presetSaved(fpos->getIntAttribute("ID"), true);
        fpos = fpos->getNextElement();
    }

    // Update the interface.
    //----------------------
    mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
    mPositionField.setSelectedSource(mSelectedSource);
    mElevationField.setSelectedSource(mSelectedSource);
    mProcessor.setSelectedSource(mSelectedSource);
    mSourceBox.updateSelectedSource(&mProcessor.getSources()[mSelectedSource],
                                    mSelectedSource,
                                    mProcessor.getOscFormat());

    int preset = (int)((float)mAudioProcessorValueTreeState.getParameterAsValue("positionPreset").getValue());
    mPositionPresetBox.setPreset(preset, true);

    mIsInsideSetPluginState = false;
}

void ControlGrisAudioProcessorEditor::updateSpanLinkButton(bool state)
{
    mParametersBox.setSpanLinkState(state);
}

void ControlGrisAudioProcessorEditor::updateSourceLinkCombo(PositionSourceLink value)
{
    mTrajectoryBox.mPositionSourceLinkCombo.setSelectedId(static_cast<int>(value),
                                                          NotificationType::dontSendNotification);
}

void ControlGrisAudioProcessorEditor::updateElevationSourceLinkCombo(ElevationSourceLink value)
{
    mTrajectoryBox.mElevationSourceLinkCombo.setSelectedId(static_cast<int>(value),
                                                           NotificationType::dontSendNotification);
}

void ControlGrisAudioProcessorEditor::updatePositionPreset(int presetNumber)
{
    mPositionPresetBox.setPreset(presetNumber, true);
}

// Value::Listener callback. Called when the stored window size changes.
//----------------------------------------------------------------------
void ControlGrisAudioProcessorEditor::valueChanged(Value &)
{
    setSize(mLastUIWidth.getValue(), mLastUIHeight.getValue());
}

// SettingsBoxComponent::Listener callbacks.
//------------------------------------------
void ControlGrisAudioProcessorEditor::settingsBoxOscFormatChanged(SpatMode mode)
{
    mSettingsBox.setOscFormat(mode);
    mProcessor.setOscFormat(mode);
    bool selectionIsLBAP = mode == SpatMode::cube;
    mParametersBox.setDistanceEnabled(selectionIsLBAP);
    mPositionField.setSpatMode(mode);
    mTrajectoryBox.setSpatMode(mode);
    repaint();
    resized();
}

void ControlGrisAudioProcessorEditor::settingsBoxOscPortNumberChanged(int oscPort)
{
    mProcessor.setOscPortNumber(oscPort);
    mSettingsBox.setOscPortNumber(oscPort);
}

void ControlGrisAudioProcessorEditor::settingsBoxOscActivated(bool state)
{
    mProcessor.handleOscConnection(state);
    mSettingsBox.setActivateButtonState(mProcessor.getOscConnected());
}

void ControlGrisAudioProcessorEditor::settingsBoxNumberOfSourcesChanged(int numOfSources)
{
    bool initSourcePlacement = false;
    if (mProcessor.getNumberOfSources() != numOfSources || mIsInsideSetPluginState) {
        if (mProcessor.getNumberOfSources() != numOfSources) {
            initSourcePlacement = true;
        }
        if (numOfSources != 2
            && (mPositionAutomationManager.getSourceLink() == PositionSourceLink::linkSymmetricX
                || mPositionAutomationManager.getSourceLink() == PositionSourceLink::linkSymmetricY)) {
            mPositionAutomationManager.setSourceLink(PositionSourceLink::independent);
            updateSourceLinkCombo(PositionSourceLink::independent);
        }
        mSelectedSource = {};
        mProcessor.setNumberOfSources(numOfSources);
        mProcessor.setSelectedSource(mSelectedSource);
        mSettingsBox.setNumberOfSources(numOfSources);
        mTrajectoryBox.setNumberOfSources(numOfSources);
        mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
        mPositionField.setSources(mProcessor.getSources());
        mElevationField.setSources(mProcessor.getSources());
        mSourceBox.setNumberOfSources(numOfSources, mProcessor.getFirstSourceId());
        if (initSourcePlacement) {
            sourceBoxPlacementChanged(SourcePlacement::leftAlternate);
        }
    }
}

void ControlGrisAudioProcessorEditor::settingsBoxFirstSourceIdChanged(SourceId const firstSourceId)
{
    mProcessor.setFirstSourceId(firstSourceId);
    mSettingsBox.setFirstSourceId(firstSourceId);
    mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
    mSourceBox.setNumberOfSources(mProcessor.getNumberOfSources(), firstSourceId);

    mPositionField.rebuildSourceComponents(mProcessor.getNumberOfSources());
    mElevationField.rebuildSourceComponents(mProcessor.getNumberOfSources());
    if (mProcessor.getOscFormat() == SpatMode::cube)
        mElevationField.repaint();
}

// SourceBoxComponent::Listener callbacks.
//----------------------------------------
void ControlGrisAudioProcessorEditor::sourceBoxSelectionChanged(SourceIndex const sourceIndex)
{
    mSelectedSource = sourceIndex;

    mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
    mPositionField.setSelectedSource(mSelectedSource);
    mElevationField.setSelectedSource(mSelectedSource);
    mProcessor.setSelectedSource(mSelectedSource);
    mSourceBox.updateSelectedSource(&mProcessor.getSources()[mSelectedSource],
                                    mSelectedSource,
                                    mProcessor.getOscFormat());
}

void ControlGrisAudioProcessorEditor::sourceBoxPlacementChanged(SourcePlacement const sourcePlacement)
{
    auto numOfSources = mProcessor.getNumberOfSources();
    Degrees const azims2[2] = { Degrees{ -90.0f }, Degrees{ 90.0f } };
    Degrees const azims4[4] = { Degrees{ -45.0f }, Degrees{ 45.0f }, Degrees{ -135.0f }, Degrees{ 135.0f } };
    Degrees const azims6[6] = { Degrees{ -30.0f }, Degrees{ 30.0f },   Degrees{ -90.0f },
                                Degrees{ 90.0f },  Degrees{ -150.0f }, Degrees{ 150.0f } };
    Degrees const azims8[8] = { Degrees{ -22.5f },  Degrees{ 22.5f },  Degrees{ -67.5f },  Degrees{ 67.5f },
                                Degrees{ -112.5f }, Degrees{ 112.5f }, Degrees{ -157.5f }, Degrees{ 157.5f } };

    bool isLBAP = mProcessor.getOscFormat() == SpatMode::cube;

    auto const offset{ Degrees{ 360.0f } / numOfSources / 2.0f };
    auto const distance{ isLBAP ? 0.7f : 1.0f };

    switch (sourcePlacement) {
    case SourcePlacement::leftAlternate: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            if (numOfSources <= 2) {
                source.setCoordinates(-azims2[i], elevation, distance, SourceLinkNotification::notify);

            } else if (numOfSources <= 4) {
                source.setCoordinates(-azims4[i], elevation, distance, SourceLinkNotification::notify);
            } else if (numOfSources <= 6) {
                source.setCoordinates(-azims6[i], elevation, distance, SourceLinkNotification::notify);
            } else {
                source.setCoordinates(-azims8[i], elevation, distance, SourceLinkNotification::notify);
            }
        }
        break;
    }
    case SourcePlacement::rightAlternate: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            if (numOfSources <= 2) {
                source.setCoordinates(azims2[i], elevation, distance, SourceLinkNotification::notify);
            } else if (numOfSources <= 4) {
                source.setCoordinates(azims4[i], elevation, distance, SourceLinkNotification::notify);
            } else if (numOfSources <= 6) {
                source.setCoordinates(azims6[i], elevation, distance, SourceLinkNotification::notify);
            } else {
                source.setCoordinates(azims8[i], elevation, distance, SourceLinkNotification::notify);
            }
        }
        break;
    }
    case SourcePlacement::leftClockwise: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const azimuth{ Degrees{ 360.0f } / numOfSources * -i + offset };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            source.setCoordinates(azimuth, elevation, distance, SourceLinkNotification::notify);
        }
        break;
    }
    case SourcePlacement::leftCounterClockwise: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const azimuth{ Degrees{ 360.0f } / numOfSources * i + offset };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            mProcessor.getSources()[i].setCoordinates(azimuth, elevation, distance, SourceLinkNotification::notify);
        }
        break;
    }
    case SourcePlacement::rightClockwise: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const azimuth{ Degrees{ 360.0f } / numOfSources * -i - offset };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            source.setCoordinates(azimuth, elevation, distance, SourceLinkNotification::notify);
        }
        break;
    }
    case SourcePlacement::rightCounterClockwise: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const azimuth{ Degrees{ 360.0f } / numOfSources * i - offset };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            source.setCoordinates(azimuth, elevation, distance, SourceLinkNotification::notify);
        }
        break;
    }
    case SourcePlacement::topClockwise: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const azimuth{ Degrees{ 360.0f } / numOfSources * -i };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            source.setCoordinates(azimuth, elevation, distance, SourceLinkNotification::notify);
        }
        break;
    }
    case SourcePlacement::topCounterClockwise: {
        for (int i{}; i < numOfSources; ++i) {
            auto & source{ mProcessor.getSources()[i] };
            auto const azimuth{ Degrees{ 360.0f } / numOfSources * i };
            auto const elevation{ isLBAP ? source.getElevation() : Radians{} };
            source.setCoordinates(azimuth, elevation, distance, SourceLinkNotification::notify);
        }
        break;
    }
    case SourcePlacement::undefined:
        jassertfalse;
    }

    for (SourceIndex i{}; i < SourceIndex{ numOfSources }; ++i) {
        mProcessor.setSourceParameterValue(i,
                                           SourceParameter::azimuth,
                                           mProcessor.getSources()[i].getNormalizedAzimuth());
        mProcessor.setSourceParameterValue(i,
                                           SourceParameter::elevation,
                                           mProcessor.getSources()[i].getNormalizedElevation());
        mProcessor.setSourceParameterValue(i, SourceParameter::distance, mProcessor.getSources()[i].getDistance());
    }

    mSourceBox.updateSelectedSource(&mProcessor.getSources()[mSelectedSource],
                                    mSelectedSource,
                                    mProcessor.getOscFormat());

    //    for (int i{}; i < numOfSources; ++i) {
    //        mProcessor.getSources()[i].fixSourcePosition(true);
    //    }

    mPositionAutomationManager.setTrajectoryType(mPositionAutomationManager.getTrajectoryType(),
                                                 mProcessor.getSources().getPrimarySource().getPos());

    repaint();
}

void ControlGrisAudioProcessorEditor::sourceBoxPositionChanged(SourceIndex const sourceIndex,
                                                               Radians angle,
                                                               float rayLen)
{
    auto & source{ mProcessor.getSources()[sourceIndex] };
    if (mProcessor.getOscFormat() == SpatMode::dome) {
        auto const elevation{ Degrees{ 90.0f } - (Degrees{ 90.0f } * rayLen) };
        source.setCoordinates(angle, elevation, 1.0f, SourceLinkNotification::notify);
    } else {
        auto const currentElevation{ source.getElevation() };
        source.setCoordinates(angle, currentElevation, rayLen, SourceLinkNotification::notify);
    }

    //    source.fixSourcePosition(true);

    mPositionAutomationManager.setTrajectoryType(mPositionAutomationManager.getTrajectoryType(),
                                                 mProcessor.getSources().getPrimarySource().getPos());

    repaint();
}

// ParametersBoxComponent::Listener callbacks.
//--------------------------------------------
void ControlGrisAudioProcessorEditor::parametersBoxParameterChanged(SourceParameter const sourceParameter,
                                                                    double const value)
{
    mProcessor.setSourceParameterValue(mSelectedSource, sourceParameter, static_cast<float>(value));

    mPositionField.repaint();
    if (mProcessor.getOscFormat() == SpatMode::cube) {
        mElevationField.repaint();
    }
}

void ControlGrisAudioProcessorEditor::parametersBoxSelectedSourceClicked()
{
    mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
    mPositionField.setSelectedSource(mSelectedSource);
    mElevationField.setSelectedSource(mSelectedSource);
    mProcessor.setSelectedSource(mSelectedSource);
    mSourceBox.updateSelectedSource(&mProcessor.getSources()[mSelectedSource],
                                    mSelectedSource,
                                    mProcessor.getOscFormat());
}

// TrajectoryBoxComponent::Listener callbacks.
//--------------------------------------------
void ControlGrisAudioProcessorEditor::trajectoryBoxPositionSourceLinkChanged(PositionSourceLink value)
{
    mProcessor.setPositionSourceLink(value);
    mPositionField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxElevationSourceLinkChanged(ElevationSourceLink value)
{
    mProcessor.setElevationSourceLink(value);
    mElevationField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxPositionTrajectoryTypeChanged(PositionTrajectoryType value)
{
    mAudioProcessorValueTreeState.state.setProperty("trajectoryType", static_cast<int>(value), nullptr);
    mPositionAutomationManager.setTrajectoryType(value, mProcessor.getSources()[0].getPos());
    mPositionField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxElevationTrajectoryTypeChanged(ElevationTrajectoryType value)
{
    mAudioProcessorValueTreeState.state.setProperty("trajectoryTypeAlt", static_cast<int>(value), nullptr);
    mElevationAutomationManager.setTrajectoryType(value);
    mElevationField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxPositionBackAndForthChanged(bool value)
{
    mAudioProcessorValueTreeState.state.setProperty("backAndForth", value, nullptr);
    mPositionAutomationManager.setPositionBackAndForth(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxElevationBackAndForthChanged(bool value)
{
    mAudioProcessorValueTreeState.state.setProperty("backAndForthAlt", value, nullptr);
    mElevationAutomationManager.setPositionBackAndForth(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxPositionDampeningCyclesChanged(int value)
{
    mAudioProcessorValueTreeState.state.setProperty("dampeningCycles", value, nullptr);
    mPositionAutomationManager.setPositionDampeningCycles(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxElevationDampeningCyclesChanged(int value)
{
    mAudioProcessorValueTreeState.state.setProperty("dampeningCyclesAlt", value, nullptr);
    mElevationAutomationManager.setPositionDampeningCycles(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxDeviationPerCycleChanged(float degrees)
{
    mAudioProcessorValueTreeState.state.setProperty("deviationPerCycle", degrees, nullptr);
    mPositionAutomationManager.setDeviationPerCycle(Degrees{ degrees });
}

void ControlGrisAudioProcessorEditor::trajectoryBoxCycleDurationChanged(double duration, int mode)
{
    mAudioProcessorValueTreeState.state.setProperty("cycleDuration", duration, nullptr);
    double dur = duration;
    if (mode == 2) {
        dur = duration * 60.0 / mProcessor.getBPM();
    }
    mPositionAutomationManager.setPlaybackDuration(dur);
    mElevationAutomationManager.setPlaybackDuration(dur);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxDurationUnitChanged(double duration, int mode)
{
    mAudioProcessorValueTreeState.state.setProperty("durationUnit", mode, nullptr);
    double dur = duration;
    if (mode == 2) {
        dur = duration * 60.0 / mProcessor.getBPM();
    }
    mPositionAutomationManager.setPlaybackDuration(dur);
    mElevationAutomationManager.setPlaybackDuration(dur);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxPositionActivateChanged(bool value)
{
    mPositionAutomationManager.setPositionActivateState(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxElevationActivateChanged(bool value)
{
    mElevationAutomationManager.setPositionActivateState(value);
}

// Update the interface if anything has changed (mostly automations).
//-------------------------------------------------------------------
void ControlGrisAudioProcessorEditor::refresh()
{
    mParametersBox.setSelectedSource(&mProcessor.getSources()[mSelectedSource]);
    mSourceBox.updateSelectedSource(&mProcessor.getSources()[mSelectedSource],
                                    mSelectedSource,
                                    mProcessor.getOscFormat());

    mPositionField.setIsPlaying(mProcessor.isPlaying());
    mElevationField.setIsPlaying(mProcessor.isPlaying());

    mPositionField.repaint();
    if (mProcessor.getOscFormat() == SpatMode::cube)
        mElevationField.repaint();

    if (mTrajectoryBox.getPositionActivateState() != mPositionAutomationManager.getPositionActivateState()) {
        mTrajectoryBox.setPositionActivateState(mPositionAutomationManager.getPositionActivateState());
    }
    if (mTrajectoryBox.getElevationActivateState() != mElevationAutomationManager.getPositionActivateState()) {
        mTrajectoryBox.setElevationActivateState(mElevationAutomationManager.getPositionActivateState());
    }
}

// FieldComponent::Listener callback.
//-----------------------------------
void ControlGrisAudioProcessorEditor::fieldSourcePositionChanged(SourceIndex const sourceIndex, int whichField)
{
    mProcessor.sourcePositionChanged(sourceIndex, whichField);
    mSelectedSource = sourceIndex;
    mParametersBox.setSelectedSource(&mProcessor.getSources()[sourceIndex]);
    mPositionField.setSelectedSource(mSelectedSource);
    mElevationField.setSelectedSource(mSelectedSource);
    mProcessor.setSelectedSource(mSelectedSource);
    mSourceBox.updateSelectedSource(&mProcessor.getSources()[mSelectedSource],
                                    mSelectedSource,
                                    mProcessor.getOscFormat());

    mProcessor.setPositionPreset(0);
    mPositionPresetBox.setPreset(0);
}

void ControlGrisAudioProcessorEditor::fieldTrajectoryHandleClicked(int whichField)
{
    //    if (whichField == 0) {
    //        mPositionAutomationManager.fixPrincipalSourcePosition();
    //        mProcessor.onSourceLinkChanged(mPositionAutomationManager.getSourceLink());
    //    } else {
    //        mElevationAutomationManager.fixPrincipalSourcePosition();
    //        mProcessor.onElevationSourceLinkChanged(
    //            static_cast<ElevationSourceLink>(mElevationAutomationManager.getSourceLink()));
    //    }
}

// PositionPresetComponent::Listener callback.
//--------------------------------------------
void ControlGrisAudioProcessorEditor::positionPresetChanged(int presetNumber)
{
    mProcessor.setPositionPreset(presetNumber);
}

void ControlGrisAudioProcessorEditor::positionPresetSaved(int presetNumber)
{
    mProcessor.addNewFixedPosition(presetNumber);
}

void ControlGrisAudioProcessorEditor::positionPresetDeleted(int presetNumber)
{
    mProcessor.deleteFixedPosition(presetNumber);
}

// InterfaceBoxComponent::Listener callback.
//------------------------------------------
void ControlGrisAudioProcessorEditor::oscOutputPluginIdChanged(int value)
{
    mProcessor.setOscOutputPluginId(value);
}

void ControlGrisAudioProcessorEditor::oscInputConnectionChanged(bool state, int oscPort)
{
    if (state) {
        mProcessor.createOscInputConnection(oscPort);
    } else {
        mProcessor.disconnectOSCInput(oscPort);
    }
}

void ControlGrisAudioProcessorEditor::oscOutputConnectionChanged(bool state, String oscAddress, int oscPort)
{
    if (state) {
        mProcessor.createOscOutputConnection(oscAddress, oscPort);
    } else {
        mProcessor.disconnectOSCOutput(oscAddress, oscPort);
    }
}

//==============================================================================
void ControlGrisAudioProcessorEditor::paint(Graphics & g)
{
    GrisLookAndFeel * lookAndFeel;
    lookAndFeel = static_cast<GrisLookAndFeel *>(&getLookAndFeel());
    g.fillAll(lookAndFeel->findColour(ResizableWindow::backgroundColourId));
}

void ControlGrisAudioProcessorEditor::resized()
{
    double width = getWidth() - 50; // Remove position preset space.
    double height = getHeight();

    double fieldSize = width / 2;
    if (fieldSize < MIN_FIELD_WIDTH) {
        fieldSize = MIN_FIELD_WIDTH;
    }

    mPositionAutomationManager.setFieldWidth(fieldSize);
    mElevationAutomationManager.setFieldWidth(fieldSize);

    mMainBanner.setBounds(0, 0, fieldSize, 20);
    mPositionField.setBounds(0, 20, fieldSize, fieldSize);

    if (mProcessor.getOscFormat() == SpatMode::cube) {
        mMainBanner.setText("Azimuth - Distance", NotificationType::dontSendNotification);
        mElevationBanner.setVisible(true);
        mElevationField.setVisible(true);
        mElevationBanner.setBounds(fieldSize, 0, fieldSize, 20);
        mElevationField.setBounds(fieldSize, 20, fieldSize, fieldSize);
    } else {
        mMainBanner.setText("Azimuth - Elevation", NotificationType::dontSendNotification);
        mElevationBanner.setVisible(false);
        mElevationField.setVisible(false);
    }

    mParametersBox.setBounds(0, fieldSize + 20, width, 50);

    mTrajectoryBanner.setBounds(0, fieldSize + 70, width, 20);
    mTrajectoryBox.setBounds(0, fieldSize + 90, width, 160);

    mSettingsBanner.setBounds(0, fieldSize + 250, width, 20);
    mConfigurationComponent.setBounds(0, fieldSize + 270, width, 130);

    mLastUIWidth = getWidth();
    mLastUIHeight = getHeight();

    mPositionPresetBanner.setBounds(width, 0, 50, 20);
    mPositionPresetBox.setBounds(width, 20, 50, height - 20);
}
