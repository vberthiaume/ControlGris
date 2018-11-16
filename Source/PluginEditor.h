/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "GrisLookAndFeel.h"
#include "FieldComponent.h"
#include "BannerComponent.h"
#include "ParametersBoxComponent.h"
#include "TrajectoryBoxComponent.h"
#include "SettingsBoxComponent.h"
#include "SourceBoxComponent.h"
#include "InterfaceBoxComponent.h"
#include "Source.h"

const int MaxNumOfSources = 36;

//==============================================================================
/**
*/
class ControlGrisAudioProcessorEditor : public AudioProcessorEditor,
                                        private Value::Listener,
                                        public FieldComponent::Listener,
                                        public ParametersBoxComponent::Listener,
                                        public SettingsBoxComponent::Listener
{
public:
    ControlGrisAudioProcessorEditor (ControlGrisAudioProcessor&);
    ~ControlGrisAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void valueChanged (Value&) override;

    //==============================================================================
    void sourcePositionChanged(int sourceId) override;
    void parameterChanged(int parameterId, double value) override;
    void oscFormatChanged(int selectedId) override;

    Source * getSources();
    int getSelectedSource();

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ControlGrisAudioProcessor& processor;

    GrisLookAndFeel mGrisFeel;

    BannerComponent mainBanner;
    BannerComponent elevationBanner;
    BannerComponent parametersBanner;
    BannerComponent trajectoryBanner;
    BannerComponent settingsBanner;

    MainFieldComponent  mainField;
    ElevationFieldComponent  elevationField;

    TabbedComponent configurationComponent { TabbedButtonBar::Orientation::TabsAtTop };

    ParametersBoxComponent  parametersBox;
    TrajectoryBoxComponent  trajectoryBox;

    SettingsBoxComponent    settingsBox;
    SourceBoxComponent      sourceBox;
    InterfaceBoxComponent   interfaceBox;

    Source sources[MaxNumOfSources];
    int m_numOfSources;
    int m_selectedSource;
    int m_selectedOscFormat;

    // these are used to persist the UI's size - the values are stored along with the
    // filter's other parameters, and the UI component will update them when it gets
    // resized.
    Value lastUIWidth, lastUIHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlGrisAudioProcessorEditor)
};
