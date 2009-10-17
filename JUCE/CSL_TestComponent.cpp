/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  17 Sep 2009 3:29:44 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...

//[/Headers]

#include "CSL_TestComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...

using namespace csl;

// the CSL test suite lists

// These are structs that are declared in the individual CSL test suite files
//		struct testStruct { char * name;   void (* fcn)();  };
//	Each test suite has a list of its functions

extern testStruct oscTestList[];
extern testStruct srcTestList[];
extern testStruct envTestList[];
extern testStruct effTestList[];
extern testStruct panTestList[];
extern testStruct ctrlTestList[];

testStruct * allTests[] = {
	oscTestList,
	srcTestList,
	envTestList,
	effTestList,
	panTestList,
#ifdef USE_JMIDI
	ctrlTestList
#endif
};

char * allTestNames[] = {
	"Oscillator Tests",
	"Source Tests",
	"Envelope Tests",
	"Effect Tests",
	"Panner Tests",
#ifdef USE_JMIDI
	"Control Tests"
#endif
};

testStruct * gTestList;				// global menu

// Pretty-print the test menu

void dumpTestList() {
	printf("\nMenu List\n");
#ifdef USE_JMIDI
	unsigned numSuites = 6;
#else
	unsigned numSuites = 5;
#endif
	for (unsigned i = 0; i < numSuites; i++) {
		gTestList = allTests[i];
		char * testName = allTestNames[i];
		printf("\t%s\n", testName);
		for (unsigned j = 0; gTestList[j].name != NULL; j++) {
			printf("\t\t%s\n", gTestList[j].name);
		}
	}
}

// The main() function reads argv[1] as the suite selector (from the above list, 1-based)
// and argv[2] as the test within the suite, e.g., use "2 10" to select the 10th test in the
// srcTestList suite ("Vector IFFT" -- found at the bottom of Test_Sources.cpp).
// This is useful for debugging; set up the GUI to run the test you're working on.

extern IO * theIO;							// global IO object accessed by other threads
Label* gCPULabel;							// CPU % label...
AudioDeviceManager * gAudioDeviceManager;	// global JUCE audio device mgr
extern unsigned argCnt;						// globals for argc/v from cmd-line
extern char **argVals;

#define WRITE_TO_FILE						// support file recording
#ifdef WRITE_TO_FILE
	Buffer * gFileBuffer = 0;				// global buffer for file cache
	int gSampIndex = 0;						// write index into out buffer
#endif


// LThread run function spawns/loops its CThread and sleeps interruptably

void LThread::run() {
	while (1) {								// endless loop
		if (thr->isThreadRunning())			// sleep some
			if (csl::sleepMsec(250))		// interruptable sleep, check for failure
				goto turn_off;
		if (this->threadShouldExit())		// check flag
			goto turn_off;
		if ( ! thr->isThreadRunning()) {	// if off
			if (loop)
				thr->startThread();			// restart if looping
			else {
turn_off:		if (CGestalt::stopNow()) {			// if a timer was interrupted
					CGestalt::clearStopNow();		// clear the global flag
					thr->signalThreadShouldExit();
				}
				comp->playing = false;
				const MessageManagerLock mmLock;	// create a lock to call the GUI thread here
				if (comp->isTimerRunning())
					comp->stopTimer();				// nasty to do this from here, but it freezes the GUI
//				if (comp->playButton)
//					comp->playButton->setButtonText(T("Play"));
				if (comp->recrding)
					comp->recordOnOff();
				return;
			}
		}
	}
}

//[/MiscUserDefs]

//==============================================================================
CSLComponent::CSLComponent ()
    : playButton (0),
      quitButton (0),
      testCombo (0),
      oscilloscopeL (0),
      label (0),
      prefsButton (0),
      cpuLabel (0),
      oscilloscopeR (0),
      VUMeterL (0),
      VUMeterR (0),
      scaleSlider (0),
      amplitudeSlider (0),
      loopButton (0),
      familyCombo (0),
      recordButton (0)
{
    addAndMakeVisible (playButton = new TextButton (T("playNote")));
    playButton->setButtonText (T("Play/Stop"));
    playButton->addButtonListener (this);

    addAndMakeVisible (quitButton = new TextButton (T("quitAction")));
    quitButton->setButtonText (T("Quit"));
    quitButton->addButtonListener (this);

    addAndMakeVisible (testCombo = new ComboBox (T("test to run")));
    testCombo->setEditableText (false);
    testCombo->setJustificationType (Justification::centredLeft);
    testCombo->setTextWhenNothingSelected (String::empty);
    testCombo->setTextWhenNoChoicesAvailable (T("(no choices)"));
    testCombo->addListener (this);

    addAndMakeVisible (oscilloscopeL = new AudioWaveformDisplay());
    oscilloscopeL->setName (T("new component"));

    addAndMakeVisible (label = new Label (T("CSL test"),
                                          T("CSL 5 Demos")));
    label->setFont (Font (Font::getDefaultSerifFontName(), 30.0000f, Font::bold));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0xfffffc00));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (prefsButton = new TextButton (T("new button")));
    prefsButton->setButtonText (T("Audio Prefs"));
    prefsButton->addButtonListener (this);

    addAndMakeVisible (cpuLabel = new Label (T("new label"),
                                             T("0.0%")));
    cpuLabel->setFont (Font (Font::getDefaultSansSerifFontName(), 15.0000f, Font::bold));
    cpuLabel->setJustificationType (Justification::centredRight);
    cpuLabel->setEditable (false, false, false);
    cpuLabel->setColour (TextEditor::textColourId, Colours::black);
    cpuLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (oscilloscopeR = new AudioWaveformDisplay());
    oscilloscopeR->setName (T("new component"));

    addAndMakeVisible (VUMeterL = new VUMeter());
    VUMeterL->setName (T("new component"));

    addAndMakeVisible (VUMeterR = new VUMeter());
    VUMeterR->setName (T("new component"));

    addAndMakeVisible (scaleSlider = new Slider (T("new slider")));
    scaleSlider->setRange (0, 1, 0);
    scaleSlider->setSliderStyle (Slider::LinearHorizontal);
    scaleSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    scaleSlider->addListener (this);

    addAndMakeVisible (amplitudeSlider = new Slider (T("new slider")));
    amplitudeSlider->setRange (-5, 5, 0);
    amplitudeSlider->setSliderStyle (Slider::LinearVertical);
    amplitudeSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    amplitudeSlider->addListener (this);

    addAndMakeVisible (loopButton = new ToggleButton (T("new toggle button")));
    loopButton->setButtonText (T("Loop"));
    loopButton->addButtonListener (this);

    addAndMakeVisible (familyCombo = new ComboBox (T("test family")));
    familyCombo->setEditableText (false);
    familyCombo->setJustificationType (Justification::centredLeft);
    familyCombo->setTextWhenNothingSelected (String::empty);
    familyCombo->setTextWhenNoChoicesAvailable (T("(no choices)"));
    familyCombo->addItem (T("Oscillators"), 1);
    familyCombo->addItem (T("Sources"), 2);
    familyCombo->addItem (T("Envelopes"), 3);
    familyCombo->addItem (T("Effects"), 4);
    familyCombo->addItem (T("Panners"), 5);
#ifdef USE_JMIDI
    familyCombo->addItem (T("Controls"), 6);
#endif
    familyCombo->addListener (this);

    addAndMakeVisible (recordButton = new ToggleButton (T("new toggle button")));
    recordButton->setButtonText (T("Record"));
    recordButton->addButtonListener (this);


    //[UserPreSize]
	oscilloscopeL->initialise(0, 20, 2, true);		// channel, rate, window, zeroX
	oscilloscopeR->initialise(1, 20, 2, true);
    //[/UserPreSize]

    setSize (600, 400);

    //[Constructor] You can add your own custom stuff here..
					// and initialise the device manager with no settings so that it picks a
					// default device to use.
	const String error (mAudioDeviceManager.initialise (0,	/* no input */
													   2,	/* stereo output  */
													   0,	/* no XML defaults */
													   true /* select default device */));
	if (error.isNotEmpty())
		AlertWindow::showMessageBox (AlertWindow::WarningIcon,
									 T("CSL Demo"),
									 T("Couldn't open an output device!\n\n") + error);
										// get the audio device
	AudioIODevice* audioIO = mAudioDeviceManager.getCurrentAudioDevice();
	unsigned sRate = (unsigned) audioIO->getCurrentSampleRate();
	unsigned bufSize = audioIO->getCurrentBufferSizeSamples();

#ifdef READ_IO_PROPS					// overwrite the system frame rate and block size from the
										//  selected hardware interface at startup time
	theIO = new csl::IO(sRate, bufSize, -1, -1,			// still use the default # IO channels
					CGestalt::numInChannels(), CGestalt::numOutChannels());
#else									// reset the HW frame rate & block size to the CSL definition
	AudioDeviceManager::AudioDeviceSetup setup;
	mAudioDeviceManager.getAudioDeviceSetup(setup);
	setup.bufferSize = CGestalt::blockSize();
	setup.sampleRate = CGestalt::frameRate();
	mAudioDeviceManager.setAudioDeviceSetup(setup,true);
										// set up CSL IO
	theIO = new csl::IO(CGestalt::frameRate(), CGestalt::blockSize(), -1, -1,
					CGestalt::numInChannels(), CGestalt::numOutChannels());
#endif
	theIO->start();						// start IO and register callback
	mAudioDeviceManager.addAudioCallback(this);

	gCPULabel = cpuLabel;
	gAudioDeviceManager = & mAudioDeviceManager;
	playThread = 0;
	loopThread = 0;
	loop = false;
	playing = false;
	displayMode = true;
	recrding = false;

	amplitudeSlider->setValue(-0.2);
	scaleSlider->setValue(0.1);
	oscilloscopeL->start();
	oscilloscopeR->start();
	VUMeterL->setChannel(0);
	VUMeterR->setChannel(1);
    loopButton->setToggleState (false, false);

//	spectrogam->setVisible(false);

//	dumpTestList();					// print out the demo/test menu
	
	int whichSuite = 1;				// set default suite/test
	int whichTest = 1;
									// try to read init file
	string initMsg(CGestalt::initFileText('T'));
	if (initMsg.size() > 0) {
		sscanf(initMsg.c_str(), "%d %d", & whichSuite, & whichTest);
		printf("Select suite %d, test %d\n", whichSuite, whichTest);
	}
	if (argCnt > 1)					// cmd-line args select test suite and test
		whichSuite = atoi(argVals[1]);
	if (argCnt > 2)
		whichTest = atoi(argVals[2]);
	if (argCnt > 1)
		printf("Select suite %d, test %d\n", whichSuite, whichTest);

	familyCombo->setSelectedId(whichSuite, true);
	this->setComboLabels(whichSuite - 1);
	testCombo->setSelectedId(whichTest, true);

    //[/Constructor]
}

CSLComponent::~CSLComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

	playing = false;
	mAudioDeviceManager.removeAudioCallback(this);
	if (gFileBuffer) {
		gFileBuffer->freeBuffers();
		delete gFileBuffer;
	}

    //[/Destructor_pre]

    deleteAndZero (playButton);
    deleteAndZero (quitButton);
    deleteAndZero (testCombo);
    deleteAndZero (oscilloscopeL);
    deleteAndZero (label);
    deleteAndZero (prefsButton);
    deleteAndZero (cpuLabel);
    deleteAndZero (oscilloscopeR);
    deleteAndZero (VUMeterL);
    deleteAndZero (VUMeterR);
    deleteAndZero (scaleSlider);
    deleteAndZero (amplitudeSlider);
    deleteAndZero (loopButton);
    deleteAndZero (familyCombo);
    deleteAndZero (recordButton);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void CSLComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xffaeaeae));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CSLComponent::resized()
{
    playButton->setBounds (336, getHeight() - 39, 136, 32);
    quitButton->setBounds (getWidth() - 8 - 176, 8, 176, 32);
    testCombo->setBounds (152, getHeight() - 35, 176, 24);
    oscilloscopeL->setBounds (44, 48, getWidth() - 52, proportionOfHeight (0.3400f));
    label->setBounds (247 - ((158) / 2), 10, 158, 28);
    prefsButton->setBounds (8, 8, 144, 32);
    cpuLabel->setBounds (getWidth() - 64, getHeight() - 35, 56, 24);
    oscilloscopeR->setBounds (44, proportionOfHeight (0.5000f), getWidth() - 52, proportionOfHeight (0.3400f));
    VUMeterL->setBounds (25, 48, 15, roundFloatToInt ((proportionOfHeight (0.3400f)) * 1.0000f));
    VUMeterR->setBounds (24, proportionOfHeight (0.5000f), 15, roundFloatToInt ((proportionOfHeight (0.3400f)) * 1.0000f));
    scaleSlider->setBounds (8, getHeight() - 62, getWidth() - 12, 24);
    amplitudeSlider->setBounds (0, 43, 20, proportionOfHeight (0.7475f));
    loopButton->setBounds ((336) + 144, getHeight() - 11 - 24, 64, 24);
    familyCombo->setBounds (16, getHeight() - 35, 128, 24);
    recordButton->setBounds ((getWidth() - 8 - 176) + -2 - 72, 37 - 24, 72, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void CSLComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == playButton)
    {
        //[UserButtonCode_playButton] -- add your button handler code here..

		this->startStop();

        //[/UserButtonCode_playButton]
    }
    else if (buttonThatWasClicked == quitButton)
    {
        //[UserButtonCode_quitButton] -- add your button handler code here..

		if (playing)
			this->startStop();
		JUCEApplication::quit();

        //[/UserButtonCode_quitButton]
    }
    else if (buttonThatWasClicked == prefsButton)
    {
        //[UserButtonCode_prefsButton] -- add your button handler code here..

					// Create an AudioDeviceSelectorComponent which contains the audio choice widgets...
//		if (displayMode) {
			oscilloscopeL->stop();			// stop scope display during dialog
			oscilloscopeR->stop();
//		} else {
//			spectrogam->stop();
//		}
//		   AudioDeviceSelectorComponent (AudioDeviceManager& deviceManager,
//                                  const int minAudioInputChannels,
//                                  const int maxAudioInputChannels,
//                                  const int minAudioOutputChannels,
//                                  const int maxAudioOutputChannels,
//                                  const bool showMidiInputOptions,
//                                  const bool showMidiOutputSelector,
//                                  const bool showChannelsAsStereoPairs,
//                                  const bool hideAdvancedOptionsWithButton);

		AudioDeviceSelectorComponent audioSettingsComp (mAudioDeviceManager,
														CGestalt::numInChannels(), CGestalt::numInChannels(),
														CGestalt::numOutChannels(), CGestalt::numOutChannels(),
														true, true,
														false, false);
											// ...and show it in a DialogWindow...
		audioSettingsComp.setSize (500, 400);
		DialogWindow::showModalDialog (T("Audio Settings"),
									   &audioSettingsComp,
									   this,
									   Colours::azure,
									   true);
//		if (displayMode) {
			oscilloscopeL->start();			// stop scope display during dialog
			oscilloscopeR->start();
//		} else {
//			spectrogam->start();
//		}

        //[/UserButtonCode_prefsButton]
    }
    else if (buttonThatWasClicked == loopButton)
    {
        //[UserButtonCode_loopButton] -- add your button handler code here..

		if (loop) {
			if (loopThread) {
				loopThread->stopThread(100);
				delete loopThread;
				loopThread = 0;
			}
		}
		loop = ! loop;

        //[/UserButtonCode_loopButton]
    }
    else if (buttonThatWasClicked == recordButton)
    {
        //[UserButtonCode_recordButton] -- add your button handler code here..

		bool wasRec = recrding;
		recordOnOff();
		if (wasRec)
			recrding = false;

        //[/UserButtonCode_recordButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void CSLComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == testCombo)
    {
        //[UserComboBoxCode_testCombo] -- add your combo box handling code here..

//		printf("\tSelect test %d\n", testCombo->getSelectedId());

        //[/UserComboBoxCode_testCombo]
    }
    else if (comboBoxThatHasChanged == familyCombo)
    {
        //[UserComboBoxCode_familyCombo] -- add your combo box handling code here..

//		printf("\tSelect suite %d\n", familyCombo->getSelectedId());
		this->setComboLabels(familyCombo->getSelectedId() - 1);

        //[/UserComboBoxCode_familyCombo]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void CSLComponent::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == scaleSlider)
    {
        //[UserSliderCode_scaleSlider] -- add your slider handling code here..

		unsigned stepValue = (unsigned) (scaleSlider->getValue() * 20.0) + 1;
		oscilloscopeL->setSamplesToAverage(stepValue);
		oscilloscopeR->setSamplesToAverage(stepValue);

        //[/UserSliderCode_scaleSlider]
    }
    else if (sliderThatWasMoved == amplitudeSlider)
    {
        //[UserSliderCode_amplitudeSlider] -- add your slider handling code here..

		amplValue = pow(2.0, amplitudeSlider->getValue());

        //[/UserSliderCode_amplitudeSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

/////////////// CSL/JUCE Audio callback method ///////////////////

void CSLComponent::audioDeviceIOCallback (const float** inputChannelData,
							int totalNumInputChannels,
							float** outputChannelData,
							int totalNumOutputChannels,
							int numSamples) {
								// put silence in the output buffers
	for (unsigned i = 0; i < totalNumOutputChannels; i++)
		memset(outputChannelData[i], 0, numSamples * sizeof(float));

	if ( ! playing)				// if off
		return;
	if (CGestalt::stopNow())	// if being interrupted
		return;
								// set up CSL buffer object
	if (theIO->mGraph) {
		outBuffer.setSize(totalNumOutputChannels, numSamples);
								// copy JUCE data ptrs
		for (unsigned i = 0; i < totalNumOutputChannels; i++)
			outBuffer.mBuffers[i] = outputChannelData[i];

		try {					//////
								// Tell the IO to call its graph
								//////
			theIO->pullInput(outBuffer);

		} catch (csl::CException e) {
			logMsg(kLogError, "Error running CSL: graph: %s\n", e.what());
		}
		if (amplValue != 1.0f) {	// scale by volume slider
								// channel loop
			for (unsigned i = 0; i < totalNumOutputChannels; i++) {
				float * sampPtr = outputChannelData[i];
								// frame loop
				for (unsigned j = 0; j < numSamples; j++)
					*sampPtr++ *= amplValue;
		}}
								// pass the audio callback on to the waveform display components
		oscilloscopeL->audioDeviceIOCallback (inputChannelData, totalNumInputChannels,
								outputChannelData, totalNumOutputChannels, numSamples);
		oscilloscopeR->audioDeviceIOCallback (inputChannelData, totalNumInputChannels,
								outputChannelData, totalNumOutputChannels, numSamples);
		VUMeterL->audioDeviceIOCallback (inputChannelData, 0,
								outputChannelData, totalNumOutputChannels, numSamples);
		VUMeterR->audioDeviceIOCallback (inputChannelData, 0,
								outputChannelData, totalNumOutputChannels, numSamples);
//		spectrogam->audioDeviceIOCallback (inputChannelData, totalNumInputChannels,
//								outputChannelData, totalNumOutputChannels, numSamples);

		if (recrding) {			// cache to (stereo) record buffer
			if (gSampIndex >= SAMPS_TO_WRITE)
				return;
			if ( ! gFileBuffer)
				return;
								// get cache ptrs & copy outbuf
			sample * sPtr = gFileBuffer->mBuffers[0] + gSampIndex;
			memcpy(sPtr, outputChannelData[0], (numSamples * sizeof(sample)));
			sPtr = gFileBuffer->mBuffers[1] + gSampIndex;
			memcpy(sPtr, outputChannelData[1], (numSamples * sizeof(sample)));
								// update ptrs
			gSampIndex += numSamples;
			gFileBuffer->mNumFrames = gSampIndex;
		}
	} // if mGraph
}

// Handle file output in record mode; creates a remp file named "dataFolder(), OUT_SFILE_NAME"
//		such as /Users/stp/Code/CSL/CSL_Data/02_csl_out.aiff
// The template is XX_csl_out.aiff, could be like CSL_TakeXX.aiff.
// Increments the field XX in the name template with integers, wrapping at 100.
//

void CSLComponent::recordOnOff() {
		if (recrding) {					// stop recording and write output file
			if (gFileBuffer->mNumFrames == 0)
				return;
			string outName = CGestalt::sndFileName();
													// write to snd file
			logMsg("Write %5.3f sec (%d ksamp) to file \"%s\"\n",
					((float) gSampIndex / CGestalt::frameRate()), gSampIndex / 1000, outName.c_str());
			SoundFile * ioFile = new SoundFile(outName);
			ioFile->openForWrite(kSoundFileFormatAIFF, 2);
			ioFile->writeBuffer(*gFileBuffer);		// snd file write of record buffer
			ioFile->close();
			delete ioFile;

		} else {									// start recording
			if ( ! gFileBuffer) {					// allocate buffer first time
				gFileBuffer = new Buffer(2, SAMPS_TO_WRITE + CGestalt::maxBufferFrames());
				gFileBuffer->allocateBuffers();
			}
			recrding = true;						// set flag checked in audioDeviceIOCallback
		}
		gSampIndex = 0;								// reset write ptr
		gFileBuffer->mNumFrames = gSampIndex;
}

// Audio device support

void CSLComponent::audioDeviceAboutToStart (AudioIODevice* device) {
	oscilloscopeL->audioDeviceAboutToStart (device);
	oscilloscopeR->audioDeviceAboutToStart (device);
//	spectrogam->audioDeviceAboutToStart (device);
//	csl::CGestalt::setBlockSize(numSamplesPerBlock);
}

void CSLComponent::audioDeviceStopped() {
//	oscilloscope->audioDeviceStopped();
}

// Set up the combo box from the individual test suite methods

void CSLComponent::setComboLabels(unsigned which) {
	testCombo->clear();
	gTestList = allTests[which];
	for (unsigned i = 0; gTestList[i].name != NULL; i++) {
//		printf("\tAdd \"%s\"\n", gTestList[i].name);
		testCombo->addItem(String(gTestList[i].name), i + 1);
	}
	testCombo->setSelectedId(1);
}

// create the component -- called from main()

Component* createCSLComponent() {
    return new CSLComponent();
}

// timer call

void CSLComponent::timerCallback() {
	const MessageManagerLock mmLock;	// create the lock so we can call the GUI thread from this thread
	char msg[10];						// print the CPU usage
	sprintf(msg, "%5.2f%%", (gAudioDeviceManager->getCpuUsage() * 100.0));
	String msgS(msg);
	gCPULabel->setText(msgS, true);
}

void CSLComponent::startStop() {
	if ( ! playing) {										// if not playing, start!
		int which = testCombo->getSelectedId();
		if (which == 0)
			return;
		char initmsg[16];									// store test to file
		sprintf(initmsg, "%d %d", familyCombo->getSelectedId(), testCombo->getSelectedId());
		CGestalt::storeToInitFile('T', string(initmsg));
		
		playing = true;
		CGestalt::clearStopNow();							// clear flag
															// create a threadfcn that plays CSL
		ThreadFunc fcn = (void * (*)(void *)) gTestList[which - 1].fcn;

		playThread = new GThread(fcn);						// thread to call the CSL test function
		playThread->startThread();
		loopThread = new LThread(playThread, this, loop);	// thread to wait and/or loop it
		loopThread->startThread();
//			playButton->setButtonText (T("Stop"));
		this->startTimer(1000);
	} else {												// if playing
		playing = false;
		theIO->clearRoot();	
//			this->stopTimer();
		CGestalt::setStopNow();								// set flag to clear timers
		if (recrding)
			recordOnOff();
		sleepMsec(100);
		if (loop)
			loopThread->stopThread(1000);
		if (playThread->isThreadRunning())
			playThread->stopThread(1000);					// try to kill
		delete playThread;
		playThread = 0;
//			playButton->setButtonText (T("Play"));
		CGestalt::clearStopNow();		// clear flag
	}
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="CSLComponent" componentName=""
                 parentClasses="public Component, public AudioIODeviceCallback, public Timer"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="1" initialWidth="600"
                 initialHeight="400">
  <BACKGROUND backgroundColour="ffaeaeae"/>
  <TEXTBUTTON name="playNote" id="ed1811e776eea99b" memberName="playButton"
              virtualName="" explicitFocusOrder="0" pos="336 39R 136 32" buttonText="Play"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="quitAction" id="dbaf2871fd41de83" memberName="quitButton"
              virtualName="" explicitFocusOrder="0" pos="8Rr 8 176 32" buttonText="Quit"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <COMBOBOX name="test to run" id="bd1a5c541fbc8bc7" memberName="testCombo"
            virtualName="" explicitFocusOrder="0" pos="152 35R 176 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="new component" id="d64c351a292a43a4" memberName="oscilloscopeL"
                    virtualName="" explicitFocusOrder="0" pos="44 48 52M 34%" class="AudioWaveformDisplay"
                    params=""/>
  <LABEL name="CSL test" id="a9876f115ab3c22e" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="247c 10 158 28" textCol="fffffc00"
         edTextCol="ff000000" edBkgCol="0" labelText="CSL 5 Demos" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default serif font"
         fontsize="30" bold="1" italic="0" justification="33"/>
  <TEXTBUTTON name="new button" id="beeb15a1537fd4f6" memberName="prefsButton"
              virtualName="" explicitFocusOrder="0" pos="8 8 144 32" buttonText="Audio Prefs"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <LABEL name="new label" id="87d3c5b55ea75f76" memberName="cpuLabel"
         virtualName="" explicitFocusOrder="0" pos="64R 35R 56 24" edTextCol="ff000000"
         edBkgCol="0" labelText="0.0%" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default sans-serif font" fontsize="15"
         bold="1" italic="0" justification="34"/>
  <GENERICCOMPONENT name="new component" id="4aa0f216430fecde" memberName="oscilloscopeR"
                    virtualName="" explicitFocusOrder="0" pos="44 50% 52M 34%" class="AudioWaveformDisplay"
                    params=""/>
  <GENERICCOMPONENT name="new component" id="28308fd8ae783890" memberName="VUMeterL"
                    virtualName="" explicitFocusOrder="0" pos="25 48 15 100%" posRelativeH="d64c351a292a43a4"
                    class="VUMeter" params=""/>
  <GENERICCOMPONENT name="new component" id="643b07b4b6cf41d" memberName="VUMeterR"
                    virtualName="" explicitFocusOrder="0" pos="24 50% 15 100%" posRelativeH="4aa0f216430fecde"
                    class="VUMeter" params=""/>
  <SLIDER name="new slider" id="c62ea84a7afde2e3" memberName="scaleSlider"
          virtualName="" explicitFocusOrder="0" pos="8 62R 12M 24" min="0"
          max="1" int="0" style="LinearHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="78c6bd39da739ba6" memberName="amplitudeSlider"
          virtualName="" explicitFocusOrder="0" pos="0 43 20 74.75%" min="-5"
          max="5" int="0" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="new toggle button" id="2322f603f1796f48" memberName="loopButton"
                virtualName="" explicitFocusOrder="0" pos="144 11Rr 64 24" posRelativeX="ed1811e776eea99b"
                buttonText="Loop" connectedEdges="0" needsCallback="1" radioGroupId="0"
                state="0"/>
  <COMBOBOX name="test family" id="55af3a98f2faf643" memberName="familyCombo"
            virtualName="" explicitFocusOrder="0" pos="16 35R 128 24" editable="0"
            layout="33" items="Oscillators&#10;Sources&#10;Envelopes&#10;Effects&#10;Panners&#10;Controls"
            textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="new toggle button" id="82a5a3f16d517231" memberName="recordButton"
                virtualName="" explicitFocusOrder="0" pos="-2r 37r 72 24" posRelativeX="dbaf2871fd41de83"
                buttonText="Record" connectedEdges="0" needsCallback="1" radioGroupId="0"
                state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
