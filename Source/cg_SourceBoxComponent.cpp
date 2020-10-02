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

#include "cg_SourceBoxComponent.hpp"

#include "cg_constants.hpp"

//==============================================================================
SourceBoxComponent::SourceBoxComponent(GrisLookAndFeel & grisLookAndFeel) : mGrisLookAndFeel(grisLookAndFeel)
{
    mSelectedSource = SourceIndex{};
    mCurrentAzimuth = {};
    mCurrentElevation = MAX_ELEVATION;

    mSourcePlacementLabel.setText("Source Placement:", NotificationType::dontSendNotification);
    addAndMakeVisible(&mSourcePlacementLabel);

    addAndMakeVisible(&mSourcePlacementCombo);
    mSourcePlacementCombo.setTextWhenNothingSelected("Choose a source placement...");
    mSourcePlacementCombo.addItemList(SOURCE_PLACEMENT_SKETCH, 1);
    mSourcePlacementCombo.onChange = [this] {
        mListeners.call([&](Listener & l) {
            l.sourceBoxPlacementChanged(static_cast<SourcePlacement>(mSourcePlacementCombo.getSelectedId()));
            mSourcePlacementCombo.setSelectedId(0, NotificationType::dontSendNotification);
        });
    };

    mSourceNumberLabel.setText("Source Number:", NotificationType::dontSendNotification);
    addAndMakeVisible(&mSourceNumberLabel);

    addAndMakeVisible(&mSourceNumberCombo);
    mSourceNumberCombo.setTextWhenNothingSelected("Choose a source...");
    for (auto i{ 1 }; i <= 8; ++i) {
        mSourceNumberCombo.addItem(juce::String{ i }, i);
    }
    mSourceNumberCombo.setSelectedId(mSelectedSource.toInt());
    mSourceNumberCombo.onChange = [this] {
        mSelectedSource = SourceIndex{ mSourceNumberCombo.getSelectedItemIndex() };
        mListeners.call([&](Listener & l) { l.sourceBoxSelectionChanged(mSelectedSource); });
    };

    mRayLengthLabel.setText("Elevation:", NotificationType::dontSendNotification);
    addAndMakeVisible(&mRayLengthLabel);

    mRayLengthSlider.setNormalisableRange(NormalisableRange<double>(0.0f, 1.0f, 0.01f));
    mRayLengthSlider.setValue(1.0, NotificationType::dontSendNotification);
    mRayLengthSlider.setTextBoxStyle(Slider::TextBoxRight, false, 40, 20);
    mRayLengthSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    addAndMakeVisible(&mRayLengthSlider);
    mRayLengthSlider.onValueChange = [this] {
        mCurrentElevation = MAX_ELEVATION * (1.0f - mRayLengthSlider.getValue());
        mListeners.call(
            [&](Listener & l) { l.sourceBoxPositionChanged(mSelectedSource, mCurrentAzimuth, mCurrentElevation); });
    };

    mAngleLabel.setText("Azimuth:", NotificationType::dontSendNotification);
    addAndMakeVisible(&mAngleLabel);

    mAngleSlider.setNormalisableRange(NormalisableRange<double>(0.0f, 360.0f, 0.01f));
    mAngleSlider.setValue(0.0, NotificationType::dontSendNotification);
    mAngleSlider.setTextBoxStyle(Slider::TextBoxRight, false, 40, 20);
    mAngleSlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    addAndMakeVisible(&mAngleSlider);
    mAngleSlider.onValueChange = [this] {
        mCurrentAzimuth = Degrees{ static_cast<float>(mAngleSlider.getValue()) };
        mListeners.call(
            [&](Listener & l) { l.sourceBoxPositionChanged(mSelectedSource, mCurrentAzimuth, mCurrentElevation); });
    };
}

//==============================================================================
void SourceBoxComponent::paint(Graphics & g)
{
    g.fillAll(mGrisLookAndFeel.findColour(ResizableWindow::backgroundColourId));
}

//==============================================================================
void SourceBoxComponent::resized()
{
    mSourcePlacementLabel.setBounds(5, 10, 150, 15);
    mSourcePlacementCombo.setBounds(130, 10, 150, 20);

    mSourceNumberLabel.setBounds(305, 10, 150, 15);
    mSourceNumberCombo.setBounds(430, 10, 150, 20);

    mRayLengthLabel.setBounds(305, 40, 150, 15);
    mRayLengthSlider.setBounds(380, 40, 200, 20);

    mAngleLabel.setBounds(305, 70, 150, 15);
    mAngleSlider.setBounds(380, 70, 200, 20);
}

//==============================================================================
void SourceBoxComponent::setNumberOfSources(int const numOfSources, SourceId const firstSourceId)
{
    mSourceNumberCombo.clear();
    for (auto id = firstSourceId; id < firstSourceId + numOfSources; ++id) {
        mSourceNumberCombo.addItem(id.toString(), id.toInt());
    }
    if (mSelectedSource >= SourceIndex{ numOfSources })
        mSelectedSource = SourceIndex{ 0 };
    mSourceNumberCombo.setSelectedItemIndex(mSelectedSource.toInt());
}

//==============================================================================
void SourceBoxComponent::updateSelectedSource(Source * source, SourceIndex const sourceIndex, SpatMode spatMode)
{
    mSelectedSource = sourceIndex;
    mSourceNumberCombo.setSelectedItemIndex(mSelectedSource.toInt());
    if (spatMode == SpatMode::dome) {
        mCurrentAzimuth = source->getAzimuth();
        mCurrentElevation = MAX_ELEVATION * source->getNormalizedElevation().toFloat();
    } else {
        mCurrentAzimuth = source->getAzimuth();
        mCurrentElevation = MAX_ELEVATION * source->getDistance();
    }
    if (mCurrentAzimuth.getAsDegrees() < 0.0f) {
        mCurrentAzimuth += Degrees{ 360.0f };
    }
    mAngleSlider.setValue(mCurrentAzimuth.getAsDegrees(), NotificationType::dontSendNotification);
    mRayLengthSlider.setValue(1.0f - mCurrentElevation / MAX_ELEVATION, NotificationType::dontSendNotification);
}
