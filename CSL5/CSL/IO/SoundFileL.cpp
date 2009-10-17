//
//  SoundFileL.cpp -- sound file class using libsndfile
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include "SoundFileL.h"
#include "math.h"

using namespace csl;

// LSoundFile Constructors

LSoundFile::LSoundFile(string tpath, int tstart, int tstop)
			: Abst_SoundFile(tpath, tstart, tstop), 
			  mSFInfo(new SF_INFO), 
			  mSndfile(NULL) {
	mSFInfo->format = NULL;
	mNumFrames = 0;
#ifdef CSL_USE_SRConv
	mSRateConv = NULL;
#endif
}

LSoundFile::LSoundFile(string folder, string tpath, int start, int stop)
			: Abst_SoundFile(folder, tpath, start, stop),
			  mSFInfo(new SF_INFO), 
			  mSndfile(NULL) {
	printf("SF: \"%s\"\n", mPath.c_str());
	mSFInfo->format = NULL;
	mNumFrames = 0;
#ifdef CSL_USE_SRConv
	mSRateConv = NULL;
#endif
}

// Copy constructor -- shares sample buffer

LSoundFile::LSoundFile(LSoundFile & otherSndFile)
			: Abst_SoundFile(otherSndFile), 
			  mSFInfo(otherSndFile.sfInfo()), 
			  mSndfile(otherSndFile.sndFile()) {
	if ( ! otherSndFile.isCached())
		logMsg(kLogError, "Cannot copy uncached sound file \"%s\"", mPath.c_str());
//	logMsg("Open sound file \"%s\"", mPath.c_str());
#ifdef CSL_USE_SRConv
	mSRateConv = NULL;
#endif
}

// Clean up data allocated

LSoundFile::~LSoundFile() {
	if (mSndfile)
		sf_close(mSndfile);
	freeBuffer();
#ifdef CSL_USE_SRConv
	if (mSRateConv)
		src_delete (mSRateConv);
#endif
}

//
// set up the receiver's variables based on the file
//

void LSoundFile::initFromSndfile() {
	mIsValid = (mSndfile != NULL);
	if ( ! mIsValid) {
//		logMsg(kLogError, "Cannot open sound file \"%s\"", mPath.c_str());
		return;
	}
	mFrameRate = (unsigned) mSFInfo->samplerate;
	mNumChannels = (unsigned) mSFInfo->channels;
	mNumFrames = (unsigned) mSFInfo->frames;
	switch (mSFInfo->format & 0x0f) {
		case 1:		mBytesPerSample = 1;	break;			// Signed 8 bit
		case 2:		mBytesPerSample = 2;	break;			// Signed 16 bit
		case 3:		mBytesPerSample = 3;	break;			// Signed 24 bit
		case 4:		mBytesPerSample = 4;	break;			// Signed 32 bit
		case 5:		mBytesPerSample = 1;	break;			// unsigned 8 bit
		case 6:		mBytesPerSample = 4;	break;			// 32 bit float
		case 7:		mBytesPerSample = 8;	break;			// 64 bit float
	}
	if (mStart > 0)		seekTo(mStart);
	else				mStart = 0;
	if (mStop < 0)		mStop = mNumFrames;
	
#ifdef CSL_USE_SRConv
	if (mMode == kSoundFileRead)	{							// sample rate convertor (SRC) state struct
		mSRateConv = src_new (CSL_SRC_MODE, mNumChannels, & mSRCReturn);
		if (mSRateConv == NULL)
			logMsg (kLogError, "SampleRateConvertor creation error : %s\n", src_strerror (mSRCReturn));
	}
#endif
}

void LSoundFile::checkBuffer(unsigned numFrames) {
	Abst_SoundFile::checkBuffer(numFrames);
#ifdef CSL_USE_SRConv
	if ( ! mSRConvBuffer.mAreBuffersAllocated) {					// if no SRC buffer allocated
		mSRConvBuffer.setSize(1, CGestalt::maxBufferFrames() * mNumChannels);
		mSRConvBuffer.allocateBuffers();
	}
#endif
}

SoundFileFormat LSoundFile::format() {
	switch (mSFInfo->format) {
		case SF_FORMAT_WAV:
			return kSoundFileFormatWAV;	break;
		case SF_FORMAT_AIFF:
			return kSoundFileFormatAIFF;	break;
		case SF_FORMAT_AU:
			return kSoundFileFormatSND;	break;
		case SF_FORMAT_RAW:
			return kSoundFileFormatRaw;	break;
		default:
			logMsg("Error: Couldn't get the file format.");
	}
	return kSoundFileFormatRaw;
}

// ~~~~ Open functions ~~~~

void LSoundFile::openForRead() throw (CException) {
	mMode = kSoundFileRead;
	mSndfile = sf_open(mPath.c_str(), SFM_READ, mSFInfo);		// libsndfile open call
	if (mSndfile == NULL) {										// check result
		const char *openError = sf_strerror(mSndfile);
		logMsg(kLogError, "Cannot open sound file \"%s\"\n\t\"%s\"", mPath.c_str(), openError);
		throw IOError("Sound file open error");
	}
	this->initFromSndfile();
	
	if (mNumFrames <= CGestalt::maxSndFileFrames()) {			// read file if size < global max
//		logMsg("Open/read sound file \"%s\" %d frames %g sec %d channels", 
//				mPath.c_str(), duration(), durationInSecs(), channels());		
		this->readBufferFromFile(mNumFrames);					// read entire file, de-interleave
		
		mCurrentFrame = mStart;
//		this->setWaveform(mSampleBuffer);						// set up oscillator pointers
		mWavetable.mDidIAllocateBuffers = true;
//		if (this->isCached())
//			logMsg("\t\tSound file cached");
	}
}

void LSoundFile::openForWrite(SoundFileFormat tformat, unsigned tchannels, unsigned rate, unsigned bitDepth) 
							throw (CException) {
	mMode = kSoundFileWrite;
	mSFInfo->samplerate = rate;
	mSFInfo->channels = tchannels;
	switch (tformat) {
		case kSoundFileFormatWAV:
			mSFInfo->format = SF_FORMAT_WAV;	break;
		case kSoundFileFormatAIFF:
			mSFInfo->format = SF_FORMAT_AIFF;		break;
		case kSoundFileFormatSND:
			mSFInfo->format = SF_FORMAT_AU;		break;
		case kSoundFileFormatRaw:
		default:
			mSFInfo->format = SF_FORMAT_RAW;	break;
	}
	if (bitDepth == 16)
		mSFInfo->format = mSFInfo->format | SF_FORMAT_PCM_16;
	else if (bitDepth == 32)
		mSFInfo->format = mSFInfo->format | SF_FORMAT_FLOAT;
	else
		logMsg("Invalid bitDepth for sound file %s. Use either 16 or 32 bits.", mPath.c_str());

	if ( ! sf_format_check(mSFInfo)) {
		logMsg(kLogError, "Invalid format for sound file %s", mPath.c_str());
		mIsValid = false;
		return;
	}
	mSndfile = sf_open(mPath.c_str(), SFM_WRITE, mSFInfo);
	initFromSndfile();
}

void LSoundFile::openForReadWrite() throw (CException) {
	mMode = kSoundFileReadWrite;
	mSndfile = sf_open(mPath.c_str(), SFM_RDWR, mSFInfo);
	initFromSndfile();
}

void LSoundFile::close() {
	sf_close(mSndfile);
	mSndfile = NULL;
	freeBuffer();
}

// file seek

unsigned LSoundFile::seekTo(int position, SeekPosition whence) throw(CException) {
	int whenceInt;
	if (this->isCached())
		return mCurrentFrame;
	switch (whence) {
		case kPositionStart: whenceInt = SEEK_SET; break;
		case kPositionCurrent: whenceInt = SEEK_CUR; break;
		case kPositionEnd: whenceInt = SEEK_END; break;
		default: 
			whenceInt = SEEK_CUR;
			logMsg("Error: Invalid position seek flag. Used kPositionCurrent.");			
			break;
	}
	mCurrentFrame =  sf_seek(mSndfile, position, whenceInt);
	return mCurrentFrame;
}

///////////////// next_buffer -- the work is done here //////////

void LSoundFile::nextBuffer(Buffer &outputBuffer) throw(CException) {
	unsigned numFrames = outputBuffer.mNumFrames;
	unsigned currentFrame = mCurrentFrame;
	DECLARE_SCALABLE_CONTROLS;							// declare the scale/offset buffers and values

	if (currentFrame >= (unsigned) mStop) {			// if done
		outputBuffer.zeroBuffers();
		return;
	}
	if (currentFrame + numFrames >= (unsigned) mStop) {		// if final window
		numFrames = mStop - currentFrame;
		outputBuffer.zeroBuffers();
	}
	if ( ! this->isCached()) {						// if not playing from cache buffer
		this->readBufferFromFile(numFrames);		// read from file
	}
	LOAD_SCALABLE_CONTROLS;							// load the scaleC and offsetC from the constant or dynamic value
	if (mRate == 1.0) {								// if playing at normal rate
//		unsigned numBytes = numFrames * sizeof(sample);
													// buffer copy loop
//		for (unsigned i = 0; i < outputBuffer.mNumChannels; i++) {
//			dataOutPtr = mWavetable.monoBuffer(csl_min(i, (mNumChannels - 1))) + currentFrame;
//			memcpy(outputBuffer.monoBuffer(i), dataOutPtr, numBytes);
//		}
		sample samp;
		for (unsigned i = 0; i < outputBuffer.mNumChannels; i++) {
			SampleBuffer buffer = outputBuffer.monoBuffer(i);		// get pointer to the selected output channel
			SampleBuffer dPtr = mWavetable.monoBuffer(csl_min(i, (mNumChannels - 1))) + currentFrame;

			for (unsigned i = 0; i < numFrames; i++) {			// sample loop
				samp = (*dPtr++ * scaleValue) + offsetValue; 	// get and scale the file sample
				*buffer++ = samp;
				UPDATE_SCALABLE_CONTROLS;						// update the dynamic scale/offset
			}
			scalePort->resetPtr();
			offsetPort->resetPtr();
		}
		currentFrame += numFrames;					// incrememnt buf ptr
	} else {						// if mRate != 1.0, 
	
#ifndef CSL_USE_SRConv				// normal case: use wavetable interpolation
		for (unsigned i = 0; i < mNumChannels; i++)
			WavetableOscillator::nextBuffer(outputBuffer, i);	
		currentFrame += numFrames;					// incrememnt buf ptr
		
#else								// else use Sample rate convertor
		int answer;									// if we're doing SRC transposition
		if (this->isCached()) 						// if playing from cache buffer
			mSRateData.data_in = mWavetable.monoBuffer(0) + (currentFrame * mNumChannels);	
		else
			mSRateData.data_in = mWavetable.monoBuffer(0);	
		mSRateData.data_out = mSRateData.data_in;	// store for later
		mSRateData.input_frames = numFrames;	
		mSRateData.output_frames = numFrames;	
		mSRateData.data_out = mSRConvBuffer.monoBuffer(0);	
		mSRateData.end_of_input = 0;	
		mSRateData.src_ratio = mRate;	
		if (answer = src_set_ratio (mSRateConv, mRate))			// set the transposition ratio
			logMsg (kLogError, "SampleRateConvertor configuration error : %s\n", 
						src_strerror (answer));
						
		if (answer = src_process (mSRateConv, & mSRateData))	// run SampleRateConvertor
			logMsg (kLogError, "SampleRateConvertor runtime error : %s\n", 
						src_strerror (answer));
						
		mSRateData.data_out = mSRConvBuffer.monoBuffer(0);		// reset out ptr
		currentFrame += mSRateData.input_frames_used;			// input frames actually read
		numFrames = mSRateData.output_frames_gen;				// output frames actually written
#endif
	}
	if ((currentFrame >= (unsigned) mStop) && mIsLooping)	// if we are past the end of the file...
		currentFrame = 0;						// this will click, have to call nextBuffer() recursively here
	mCurrentFrame = currentFrame;				// store back to member
	return;
}

// write a CSL buffer to the interleaved output file

void LSoundFile::writeBuffer(Buffer &inputBuffer) throw(CException) {
	unsigned numFrames = inputBuffer.mNumFrames;
	if (mSFInfo->channels > 1) {				// interleave stereo-to-mono
		mWavetable.setSize(1, mNumChannels * numFrames);
		mWavetable.allocateBuffers();
												// call interleaver
		mInterleaver.interleave(inputBuffer, mWavetable.monoBuffer(0), 
					numFrames, mSFInfo->channels);
		sf_writef_float(mSndfile, mWavetable.monoBuffer(0), numFrames);		// libsndfile write 
	} else {									// mono file write
		sf_writef_float(mSndfile, inputBuffer.monoBuffer(0), numFrames);	// libsndfile write 
	}
	mCurrentFrame += numFrames;
	return;
}

// read some samples from the file into the temp buffer

void LSoundFile::readBufferFromFile(unsigned numFrames) {
	sf_count_t numFramesRead;
	unsigned currentFrame = mCurrentFrame;
	unsigned myChannels = mSFInfo->channels;

	this->checkBuffer(numFrames);			// check my buffer, allocate if necessary
	SampleBuffer sampleBufferPtr;
	if ((myChannels > 1)) {					// read into temp buffer to multichannel files, then de-interleave
		SAFE_MALLOC(sampleBufferPtr, sample, (myChannels * numFrames));
	} else {
		sampleBufferPtr = mWavetable.monoBuffer(0);
	}
						// if we are at the end of the file and not looping
	if ((currentFrame >= (unsigned) mStop) && !mIsLooping) {
		memset(sampleBufferPtr, 0, numFrames * myChannels * sizeof(sample));
		return;
	}
	numFramesRead = sf_readf_float(mSndfile, sampleBufferPtr, numFrames);	// libsndfile read function
	currentFrame += numFramesRead;	
											// if we are past the end of the file...
	if (currentFrame > (unsigned) mStop) {
		unsigned numFramesRemaining = numFrames - numFramesRead;
		SampleBuffer tempBufferPtr = sampleBufferPtr + (numFramesRead * myChannels);
		if (mIsLooping) {													// loop back to start of file
			while (numFramesRead < numFrames) {
				currentFrame = sf_seek(mSndfile, 0, SEEK_SET);				// libsndfile seek/read function
				numFramesRead += sf_readf_float(mSndfile, tempBufferPtr, numFramesRemaining);
				currentFrame += numFramesRead;
			}
		} else {
			unsigned bytesToClear = numFramesRemaining * myChannels * sizeof(sample);
			memset(tempBufferPtr, 0, bytesToClear);
		}
	}
	if ((myChannels > 1)) {					// auto-de-interleave if multichannel
		mInterleaver.deinterleave(mWavetable, sampleBufferPtr, numFrames, mNumChannels);
		SAFE_FREE(sampleBufferPtr);
	}
	mCurrentFrame = currentFrame;
}

#ifdef CSL_USE_SRConv						// sample-rate conversion

#define	BUFFER_LEN		4096	/*-(1<<16)-*/

// fcn prototypes below

static sf_count_t sample_rate_convert (SNDFILE *infile, SNDFILE *outfile, 
									int converter, double src_ratio, int channels, double * gain);
		
static double apply_gain (float * data, long frames, int channels, double max, double gain);

// perform sample-rate conversion

void LSoundFile::convertRate(char * mTempPath, int fromRate, int toRate) {
	SNDFILE	*infile, *outfile;
	SF_INFO sfinfo;
	sf_count_t count;
	char tempName[CSL_NAME_LEN];
	
	double src_ratio = -1.0, gain = 1.0;
	int converter;
	converter = SRC_SINC_BEST_QUALITY;
	logMsg("Convert-rate of \"%s\" from %d to %d", mTempPath, fromRate, toRate);
												// create temp name and rename file
	sprintf(tempName, "%s.TEMP", mTempPath);
	if (rename(mTempPath, tempName) != 0) {
		logMsg(kLogError, "Convert-rate cannot rename \"%s\" to \"%s\"", mTempPath, tempName);
		perror("SoundFile::convertRate");
		return;
	}
	infile = sf_open(tempName, SFM_READ, &sfinfo);
	src_ratio = (double) toRate / (double) fromRate;
	sfinfo.samplerate = toRate;
	if (src_is_valid_ratio (src_ratio) == 0) {
		logMsg (kLogError, "Error: Sample rate change out of valid range.");
		sf_close (infile);
		return;
	}
	outfile = sf_open (mTempPath, SFM_WRITE, &sfinfo);
	if (outfile == NULL) {
		logMsg (kLogError, "Error: Cannot open output file.");
		sf_close (infile);
		return;
	}
	sf_command (outfile, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE);
	sf_command (outfile, SFC_SET_CLIPPING, NULL, SF_TRUE);
	do {
		count = sample_rate_convert(infile, outfile, converter, src_ratio, sfinfo.channels, &gain);
	} while (count < 0);

	logMsg("Convert-rate: output frames %ld", (long) count);
	sf_close(infile);
	sf_close(outfile);
	if (remove(tempName) != 0)
		logMsg(kLogError, "Convert-rate cannot remove temp file \"%s\"\n", tempName);
}

/////////////////////////
// These 2 functions are taken from libSampleRate
//

static sf_count_t sample_rate_convert (SNDFILE *infile, SNDFILE *outfile, int converter, 
				double src_ratio, int channels, double * gain) {
	static float input [BUFFER_LEN];
	static float output [BUFFER_LEN];
	SRC_STATE * src_state;
	SRC_DATA src_data;
	int	error;
	double max = 0.0;
	sf_count_t	output_count = 0;
	sf_seek (infile, 0, SEEK_SET);
	sf_seek (outfile, 0, SEEK_SET);

	/* Initialize the sample rate converter. */
	if ((src_state = src_new (converter, channels, &error)) == NULL) {	logMsg ("Error : src_new() failed : %s.", src_strerror (error));
		exit (1);
		};

	src_data.end_of_input = 0; /* Set this later. */

	/* Start with zero to force load in while loop. */
	src_data.input_frames = 0;
	src_data.data_in = input;
	src_data.src_ratio = src_ratio;
	src_data.data_out = output;
	src_data.output_frames = BUFFER_LEN /channels;

	while (1) {
		/* If the input buffer is empty, refill it. */
		if (src_data.input_frames == 0) {
			src_data.input_frames = sf_readf_float (infile, input, BUFFER_LEN / channels);
			src_data.data_in = input;

			/* The last read will not be a full buffer, so snd_of_input. */
			if (src_data.input_frames < BUFFER_LEN / channels)
				src_data.end_of_input = SF_TRUE;
			}

		if ((error = src_process (src_state, &src_data))) {
			logMsg ("SRC Error : %s", src_strerror (error));
			return (0);
			}

		/* Terminate if done. */
		if (src_data.end_of_input && src_data.output_frames_gen == 0)
			break;

		max = apply_gain (src_data.data_out, src_data.output_frames_gen, channels, max, *gain);

		/* Write output. */
		sf_writef_float (outfile, output, src_data.output_frames_gen);
		output_count += src_data.output_frames_gen;

		src_data.data_in += src_data.input_frames_used * channels;
		src_data.input_frames -= src_data.input_frames_used;
		}

	src_state = src_delete (src_state);

	if (max > 1.0) {	*gain = 1.0 / max;
		logMsg ("Output has clipped. Restarting conversion to prevent clipping.");
		output_count = 0;
		sf_command (outfile, SFC_FILE_TRUNCATE, &output_count, sizeof (output_count));
		return -1;
		}

	return output_count;
} /* sample_rate_convert */

static double apply_gain (float * data, long frames, int channels, double max, double gain) {
	for (long k = 0; k < frames * channels; k++) {
		double val = (double) data [k];
		val *= gain;
		data[k] = val;
		val = fabs (val);
		if (val > max)
			max = val;
	}
	return max;
} /* apply_gain */

#endif // SR_Conv

//
//////////////////// MP3File implementation
//

#ifdef USE_MP3

#include "mad.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

// Should I store the intermediate file as 32-bit floats or 16-bit shorts?

//#define CSL_MP3_AS_FLOATS
// else CSL_MP3_AS_INTS

// simple vector struct

struct vbuffer {
	unsigned char const * start;
	unsigned long length;
};

// The following utility routine performs rounding to MP3_SAMP_TYPE

#ifdef CSL_MP3_AS_FLOATS

static double samp_scale = 0.01 / (double) (1L << MAD_F_FRACBITS);

static inline float scale_MP3(mad_fixed_t sample) {
	double fsample = sample;
	return (float) (fsample / samp_scale);
}

#define MP3_SAMP_TYPE float
#define MP3_WRITE sf_writef_float
#define MP3_DEPTH 32

#else // CSL_MP3_AS_INTS

#define MP3_SAMP_TYPE short

// static double samp_scale = (1L << MAD_F_FRACBITS) + 0.5;
//
//static inline int scale_MP3(mad_fixed_t sample) {
//	double fsample = SampleBuffer samp_scale;
//	return (int) fsample;
//}

static inline MP3_SAMP_TYPE scale_MP3(mad_fixed_t sample) {
	sample += (1L << (MAD_F_FRACBITS - 16));		// round
	if (sample >= MAD_F_ONE)						// clip
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;
	return (MP3_SAMP_TYPE) (sample >> (MAD_F_FRACBITS + 1 - 16));		// quantize
}

#define MP3_WRITE sf_writef_short
#define MP3_DEPTH 16

#endif

// temp buffer & file for writing output

static MP3_SAMP_TYPE * xlate_buffer = NULL;
static SNDFILE * sfOutput = NULL;

void close_file(int fildes) {
	close(fildes);
}


// This is the input callback. The purpose of this callback is to (re)fill
// the stream buffer which is to be decoded.

static enum mad_flow m3_input(void * data, struct mad_stream * stream) {
	struct vbuffer * buffer = (vbuffer *) data;
	if ( ! buffer->length)
		return MAD_FLOW_STOP;
	mad_stream_buffer(stream, buffer->start, buffer->length);
	buffer->length = 0;
	return MAD_FLOW_CONTINUE;
}

// This is the output callback function; it scales and writes the decoder output to an AIFF file.

static enum mad_flow m3_output(void * data, struct mad_header const * header, struct mad_pcm * pcm) {
	unsigned int nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;
						// pcm->samplerate contains the sampling frequency
	nchannels = pcm->channels;
	nsamples  = pcm->length;				// normally 1152
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	MP3_SAMP_TYPE * out_ptr = xlate_buffer;
	while (nsamples--) {
		*out_ptr++ = scale_MP3(*left_ch++);
		if (nchannels == 2)
			*out_ptr++ = scale_MP3(*right_ch++);
	}
	MP3_WRITE(sfOutput, xlate_buffer, pcm->length);		// libsndfile write function

	return MAD_FLOW_CONTINUE;
}

// This is the error callback function. It is called whenever a decoding
// error occurs. The error is indicated by stream->error; the list of
// possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
// header file.

static enum mad_flow m3_error(void *data, struct mad_stream *stream, struct mad_frame *frame) {
//	struct vbuffer * buffer = (vbuffer *) data;
//  fprintf(stderr, "\tMP3 decoding error 0x%04x (%s) at byte offset %u\n",
//	  stream->error, mad_stream_errorstr(stream),
//	  stream->this_frame - buffer->start);
	return MAD_FLOW_CONTINUE;
}

// Header handler

// make the MP3File global as a hack for this next trick...

MP3File * gMP3FilePtr;

static enum mad_flow m3_header(void *data, struct mad_header const *header) {
//	printf("SR: %d\n", header->samplerate);
	gMP3FilePtr->mMP3Rate = header->samplerate;
	return MAD_FLOW_CONTINUE;
}

// This is the function that performs all the decoding.
// It instantiates a decoder object and configures it with the input,
// output, and error callback functions above. A single call to
// mad_decoder_run() continues until a callback function returns
// MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
// signal an error).
//
// Note that this assumes:
//		a stereo input file
//		a file name with a "/" and a ".mp3" 

int MP3File::decodeMP3() throw (CException) {						
	int inFile;
	struct stat stat;
//	printf("\tMP3File::decodeMP3() \"%s\"n", mPath.c_str());
	inFile = open(mPath.c_str(), O_RDONLY);		// open & test in file
	if (fstat(inFile, &stat) == -1 || stat.st_size == 0)
		throw IOError("Sound file MP3 open error");
												// memory-map input file -- ToDo: this is UNIX-specific
	void * mapped_data = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, inFile, 0);

	mTempPath = new char[CSL_LINE_LEN];			// create temp file name
	if (mTempDir)
		MP3_TEMP_NAME(mPath.c_str(), mTempPath, mTempDir)
	else
		AIFF_TEMP_NAME(mPath.c_str(), mTempPath)
												// create output sound file
	mTempFile = new LSoundFile(mTempPath);
	mTempFile->openForWrite(kSoundFileFormatAIFF, 2, CGestalt::frameRate(), MP3_DEPTH);

	struct vbuffer buffer;						// now set up libMAD decoder
	struct mad_decoder decoder;
	int result;
	buffer.start  = (unsigned char const *) mapped_data;
	buffer.length = stat.st_size;
												// set up the statics
	sfOutput = mTempFile->sndFile();
	xlate_buffer = new MP3_SAMP_TYPE[CGestalt::maxBufferFrames()];
	gMP3FilePtr = this;							// make me global in case I need to use me in the call-backs
	
	mad_decoder_init(&decoder, &buffer,			// configure input, output, and error functions
					 m3_input, 
					 m3_header, /* header */
					 0 /* filter */, 
					 m3_output,
					 m3_error, 
					 0 /* message */);					 
												// start decoding 
	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
												// release the decoder 
	mad_decoder_finish(&decoder);
#ifdef CSL_DEBUG
	logMsg("MP3 decoder returned %d; temp file %s", result, tempPath);
#endif	
	mTempFile->close();
	delete mTempFile;
	munmap(mapped_data, stat.st_size);
//	this->closef(inFile);
	close_file(inFile);
	return result;
}

/// MP3File constructors

MP3File::MP3File(string path, int start, int stop)
		: LSoundFile(path, start, stop) { 
	mTempDir = NULL;
	mMP3Rate = DEFAULT_MP3_RATE;			// default rate
//	printf("\tMP3File::c'tor \"%s\" \n", path.c_str());
}

MP3File::~MP3File() { }

/// openForRead decodes MP3 file into a temp file and plugs that into the receiver

void MP3File::openForRead() throw (CException) {
//	printf("\tMP3File::openForRead() \"%s\"n", mTempPath);
	this->decodeMP3();					// decode the MP3 using libMAD
	if (mMP3Rate != DEFAULT_MP3_RATE) {
		if ((mMP3Rate != 48000) && (mMP3Rate != 32000)) {
			logMsg(kLogError, "Unsupported MP3 file sample rate: %d", mMP3Rate);
			throw IOError("Unsupported MP3 file sample rate");
		}
		this->convertRate(mTempPath, mMP3Rate, DEFAULT_MP3_RATE);
	}
	mTempFile = new LSoundFile(mTempPath);
	mTempFile->openForRead();
	mSFInfo = mTempFile->sfInfo();	
	mSndfile = mTempFile->sndFile();	
	mMode = mTempFile->mode();
	mStart = mTempFile->startFrame();
	mStop = mTempFile->stopFrame();
	mIsValid = mTempFile->isValid();
	mNumChannels = mTempFile->numChannels();
//	mNumFrames = mTempFile->mNumFrames;
	mWavetable.copyFrom(mTempFile->mWavetable);
}

void MP3File::openForWrite(SoundFileFormat format, unsigned channels, unsigned rate, unsigned bitDepth) 
			throw (CException) {
	throw IOError("MP3 files cannot be written");
}

void MP3File::openForReadWrite() throw (CException) {
	throw IOError("MP3 files cannot be written");
}

#endif // USE_MP3
