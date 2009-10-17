/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  23 Nov 2007 2:34:19 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.11

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...

#include "Granulator.h"

//[/Headers]

#include "JGranulator.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...

// Globals

AudioDeviceManager * gAudioDeviceManager;
using namespace csl;

//[/MiscUserDefs]

//==============================================================================
CSLComponent::CSLComponent ()
    : startButton (0),
      quitButton (0),
      label (0),
      fileButton (0),
      oscilloscope (0),
      rateSlider (0),
      offsetSlider (0),
      densitySlider (0),
      durationSlider (0),
      widthSlider (0),
      label2 (0),
      label4 (0),
      label7 (0),
      label8 (0),
      label9 (0),
      volumeSlider (0),
      label3 (0),
      VUMeterL (0),
      VUMeterR (0),
      envelopeSlider (0),
      reverbSlider (0),
      label5 (0),
      label6 (0)
{
    addAndMakeVisible (startButton = new TextButton (T("startStop")));
    startButton->setButtonText (T("start"));
    startButton->addButtonListener (this);

    addAndMakeVisible (quitButton = new TextButton (T("quitAction")));
    quitButton->setButtonText (T("quit"));
    quitButton->addButtonListener (this);

    addAndMakeVisible (label = new Label (T("CSL test"),
                                          T("CSL Granulator")));
    label->setFont (Font (Font::getDefaultSerifFontName(), 32.0000f, Font::bold));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colours::blue);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (fileButton = new TextButton (T("fileButton")));
    fileButton->setButtonText (T("choose file"));
    fileButton->addButtonListener (this);

    addAndMakeVisible (oscilloscope = new AudioWaveformDisplay());
    oscilloscope->setName (T("oscilloscope"));

    addAndMakeVisible (rateSlider = new RangeSlider (T("rateSlider")));
    rateSlider->setRange (-4, 4, 0);
    rateSlider->setSliderStyle (Slider::TwoValueHorizontal);
    rateSlider->setTextBoxStyle (Slider::NoTextBox, true, 60, 20);
    rateSlider->addListener (this);

    addAndMakeVisible (offsetSlider = new RangeSlider (T("offsetSlider")));
    offsetSlider->setRange (0, 1, 0);
    offsetSlider->setSliderStyle (Slider::TwoValueHorizontal);
    offsetSlider->setTextBoxStyle (Slider::NoTextBox, true, 60, 20);
    offsetSlider->addListener (this);

    addAndMakeVisible (densitySlider = new RangeSlider (T("densitySlider")));
    densitySlider->setRange (1, 150, 0);
    densitySlider->setSliderStyle (Slider::TwoValueHorizontal);
    densitySlider->setTextBoxStyle (Slider::NoTextBox, true, 60, 20);
    densitySlider->addListener (this);

    addAndMakeVisible (durationSlider = new RangeSlider (T("durationSlider")));
    durationSlider->setRange (0.01, 0.25, 0);
    durationSlider->setSliderStyle (Slider::TwoValueHorizontal);
    durationSlider->setTextBoxStyle (Slider::NoTextBox, true, 60, 20);
    durationSlider->addListener (this);

    addAndMakeVisible (widthSlider = new RangeSlider (T("widthSlider")));
    widthSlider->setRange (0, 1.0, 0);
    widthSlider->setSliderStyle (Slider::TwoValueHorizontal);
    widthSlider->setTextBoxStyle (Slider::NoTextBox, true, 60, 20);
    widthSlider->addListener (this);

    addAndMakeVisible (label2 = new Label (T("new label"),
                                           T("Rate")));
    label2->setFont (Font (15.0000f, Font::plain));
    label2->setJustificationType (Justification::centredLeft);
    label2->setEditable (false, false, false);
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label4 = new Label (T("new label"),
                                           T("Offset")));
    label4->setFont (Font (15.0000f, Font::plain));
    label4->setJustificationType (Justification::centredLeft);
    label4->setEditable (false, false, false);
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label7 = new Label (T("new label"),
                                           T("Duration")));
    label7->setFont (Font (15.0000f, Font::plain));
    label7->setJustificationType (Justification::centredLeft);
    label7->setEditable (false, false, false);
    label7->setColour (TextEditor::textColourId, Colours::black);
    label7->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label8 = new Label (T("new label"),
                                           T("Density")));
    label8->setFont (Font (15.0000f, Font::plain));
    label8->setJustificationType (Justification::centredLeft);
    label8->setEditable (false, false, false);
    label8->setColour (TextEditor::textColourId, Colours::black);
    label8->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label9 = new Label (T("new label"),
                                           T("Position")));
    label9->setFont (Font (15.0000f, Font::plain));
    label9->setJustificationType (Justification::centredLeft);
    label9->setEditable (false, false, false);
    label9->setColour (TextEditor::textColourId, Colours::black);
    label9->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (volumeSlider = new RangeSlider (T("volumeSlider")));
    volumeSlider->setRange (0, 40, 0);
    volumeSlider->setSliderStyle (Slider::TwoValueHorizontal);
    volumeSlider->setTextBoxStyle (Slider::NoTextBox, true, 60, 20);
    volumeSlider->addListener (this);

    addAndMakeVisible (label3 = new Label (T("new label"),
                                           T("Volume")));
    label3->setFont (Font (15.0000f, Font::plain));
    label3->setJustificationType (Justification::centredLeft);
    label3->setEditable (false, false, false);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (VUMeterL = new VUMeter());
    VUMeterL->setName (T("VUMeterL"));

    addAndMakeVisible (VUMeterR = new VUMeter());
    VUMeterR->setName (T("VUMeterR"));

    addAndMakeVisible (envelopeSlider = new RangeSlider (T("new slider")));
    envelopeSlider->setRange (0.0001, 1, 0);
    envelopeSlider->setSliderStyle (Slider::TwoValueHorizontal);
    envelopeSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    envelopeSlider->addListener (this);

    addAndMakeVisible (reverbSlider = new Slider (T("new slider")));
    reverbSlider->setRange (0.0, 0.99, 0);
    reverbSlider->setSliderStyle (Slider::LinearHorizontal);
    reverbSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    reverbSlider->addListener (this);

    addAndMakeVisible (label5 = new Label (T("new label"),
                                           T("Envelope")));
    label5->setFont (Font (15.0000f, Font::plain));
    label5->setJustificationType (Justification::centredLeft);
    label5->setEditable (false, false, false);
    label5->setColour (TextEditor::textColourId, Colours::black);
    label5->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label6 = new Label (T("new label"),
                                           T("Reverb")));
    label6->setFont (Font (15.0000f, Font::plain));
    label6->setJustificationType (Justification::centredLeft);
    label6->setEditable (false, false, false);
    label6->setColour (TextEditor::textColourId, Colours::black);
    label6->setColour (TextEditor::backgroundColourId, Colour (0x0));

    //[UserPreSize]

//////////////////////////////////////////////////////////////////

	oscilloscope->initialise(-1, 10, 2, true);	// unsigned rate, unsigned window, bool zeroX
												// Create the VU meters
	VUMeterL = new VUMeter(0, VUMeter::Vertical, 32);
	VUMeterL->setDecay(33,2);
	addAndMakeVisible(VUMeterL);

	VUMeterR = new VUMeter(1, VUMeter::Vertical, 32);
	VUMeterR->setDecay(33,2);
	addAndMakeVisible(VUMeterR);

    //[/UserPreSize]

    setSize (400, 480);

    //[Constructor] You can add your own custom stuff here..
	
					// and initialise the device manager with no settings so that it picks a
					// default device to use.

//////////////////////////////////////////////////////////////////

	const String error (mAudioDeviceManager.initialise (0, /* number of input channels */
													   2, /* number of output channels */
													   0, /* no XML settings.. */
													   true  /* select default device on failure */));
	if (error.isNotEmpty())
		AlertWindow::showMessageBox (AlertWindow::WarningIcon,
									 T("CSL Demo"),
									 T("Couldn't open an output device!\n\n") + error);
	else
		mAudioDeviceManager.addAudioCallback (this);
	gAudioDeviceManager = & mAudioDeviceManager;

				// set up CSL granulator objects
	cloud = new csl::GrainCloud;						// grain cloud controller
#ifdef USE_REVERB
	player = new csl::GrainPlayer;						// grain player
	source = new csl::Stereoverb(*player);				// stereo reverb
#else
	source = new csl::GrainPlayer(cloud);
#endif
	isPlaying = false;
			// set slider defaults
	rateSlider->setBaseValue(1.0);
	rateSlider->setRangeValue(0.02);
	offsetSlider->setBaseValue(0.1);
	offsetSlider->setRangeValue(0.0);
	durationSlider->setBaseValue(0.1);
	durationSlider->setRangeValue(0.0);
	densitySlider->setBaseValue(8);
	densitySlider->setRangeValue(0.0);
	widthSlider->setBaseValue(0.5);
	widthSlider->setRangeValue(0.1);
	volumeSlider->setBaseValue(8.0);
	volumeSlider->setRangeValue(0.0);
	envelopeSlider->setBaseValue(0.5);
	envelopeSlider->setRangeValue(0.0);
	reverbSlider->setValue(0.2);

	cloud->mRateBase = rateSlider->getBaseValue();
	cloud->mRateRange = rateSlider->getRangeValue();
	cloud->mOffsetBase = offsetSlider->getBaseValue();
	cloud->mOffsetRange = offsetSlider->getRangeValue();
	cloud->mDurationBase = durationSlider->getBaseValue();
	cloud->mDurationRange = durationSlider->getRangeValue();
	cloud->mDensityBase = densitySlider->getBaseValue();
	cloud->mDensityRange = densitySlider->getRangeValue();
	cloud->mWidthBase = widthSlider->getBaseValue();
	cloud->mWidthRange = widthSlider->getRangeValue();
	cloud->mVolumeBase = volumeSlider->getBaseValue();
	cloud->mVolumeRange = volumeSlider->getRangeValue();
	cloud->mEnvelopeBase = envelopeSlider->getBaseValue();
	cloud->mEnvelopeRange = envelopeSlider->getRangeValue();
#ifdef USE_REVERB
	((Stereoverb *)source)->setRoomSize(reverbSlider->getValue());
#endif
					// Create the MIDI mapper: pass it the MIDI device index and base controller channel
	theMapper = new MIDIControllerMapper(this, 3, 48);

//////////////////////////////////////////////////////////////////
    //[/Constructor]
}

CSLComponent::~CSLComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (startButton);
    deleteAndZero (quitButton);
    deleteAndZero (label);
    deleteAndZero (fileButton);
    deleteAndZero (oscilloscope);
    deleteAndZero (rateSlider);
    deleteAndZero (offsetSlider);
    deleteAndZero (densitySlider);
    deleteAndZero (durationSlider);
    deleteAndZero (widthSlider);
    deleteAndZero (label2);
    deleteAndZero (label4);
    deleteAndZero (label7);
    deleteAndZero (label8);
    deleteAndZero (label9);
    deleteAndZero (volumeSlider);
    deleteAndZero (label3);
    deleteAndZero (VUMeterL);
    deleteAndZero (VUMeterR);
    deleteAndZero (envelopeSlider);
    deleteAndZero (reverbSlider);
    deleteAndZero (label5);
    deleteAndZero (label6);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void CSLComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xffc1d5d5));

    g.setColour (Colour (0xffd8d9dd));
    g.fillRect (-4, 297, 52, 143);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CSLComponent::resized()
{
    startButton->setBounds (208, 8, 176, 24);
    quitButton->setBounds (208, 452, 176, 24);
    label->setBounds (8, 452, 192, 24);
    fileButton->setBounds (16, 8, 176, 24);
    oscilloscope->setBounds (48, 296, 344, 144);
    rateSlider->setBounds (80, 40, 312, 24);
    offsetSlider->setBounds (80, 72, 312, 24);
    densitySlider->setBounds (80, 136, 312, 24);
    durationSlider->setBounds (80, 104, 312, 24);
    widthSlider->setBounds (80, 168, 312, 24);
    label2->setBounds (8, 40, 72, 24);
    label4->setBounds (8, 72, 72, 24);
    label7->setBounds (8, 104, 72, 24);
    label8->setBounds (8, 136, 72, 24);
    label9->setBounds (8, 168, 72, 24);
    volumeSlider->setBounds (80, 200, 312, 24);
    label3->setBounds (8, 200, 72, 24);
    VUMeterL->setBounds (6, 296, 16, 144);
    VUMeterR->setBounds (26, 296, 16, 144);
    envelopeSlider->setBounds (80, 232, 312, 24);
    reverbSlider->setBounds (80, 264, 310, 24);
    label5->setBounds (8, 232, 72, 24);
    label6->setBounds (8, 264, 72, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void CSLComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == startButton)
    {
        //[UserButtonCode_startButton] -- add your button handler code here..
		
		if (isPlaying) {
			oscilloscope->stop();
			startButton->setButtonText(T("start"));
			isPlaying = false;
			cloud->isPlaying = false;
			cloud->reset();
		} else {
			if (cloud->mSamples == 0)
				this->loadFile();
			startButton->setButtonText(T("stop"));
			oscilloscope->start();
			isPlaying = true;
			cloud->startThreads();				// start the cloud playing
		}
		
        //[/UserButtonCode_startButton]
    }
    else if (buttonThatWasClicked == quitButton)
    {
        //[UserButtonCode_quitButton] -- add your button handler code here..
		
		isPlaying = false;
		JUCEApplication::quit();
		
        //[/UserButtonCode_quitButton]
    }
    else if (buttonThatWasClicked == fileButton)
    {
        //[UserButtonCode_fileButton] -- add your button handler code here..
		
		this->loadFile();
		
        //[/UserButtonCode_fileButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void CSLComponent::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == rateSlider)
    {
        //[UserSliderCode_rateSlider] -- add your slider handling code here..
		
		cloud->mRateBase = rateSlider->getBaseValue();
		cloud->mRateRange = rateSlider->getRangeValue();
		
        //[/UserSliderCode_rateSlider]
    }
    else if (sliderThatWasMoved == offsetSlider)
    {
        //[UserSliderCode_offsetSlider] -- add your slider handling code here..
		
 		cloud->mOffsetBase = offsetSlider->getBaseValue();
 		cloud->mOffsetRange = offsetSlider->getRangeValue();
		
        //[/UserSliderCode_offsetSlider]
    }
    else if (sliderThatWasMoved == densitySlider)
    {
        //[UserSliderCode_densitySlider] -- add your slider handling code here..
		
		cloud->mDensityBase = densitySlider->getBaseValue();
		cloud->mDensityRange = densitySlider->getRangeValue();
		
        //[/UserSliderCode_densitySlider]
    }
    else if (sliderThatWasMoved == durationSlider)
    {
        //[UserSliderCode_durationSlider] -- add your slider handling code here..
		cloud->mDurationBase = durationSlider->getBaseValue();
		cloud->mDurationRange = durationSlider->getRangeValue();
		
        //[/UserSliderCode_durationSlider]
    }
    else if (sliderThatWasMoved == widthSlider)
    {
        //[UserSliderCode_widthSlider] -- add your slider handling code here..
		
 		cloud->mWidthBase = widthSlider->getBaseValue();
 		cloud->mWidthRange = widthSlider->getRangeValue();
		
        //[/UserSliderCode_widthSlider]
    }
    else if (sliderThatWasMoved == volumeSlider)
    {
        //[UserSliderCode_volumeSlider] -- add your slider handling code here..
		
 		cloud->mVolumeBase = volumeSlider->getBaseValue();
 		cloud->mVolumeRange = volumeSlider->getRangeValue();
		
        //[/UserSliderCode_volumeSlider]
    }
    else if (sliderThatWasMoved == envelopeSlider)
    {
        //[UserSliderCode_envelopeSlider] -- add your slider handling code here..
		
		cloud->mEnvelopeBase = envelopeSlider->getBaseValue();
		cloud->mEnvelopeRange = envelopeSlider->getRangeValue();
		
        //[/UserSliderCode_envelopeSlider]
    }
    else if (sliderThatWasMoved == reverbSlider)
    {
        //[UserSliderCode_reverbSlider] -- add your slider handling code here..
		
#ifdef USE_REVERB
		((Stereoverb *)source)->setRoomSize(reverbSlider->getValue());
#endif

        //[/UserSliderCode_reverbSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void CSLComponent::audioDeviceIOCallback (const float** inputChannelData,
							int totalNumInputChannels,
							float** outputChannelData,
							int totalNumOutputChannels,
							int numSamples) {
							
					// put silence in the output buffers
	for (unsigned i = 0; i < totalNumOutputChannels; i++)
		memset(outputChannelData[i], 0, numSamples * sizeof(float));
		
					// set up CSL buffer object
	if (isPlaying) {
		outBuffer.setSize(totalNumOutputChannels, numSamples);
		for (unsigned i = 0; i < totalNumOutputChannels; i++)
			outBuffer.mBuffers[i] = outputChannelData[i];
		try {	
			source->nextBuffer(outBuffer);		// Get a buffer from the CSL graph
		} catch (CException e) {
			printf("Error running CSL graph\n");
		}
					// pass the audio callback on to the waveform display & VU metercompoments
		oscilloscope->audioDeviceIOCallback (inputChannelData, 0,
				outputChannelData, totalNumOutputChannels, numSamples);
		VUMeterL->audioDeviceIOCallback (inputChannelData, 0,
				outputChannelData, totalNumOutputChannels, numSamples);
		VUMeterR->audioDeviceIOCallback (inputChannelData, 0,
				outputChannelData, totalNumOutputChannels, numSamples);
	}
}

// Audio device support

void CSLComponent::audioDeviceAboutToStart (AudioIODevice* device) {
	oscilloscope->audioDeviceAboutToStart (device);
}

void CSLComponent::audioDeviceStopped() {
//	oscilloscope->audioDeviceStopped();
}

Component* createCSLComponent() {
    return new CSLComponent();
}

// put up a file dialog to select an AIFF file

void CSLComponent::loadFile() {
        FileChooser myChooser ("Please select the file to granulate...",
                               File(String(CGestalt::dataFolder().c_str())),
                               "*.aiff");
	if (myChooser.browseForFileToOpen()) {
		File fil = myChooser.getResult();
		String nam = fil.getFullPathName();
		SoundFile sndFile(nam.toUTF8());
										// open and read in the file
		sndFile.openForRead();
		cloud->mSamples = sndFile.mWavetable.mBuffers[0];
		cloud->numSamples = sndFile.duration();
	}
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="CSLComponent" componentName=""
                 parentClasses="public Component, public AudioIODeviceCallback"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="1" initialWidth="400"
                 initialHeight="480">
  <BACKGROUND backgroundColour="ffc1d5d5">
  </BACKGROUND>
  <TEXTBUTTON name="startStop" id="ed1811e776eea99b" memberName="startButton"
              virtualName="" explicitFocusOrder="0" pos="208 8 176 24" buttonText="start"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="quitAction" id="dbaf2871fd41de83" memberName="quitButton"
              virtualName="" explicitFocusOrder="0" pos="208 452 176 24" buttonText="quit"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <LABEL name="CSL test" id="a9876f115ab3c22e" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="8 452 192 24" textCol="ff0000ff"
         edTextCol="ff000000" edBkgCol="0" labelText="CSL Granulator"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default serif font" fontsize="32" bold="1" italic="0"
         justification="33"/>
  <TEXTBUTTON name="fileButton" id="beeb15a1537fd4f6" memberName="fileButton"
              virtualName="" explicitFocusOrder="0" pos="16 8 176 24" buttonText="choose file"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <GENERICCOMPONENT name="oscilloscope" id="4aa0f216430fecde" memberName="oscilloscope"
                    virtualName="" explicitFocusOrder="0" pos="48 296 344 144" class="AudioWaveformDisplay"
                    params=""/>
  <SLIDER name="rateSlider" id="6202c9e598d1a508" memberName="rateSlider"
          virtualName="" explicitFocusOrder="0" pos="80 40 312 24" min="-4"
          max="4" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="offsetSlider" id="ea6a130f89e2bae6" memberName="offsetSlider"
          virtualName="" explicitFocusOrder="0" pos="80 72 312 24" min="0"
          max="1" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="densitySlider" id="2936deaa89272d72" memberName="densitySlider"
          virtualName="" explicitFocusOrder="0" pos="80 136 312 24" min="1"
          max="150" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="durationSlider" id="b0e1603976b0d90" memberName="durationSlider"
          virtualName="" explicitFocusOrder="0" pos="80 104 312 24" min="0.01"
          max="0.25" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="widthSlider" id="1f5d723d03076b81" memberName="widthSlider"
          virtualName="" explicitFocusOrder="0" pos="80 168 312 24" min="0"
          max="0.5" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="3e84926a93870273" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="8 40 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Rate" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="38a08ec893158b95" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="8 72 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Offset" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="aa59194dda9941d4" memberName="label7" virtualName=""
         explicitFocusOrder="0" pos="8 104 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Duration" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="4fb65844f12f6f30" memberName="label8" virtualName=""
         explicitFocusOrder="0" pos="8 136 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Density" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="a3782d96be6c4f81" memberName="label9" virtualName=""
         explicitFocusOrder="0" pos="8 168 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Position" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <SLIDER name="volumeSlider" id="7dff28396578f2ba" memberName="volumeSlider"
          virtualName="" explicitFocusOrder="0" pos="80 200 312 24" min="0"
          max="40" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="2fc7184ff00df0b1" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="8 200 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Volume" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <GENERICCOMPONENT name="VUMeterL" id="2d6560d1efd79b93" memberName="VUMeterL" virtualName=""
                    explicitFocusOrder="0" pos="6 296 16 144" class="VUMeter" params=""/>
  <GENERICCOMPONENT name="VUMeterR" id="a532c6b2609a9799" memberName="VUMeterR" virtualName=""
                    explicitFocusOrder="0" pos="26 296 16 144" class="VUMeter" params=""/>
  <SLIDER name="new slider" id="426f67d6484c3c25" memberName="envelopeSlider"
          virtualName="" explicitFocusOrder="0" pos="80 232 312 24" min="0.00001"
          max="1" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="7d42c91d70e1546f" memberName="reverbSlider"
          virtualName="" explicitFocusOrder="0" pos="80 264 310 24" min="0.8"
          max="0.99" int="0" style="LinearHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="78a51254642d2fdf" memberName="label5" virtualName=""
         explicitFocusOrder="0" pos="8 232 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Envelope" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="dc64f45ed78831c" memberName="label6" virtualName=""
         explicitFocusOrder="0" pos="8 264 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Reverb" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
