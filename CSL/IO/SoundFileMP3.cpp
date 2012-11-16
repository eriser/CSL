//
//  SoundFileMP3.cpp -- MP3File implementation
//	See the copyright notice and acknowledgment of authors in the file COPYRIGHT
//

#include "SoundFileMP3.h"
#include "math.h"

#ifdef USE_libMAD

using namespace csl;

#define DEFAULT_MP3_RATE 44100

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

#define scale_MP3F(v) (((float) scale_MP3(v)) / 32768.0f) 

//
// DecodedFile
//

// constructor

DecodedFile::DecodedFile(string tpath, int start, int stop, float maxDurInSecs)
		: LSoundFile(tpath, start, stop, false, maxDurInSecs) {
	// no-op
}

// Desctructor

DecodedFile::~DecodedFile() {
	if ( ! mBuffers.empty())
		mBuffers.clear();				// erase old values
}

// DecodedFile::checkBufferStack - make certain the buffer can hold num_samples frames

bool DecodedFile::checkBufferStack(unsigned num_samples) {
	if (mBuffers.empty()) {
		return this->addBuffer(mNumChannels, CGestalt::sndFileFrames());
	} else {
		Buffer * bf = mBuffers.back();							// get buffer from vector in data struct
		if ((bf->mNumFrames + num_samples) > bf->mNumAlloc) {	// if it's full
			return this->addBuffer(mNumChannels, CGestalt::sndFileFrames());
		}
	}
    return true;
}

// Add a new buffer to the buffer vector

bool DecodedFile::addBuffer(unsigned nnumChannels, unsigned nnumFrames) {
//#ifdef FMAK_ANALYZER
//	unsigned totS = this->totalSize();
//	unsigned maxSongLength = gConfig->iValue(kMaxSongLength) * mFrameRate;
//	
//    if (totS > maxSongLength) {
//	    // numFrames = maxSongLength; 
//		return false;
//		// LMS used to throw a runtime error, which is a rather useless response.
//		// throw RunTimeError("Sound file too long");
//    }
//#endif
	Buffer * bf2 = new Buffer(nnumChannels, nnumFrames);
	bf2->allocateBuffers();
	bf2->mNumFrames = 0;
	mBuffers.push_back(bf2);						// push new buffer onto vector
	return true;
}

/// store samples into the receiver, adding another buffer if necessary

bool DecodedFile::writeBuffer (float * L_buffer, float * R_buffer, unsigned num_samples) {
	sample * outL, * outR;
	bool retVal = true;

	if (!this->checkBufferStack(num_samples))
	    return false;
	Buffer * bf = mBuffers.back();						// get buffer from vector
	
	outL = bf->buffer(0) + bf->mNumFrames;				// get prts for copy loop
	unsigned nsamples = num_samples;
	if (mMaxDurInSecs > 0.0) {										// check for overflow of mMaxDurInSecs
		unsigned tot = this->totalSize();
		unsigned maxFrames = (unsigned) (mMaxDurInSecs * mFrameRate);
		if ((tot + nsamples) >= maxFrames) {
			retVal = false;
			nsamples = maxFrames - tot;
		}
	}
	bf->mNumFrames += nsamples;							// increment buf ctr
	memcpy(L_buffer, outL, (nsamples * sizeof(sample)));		// do memcpy
	if (mNumChannels == 2) {
		outR = bf->buffer(1) + bf->mNumFrames;
		memcpy(R_buffer, outL, (nsamples * sizeof(sample)));	// other memcpy
	}
    return retVal;
}

bool DecodedFile::writeBuffer (int * L_buffer, int * R_buffer, unsigned num_samples) {
	sample * outL, * outR;
	int * left_ch, * right_ch;
	bool retVal = true;
	
	if (!this->checkBufferStack(num_samples))
	    return false;
	Buffer * bf = mBuffers.back();						// get buffer from vector
	outL = bf->buffer(0) + bf->mNumFrames;				// get prts for copy loop
	left_ch = L_buffer;
	if (mNumChannels == 2) {
		outR = bf->buffer(1) + bf->mNumFrames;
		right_ch = R_buffer;
	}
	unsigned nsamples = num_samples;
	if (mMaxDurInSecs > 0.0) {										// check for overflow of mMaxSize
		unsigned tot = this->totalSize();
		unsigned maxFrames = (unsigned) (mMaxDurInSecs * mFrameRate);
		if ((tot + nsamples) >= maxFrames) {
			retVal = false;
			// We set a check for an absurd situation when badly encoded MP3 files change sample rates half way through the file, changing maxFrames.
			nsamples = (tot > maxFrames) ? 0 : maxFrames - tot;
		}
	}
	bf->mNumFrames += nsamples;							// increment buf ctr
	while (nsamples--) {								// float copy loop
		*outL++ = scale_MP3F(*left_ch++);
		if (mNumChannels == 2)
			*outR++ = scale_MP3F(*right_ch++);
	}
    return retVal;
}

bool DecodedFile::writeBuffer (float * stereo_buffer, unsigned num_samples) {
	sample * outL, * outR, * inP;
	bool retVal = true;

	if (!this->checkBufferStack(num_samples))
	    return false;
	Buffer * bf = mBuffers.back();						// get buffer from vector
	outL = bf->buffer(0) + bf->mNumFrames;				// get prts for copy loop
	if (mNumChannels == 2)
		outR = bf->buffer(1) + bf->mNumFrames;
	unsigned nsamples = num_samples / mNumChannels;
	if (mMaxDurInSecs > 0.0) {										// check for overflow of mMaxSize
		unsigned tot = this->totalSize();
		unsigned maxFrames = (unsigned) (mMaxDurInSecs * mFrameRate);
		if ((tot + nsamples) >= maxFrames) {
			retVal = false;
			nsamples = maxFrames - tot;
		}
	}
	bf->mNumFrames += nsamples;						// increment buf ctr
	inP = stereo_buffer;
	while (nsamples--) {								// float copy loop
		*outL++ = *inP++;
		if (mNumChannels == 2)
			*outR++ = *inP++;
	}
    return retVal;
}

// calc total size

unsigned DecodedFile::totalSize() {
	unsigned totS = 0;
	for (BufferVector::iterator it = mBuffers.begin(); it != mBuffers.end(); it++)
		totS += (*it)->mNumFrames;
	return totS;
}

/// merge (concatenate) the buffers into 1 after loading

void DecodedFile::mergeBuffers() {
	if (mBuffers.empty()) {
//		logMsg(kLogError, "DecodedFile::mergeBuffers - empty buffer vector");
		mIsValid = false;
		return;
	}
	Buffer * bf = mBuffers.front();
	if (bf->mNumFrames <= 0) {
//		logMsg(kLogError, "DecodedFile::mergeBuffers - empty buffer");
		mIsValid = false;
		return;
	}
	if (mBuffers.size() > 1) {					// if > 1 buffer
		unsigned totS = this->totalSize();
		bf = new Buffer(2, totS);				// create large buffer
		bf->allocateBuffers();
		unsigned offset = 0;					// copy loop
		for (BufferVector::iterator it = mBuffers.begin(); it != mBuffers.end(); it++) {
			Buffer * b2 = *it;
			bf->copySamplesFromTo(*b2, offset);	// write sample to concat buffer
			offset += b2->mNumFrames;
		}
	}
	mNumChannels = bf->mNumChannels;
	mNumFrames = bf->mNumFrames;
	mWavetable.freeBuffers();
	mWavetable.setSize(mNumChannels, mNumFrames);
	mWavetable.allocateBuffers();
	mWavetable.copySamplesFrom(*bf);			// copy data over to mWavetable
	mSFInfo = 0;	
	mSndfile = 0;	
	mStart = 0;
	mStop = mNumFrames;
	mBytesPerSample = 4;
	mIsValid = true;
	mBase = 0;
	if (mBuffers.size() > 1)
		delete bf;
//	mBuffers.clear();							// erase old values
	for (BufferVector::iterator it = mBuffers.begin(); it != mBuffers.end(); it++)
		delete *it;
}

void DecodedFile::openForWrite(SoundFileFormat tformat, unsigned nchannels, unsigned rate, unsigned bitDepth) 
			throw (CException) {
	throw IOError("Decoded files cannot be written");
}

void DecodedFile::openForReadWrite() throw (CException) {
	throw IOError("Decoded files cannot be written");
}

// copy next buffer from cache

void DecodedFile::nextBuffer(Buffer &outB) throw (CException) {

}	

//
//////////////////// MP3File implementation ///////////////////////////////////////////////////////
//

// simple buffer vector struct with the MP3 file ptr

struct vbuffer {
	unsigned char const * start;
	unsigned long length;
	BufferVector * outBufs;
	MP3File * mp3File;
};

// temp buffer & file for writing output

#ifdef WRITE_MP3_TO_FILE
static MP3_SAMP_TYPE * xlate_buffer = NULL;
static SNDFILE * sfOutput = NULL;
#endif

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
	bool continueDecoding = true;
						// pcm->samplerate contains the sampling frequency
	nchannels = pcm->channels;
	nsamples  = pcm->length;				// normally 1152
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	
#ifdef WRITE_MP3_TO_FILE
	MP3_SAMP_TYPE * out_ptr = xlate_buffer;
	while (nsamples--) {
		*out_ptr++ = scale_MP3(*left_ch++);
		if (nchannels == 2)
			*out_ptr++ = scale_MP3(*right_ch++);
	}
	MP3_WRITE(sfOutput, xlate_buffer, pcm->length);		// libsndfile sf_writef_short function
	
#else													// else write to buffer
	
	struct vbuffer * vbf = (vbuffer *) data;
	MP3File * mp3File = vbf->mp3File;
	if (mp3File->numChannels() != nchannels)
		mp3File->setNumChannels(nchannels);

	continueDecoding = mp3File->writeBuffer((int *) left_ch, (int *) right_ch, nsamples);
#endif
	
    return continueDecoding ? MAD_FLOW_CONTINUE : MAD_FLOW_STOP;
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

static enum mad_flow m3_header(void *data, struct mad_header const *header) {
//	fprintf(stderr, "SR: %d\n", header->samplerate);
	struct vbuffer * vbf = (vbuffer *) data;
	MP3File * mp3File = vbf->mp3File;
	mp3File->mMP3Rate = header->samplerate;
	mp3File->setFrameRate(header->samplerate);
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
//	fprintf(stderr, "\tMP3File::decodeMP3() \"%s\"n", mPath.c_str());
	inFile = open(mPath.c_str(), O_RDONLY);		// open & test in file
	if (fstat(inFile, &stat) == -1 || stat.st_size == 0)
//		throw IOError("Sound file MP3 open error");
		return -1;
												// memory-map input file -- ToDo: this is UNIX-specific
	void * mapped_data = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, inFile, 0);

#ifdef WRITE_MP3_TO_FILE
	mTempPath = new char[CSL_LINE_LEN];			// create temp file name
	if (mTempDir)
		MP3_TEMP_NAME(mPath.c_str(), mTempPath, mTempDir)
	else
		AIFF_TEMP_NAME(mPath.c_str(), mTempPath)
												// create output sound file
	mTempFile = new LSoundFile(mTempPath, -1, -1, false);
	mTempFile->openForWrite(kSoundFileFormatAIFF, 2, CGestalt::frameRate(), MP3_DEPTH);
#endif

	struct vbuffer vbf;						// now set up libMAD decoder
	struct mad_decoder decoder;
	int result = 0;
	vbf.start  = (unsigned char const *) mapped_data;
	vbf.length = stat.st_size;
	vbf.mp3File = this;
												// create first buffer
	Buffer * bf = new Buffer(2, CGestalt::sndFileFrames());

	bf->allocateBuffers();
	bf->mNumFrames = 0;
	mBuffers.push_back(bf);						// store buffer
	vbf.outBufs = & mBuffers;
#ifdef WRITE_MP3_TO_FILE						// set up the statics
	sfOutput = mTempFile->sndFile();
	xlate_buffer = new MP3_SAMP_TYPE[CGestalt::maxBufferFrames()];
#endif

	mad_decoder_init(&decoder, &vbf,			// configure input, output, and error functions
					 m3_input, 
					 m3_header, /* header */
					 0			/* filter */, 
					 m3_output,	/* output function does the work */
					 m3_error, 
					 0			/* message */);

												// start decoding 
	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	mad_decoder_finish(&decoder);				// release the decoder
#ifdef CSL_DEBUG
	logMsg("MP3 decoder returned %d; temp file %s", result, tempPath);
#endif
	munmap(mapped_data, stat.st_size);
	close_file(inFile);
#ifdef WRITE_MP3_TO_FILE
	mTempFile->close();
	delete mTempFile;
#else											// handle MP3 buffer vector
	this->mergeBuffers();
#endif
	return result;
}

/// MP3File constructors

MP3File::MP3File(string tpath, int start, int stop)
		: DecodedFile(tpath, start, stop, 0) { 
	mMode = kSoundFileClosed;
	mMP3Rate = DEFAULT_MP3_RATE;			// default rate
//	fprintf(stderr, "\tMP3File::c'tor \"%s\" \n", path.c_str());
	openForRead();						// read and cache whole file
}

MP3File::MP3File(float maxDurInSecs, string tpath)
		: DecodedFile(tpath, -1, -1, maxDurInSecs) { 
	mMode = kSoundFileClosed;
	mMP3Rate = DEFAULT_MP3_RATE;			// default rate
//	fprintf(stderr, "\tMP3File::c'tor \"%s\" \n", path.c_str());
	openForRead();						// read and cache whole file
}

MP3File::~MP3File() { }

/// openForRead decodes MP3 file into a temp file and plugs that into the receiver

void MP3File::openForRead() throw (CException) {
//	fprintf(stderr, "\tMP3File::openForRead() \"%s\"n", mTempPath);
//	mTempDir = NULL;
	if (mMode == kSoundFileRead)
		return;
	mMode = kSoundFileRead;

	if(!this->readTags()) {			// read the ID3 tags
//		logMsg(kLogError, "No tags in MP3 sound file");
//		return;
	}
	int ret = this->decodeMP3();				// decode the MP3 using libMAD
	if (ret < 0) {
		mIsValid = false;
		return;
	}
	mFrameRate = mMP3Rate;			// ToDo: this can be a problem if the sample rate changes during the file.
	
//	if (mFrameRate != DEFAULT_MP3_RATE) {
////		if ((mMP3Rate != 48000) && (mMP3Rate != 32000)) {
//		if ((mMP3Rate < 20000) || (mMP3Rate > 50000)) {
//			logMsg(kLogError, "Unsupported MP3 file sample rate: %d", mMP3Rate);
//			throw IOError("Unsupported MP3 file sample rate");
//		}
//#ifdef CSL_USE_SRConv
//		this->convertRate(mTempPath, mMP3Rate, DEFAULT_MP3_RATE);
//#endif
//	}
#ifdef WRITE_MP3_TO_FILE
	mTempFile = new LSoundFile(mTempPath);
	mTempFile->openForRead();
	mSFInfo = mTempFile->sfInfo();	
	mSndfile = mTempFile->sndFile();	
	mMode = mTempFile->mode();
	mStart = mTempFile->startFrame();
	mStop = mTempFile->stopFrame();
	mIsValid = mTempFile->isValid();
	mNumChannels = mTempFile->numChannels();
	mNumFrames = mTempFile->duration();
	mWavetable.copyFrom(mTempFile->mWavetable);
#endif
}

#endif // USE_libMAD

#ifdef USE_libFAAD				// Support AAC/MP4 file reading using libFAAD

//
//////////////////// AACFile implementation /////////////////////////////////////////////////////////////////////
//

static int fill_buffer(aac_buffer *b) {
    int bread;
    if (b->bytes_consumed > 0) {
        if (b->bytes_into_buffer) {
            memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed),
                b->bytes_into_buffer*sizeof(unsigned char));
        }
        if (!b->at_eof) {
            bread = fread((void*)(b->buffer + b->bytes_into_buffer), 1,
                b->bytes_consumed, b->infile);

            if (bread != b->bytes_consumed)
                b->at_eof = 1;

            b->bytes_into_buffer += bread;
        }
        b->bytes_consumed = 0;
        if (b->bytes_into_buffer > 3) {
            if (memcmp(b->buffer, "TAG", 3) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 11) {
            if (memcmp(b->buffer, "LYRICSBEGIN", 11) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 8) {
            if (memcmp(b->buffer, "APETAGEX", 8) == 0)
                b->bytes_into_buffer = 0;
        }
    }
    return 1;
}

static void advance_buffer(aac_buffer *b, int bytes) {
    b->file_offset += bytes;
    b->bytes_consumed = bytes;
    b->bytes_into_buffer -= bytes;
	if (b->bytes_into_buffer < 0)
		b->bytes_into_buffer = 0;
}

static int adts_sample_rates[] = 
{96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

static int adts_parse(aac_buffer *b, int *bitrate, float *length) {
    int frames, frame_length;
    int t_framelength = 0;
    int samplerate;
    float frames_per_sec, bytes_per_frame;

							/* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++) {
        fill_buffer(b);
        if (b->bytes_into_buffer > 7) {
								/* check syncword */
            if (!((b->buffer[0] == 0xFF)&&((b->buffer[1] & 0xF6) == 0xF0)))
                break;
            if (frames == 0)
                samplerate = adts_sample_rates[(b->buffer[2]&0x3c)>>2];
            frame_length = ((((unsigned int)b->buffer[3] & 0x3)) << 11)
						 | (((unsigned int)b->buffer[4]) << 3) | (b->buffer[5] >> 5);
            t_framelength += frame_length;
            if (frame_length > b->bytes_into_buffer)
                break;

            advance_buffer(b, frame_length);
        } else {
            break;
        }
    }
    frames_per_sec = (float)samplerate/1024.0f;
    if (frames != 0)
        bytes_per_frame = (float)t_framelength/(float)(frames*1000);
    else
        bytes_per_frame = 0;
    *bitrate = (int)(8. * bytes_per_frame * frames_per_sec + 0.5);
    if (frames_per_sec != 0)
        *length = (float)frames/frames_per_sec;
    else
        *length = 1;
    return 1;
}

uint32_t read_callback(void *user_data, void *buffer, uint32_t length) {
    return fread(buffer, 1, length, (FILE*)user_data);
}

uint32_t seek_callback(void *user_data, uint64_t position) {
    return fseek((FILE*)user_data, position, SEEK_SET);
}

///* MicroSoft channel definitions */
//#define SPEAKER_FRONT_LEFT             0x1
//#define SPEAKER_FRONT_RIGHT            0x2
//#define SPEAKER_FRONT_CENTER           0x4
//#define SPEAKER_LOW_FREQUENCY          0x8
//#define SPEAKER_BACK_LEFT              0x10
//#define SPEAKER_BACK_RIGHT             0x20
//#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
//#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
//#define SPEAKER_BACK_CENTER            0x100
//#define SPEAKER_SIDE_LEFT              0x200
//#define SPEAKER_SIDE_RIGHT             0x400
//#define SPEAKER_TOP_CENTER             0x800
//#define SPEAKER_TOP_FRONT_LEFT         0x1000
//#define SPEAKER_TOP_FRONT_CENTER       0x2000
//#define SPEAKER_TOP_FRONT_RIGHT        0x4000
//#define SPEAKER_TOP_BACK_LEFT          0x8000
//#define SPEAKER_TOP_BACK_CENTER        0x10000
//#define SPEAKER_TOP_BACK_RIGHT         0x20000
//#define SPEAKER_RESERVED               0x80000000
//
//static long aacChannelConfig2wavexChannelMask(NeAACDecFrameInfo *hInfo) {
//    if (hInfo->channels == 6 && hInfo->num_lfe_channels) {
//        return SPEAKER_FRONT_LEFT + SPEAKER_FRONT_RIGHT +
//            SPEAKER_FRONT_CENTER + SPEAKER_LOW_FREQUENCY +
//            SPEAKER_BACK_LEFT + SPEAKER_BACK_RIGHT;
//    } else {
//        return 0;
//    }
//}
//
//static const char * position2string(int position) {
//    switch (position) {
//    case FRONT_CHANNEL_CENTER: return "Center front";
//    case FRONT_CHANNEL_LEFT:   return "Left front";
//    case FRONT_CHANNEL_RIGHT:  return "Right front";
//    case SIDE_CHANNEL_LEFT:    return "Left side";
//    case SIDE_CHANNEL_RIGHT:   return "Right side";
//    case BACK_CHANNEL_LEFT:    return "Left back";
//    case BACK_CHANNEL_RIGHT:   return "Right back";
//    case BACK_CHANNEL_CENTER:  return "Center back";
//    case LFE_CHANNEL:          return "LFE";
//    case UNKNOWN_CHANNEL:      return "Unknown";
//    default: return "";
//    }
//    return "";
//}
//
//static void print_channel_info(NeAACDecFrameInfo *frameInfo) {
//    /* print some channel info */
//    int i;
//    long channelMask = aacChannelConfig2wavexChannelMask(frameInfo);
//
//    logMsg("  ---------------------");
//    if (frameInfo->num_lfe_channels > 0) {
//        logMsg(" | Config: %2d.%d Ch     |", frameInfo->channels-frameInfo->num_lfe_channels, frameInfo->num_lfe_channels);
//    } else {
//        logMsg(" | Config: %2d Ch       |", frameInfo->channels);
//    }
//    if (channelMask)
//        logMsg(" WARNING: channels are reordered according to");
//    else
//        logMsg("");
//    logMsg("  ---------------------");
//    if (channelMask)
//        logMsg("  MS defaults defined in WAVE_FORMAT_EXTENSIBLE");
//    else
//        logMsg("");
//    logMsg(" | Ch |    Position    |");
//    logMsg("  ---------------------");
//    for (i = 0; i < frameInfo->channels; i++) {
//        logMsg(" | %.2d | %-14s |", i, position2string((int)frameInfo->channel_position[i]));
//    }
//    logMsg("  ---------------------");
//    logMsg("");
//}

//static int FindAdtsSRIndex(int sr) {
//    int i;
//    for (i = 0; i < 16; i++) {
//        if (sr == adts_sample_rates[i])
//            return i;
//    }
//    return 16 - 1;
//}

/* find AAC track */

static int GetAACTrack(mp4ff_t *infile) {
    int i, rc;
    int numTracks = mp4ff_total_tracks(infile);

    for (i = 0; i < numTracks; i++) {
        unsigned char * buff = NULL;
        unsigned int buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;
        mp4ff_get_decoder_config(infile, i, &buff, &buff_size);
        if (buff) {
            rc = NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC);
            free(buff);
            if (rc < 0)
                continue;
            return i;
        }
    }
    return -1;					/* can't decode this */
}

static const unsigned long srates[] = 
{ 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

/* print some mp4 file info */

void MP4File::printMP4(mp4ff_t * infile, int track, unsigned *song_length) {
    logMsg("%s file info:", mPath.c_str());
	char *tag = NULL, *item = NULL;
	int k, j;
	const char *ot[6] = { "NULL", "MAIN AAC", "LC AAC", "SSR AAC", "LTP AAC", "HE AAC" };
	long samples = mp4ff_num_samples(infile, track);
	float f = 1024.0;
	float seconds;
	if (mp4ASC.sbr_present_flag == 1)
		f = f * 2.0;
	seconds = (float)samples * (float)(f-1.0) / (float)mp4ASC.samplingFrequency;
	*song_length = (unsigned) samples * (unsigned) (f - 1);
	logMsg("%s\t%.3f secs, %d ch, %d Hz", ot[(mp4ASC.objectTypeIndex > 5) ? 0 : mp4ASC.objectTypeIndex],
		seconds, mp4ASC.channelsConfiguration, mp4ASC.samplingFrequency);
#define PRINT_MP4_METADATA
#ifdef PRINT_MP4_METADATA
	j = mp4ff_meta_get_num_items(infile);
	for (k = 0; k < j; k++) {
		if (mp4ff_meta_get_by_index(infile, k, &item, &tag)) {
			if (item != NULL && tag != NULL) {
				logMsg("\t\t%s: %s", item, tag);
				free(item); item = NULL;
				free(tag); tag = NULL;
			}
		}
	}
	if (j > 0) logMsg("");
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

MP4File::MP4File(string tpath, int start, int stop)
		: DecodedFile(tpath, start, stop, 0) { 
	mMode = kSoundFileClosed;
	openForRead();						// read and cache whole file
}

MP4File::MP4File(float maxDurInSecs, string tpath)
		: DecodedFile(tpath, -1, -1, maxDurInSecs) { 
	mMode = kSoundFileClosed;
	openForRead();						// read and cache whole file
}

MP4File::~MP4File() {
			// structs are auto-free'd
}

// open file and get stats

void MP4File::openForRead() throw (CException) {
	unsigned char header[8];
	int mp4file = 0;
	Status result;
	if (mMode == kSoundFileRead)
		return;
	mMode = kSoundFileRead;

//	unsigned long cap = NeAACDecGetCapabilities();
//	if (cap & FIXED_POINT_CAP)
//		logMsg("Fixed point version");
//	else
//		logMsg("Floating point version");
	
	/* check for mp4 file */
	FILE * hMP4File = fopen(mPath.c_str(), "rb");
	if (!hMP4File) {
		logMsg("Error opening file (fopen): %s", mPath.c_str());
		return;
	}
	fread(header, 1, 8, hMP4File);
	rewind(hMP4File);
	//	logMsg("Header: %.4s - %.4s", &header[0], &header[4]);
	//	if (header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p')
	if (memcmp(&header[4], "ftyp", 4) == 0)
		mp4file = 1;
	
	if (mp4file)					// now call the right decoder method
		result = this->decodeMP4file(hMP4File);
	else
		result = this->decodeAACfile(hMP4File);
	
	if (result != kOk) {
//		logMsg("Error opening file (decode): %s", mPath.c_str());
//		return;
	}
	fclose(hMP4File);
	this->mergeBuffers();
	
	if (mIsValid) {
		this->readTags();			// read the ID3 tags
	}
}

// decode

Status MP4File::decodeAACfile(FILE * inFile) throw (CException){
	float length;
    int bread, fileread;
    int header_type;
    int bitrate;
	int tagsize;
    unsigned long samplerate;
    unsigned char tchannels;
    void *sample_buffer;
//	unsigned char *adtsData;
//    int adtsDataSize;

    memset(&aacBuff, 0, sizeof(aac_buffer));
    aacBuff.infile = inFile;
    fseek(aacBuff.infile, 0, SEEK_END);
    fileread = ftell(aacBuff.infile);
    fseek(aacBuff.infile, 0, SEEK_SET);

    if ( ! (aacBuff.buffer = (unsigned char *) malloc(FAAD_MIN_STREAMSIZE * MAX_CHANNELS))) {
        logMsg("Memory allocation error");
        return kErr;
    }
    memset(aacBuff.buffer, 0, FAAD_MIN_STREAMSIZE * MAX_CHANNELS);

    bread = fread(aacBuff.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, aacBuff.infile);
    aacBuff.bytes_into_buffer = bread;
    aacBuff.bytes_consumed = 0;
    aacBuff.file_offset = 0;

    if (bread != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
        aacBuff.at_eof = 1;

    tagsize = 0;
    if (!memcmp(aacBuff.buffer, "ID3", 3)) {
									/* high bit is not used */
        tagsize = (aacBuff.buffer[6] << 21) | (aacBuff.buffer[7] << 14) |
				  (aacBuff.buffer[8] <<  7) | (aacBuff.buffer[9] <<  0);

        tagsize += 10;
        advance_buffer(&aacBuff, tagsize);
        fill_buffer(&aacBuff);
    }
    hDecoder = NeAACDecOpen();

									/* Set the default object type and samplerate */
									/* This is useful for RAW AAC files */
    config = NeAACDecGetCurrentConfiguration(hDecoder);
    config->defObjectType = LC;
    config->outputFormat = 4;
    config->downMatrix = 1;
    config->useOldADTSFormat = 0;
//	config->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(hDecoder, config);

									/* get AAC infos for printing */
    header_type = 0;
    if ((aacBuff.buffer[0] == 0xFF) && ((aacBuff.buffer[1] & 0xF6) == 0xF0)) {
        adts_parse(&aacBuff, &bitrate, &length);
        fseek(aacBuff.infile, tagsize, SEEK_SET);

        bread = fread(aacBuff.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, aacBuff.infile);
        if (bread != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
            aacBuff.at_eof = 1;
        else
            aacBuff.at_eof = 0;
        aacBuff.bytes_into_buffer = bread;
        aacBuff.bytes_consumed = 0;
        aacBuff.file_offset = tagsize;
        header_type = 1;
    } else if (memcmp(aacBuff.buffer, "ADIF", 4) == 0) {
        int skip_size = (aacBuff.buffer[4] & 0x80) ? 9 : 0;
        bitrate = ((unsigned int)(aacBuff.buffer[4 + skip_size] & 0x0F)<<19) |
            ((unsigned int)aacBuff.buffer[5 + skip_size]<<11) |
            ((unsigned int)aacBuff.buffer[6 + skip_size]<<3) |
            ((unsigned int)aacBuff.buffer[7 + skip_size] & 0xE0);

        length = (float) fileread;
        if (length != 0) {
            length = ((float)length*8.0f)/((float)bitrate) + 0.5f;
        }
        bitrate = (int)((float)bitrate/1000.0f + 0.5f);
        header_type = 2;
    }

    fill_buffer(&aacBuff);
    if ((bread = NeAACDecInit(hDecoder, aacBuff.buffer,
			aacBuff.bytes_into_buffer, &samplerate, &tchannels)) < 0) {
									/* If some error initializing occured, skip the file */
        logMsg("Error initializing decoder library.");
        if (aacBuff.buffer)
            free(aacBuff.buffer);
        NeAACDecClose(hDecoder);
        fclose(aacBuff.infile);
        return kErr;
    }
    advance_buffer(&aacBuff, bread);
    fill_buffer(&aacBuff);

									/* print AAC file info */
		logMsg("%s file info:", mPath.c_str());
		switch (header_type) {
		case 0:
			logMsg("RAW");
			break;
		case 1:
			logMsg("ADTS, %.3f sec, %d kbps, %d Hz", length, bitrate, samplerate);
			break;
		case 2:
			logMsg("ADIF, %.3f sec, %d kbps, %d Hz", length, bitrate, samplerate);
			break;
		}
	
    do {										// main read loop
        sample_buffer = NeAACDecDecode(hDecoder, &frameInfo, aacBuff.buffer, aacBuff.bytes_into_buffer);

												/* update buffer indices */
        advance_buffer(&aacBuff, frameInfo.bytesconsumed);

        if (frameInfo.error > 0) {
            logMsg("Error: %s",
                NeAACDecGetErrorMessage(frameInfo.error));
        }
		if ( ! writeBuffer((float *) sample_buffer, frameInfo.samples) )
			break;		
												/* fill buffer */
        fill_buffer(&aacBuff);
        if (aacBuff.bytes_into_buffer == 0)
            sample_buffer = NULL;				/* to make sure it stops now */

    } while (sample_buffer != NULL);			// end of loop

    NeAACDecClose(hDecoder);
	if (aacBuff.buffer)
        free(aacBuff.buffer);
    return (frameInfo.error ? kErr : kOk);
}

// read and decode an MP4 file

Status MP4File::decodeMP4file(FILE * inFile) throw (CException) {
	int track;
	void *sample_buffer;
	long sampleId, numSamples;
	mp4ff_t *mp4File;
//	FILE *adtsFile;
//	unsigned char *adtsData;
//	int adtsDataSize;
	unsigned char *tbuffer = NULL;
	unsigned int buffer_size;
	unsigned long samplerate;
	unsigned char tchannels;
							/* for gapless decoding */
	unsigned int useAacLength = 1;
	unsigned int initial = 1;
	unsigned int framesize;
	unsigned long timescale;
	
							/* initialise the callback structure */
	mp4ff_callback_t * mp4cb = (mp4ff_callback_t *) malloc(sizeof(mp4ff_callback_t));
	mp4cb->read = read_callback;
	mp4cb->seek = seek_callback;
	mp4cb->user_data = inFile;
	
	hDecoder = NeAACDecOpen();
	
							/* Set configuration */
	config = NeAACDecGetCurrentConfiguration(hDecoder);
	config->outputFormat = 4;
	config->downMatrix = 1;
							//	config->dontUpSampleImplicitSBR = 1;
	NeAACDecSetConfiguration(hDecoder, config);
	
	mp4File = mp4ff_open_read(mp4cb);
	
	if ((track = GetAACTrack(mp4File)) < 0) {
		logMsg("Warning: Unable to find correct AAC sound track in the MP4 file.");
		NeAACDecClose(hDecoder);
		mp4ff_close(mp4File);
		free(mp4cb);
		fclose(inFile);
		return kErr;
	}
	tbuffer = NULL;
	buffer_size = 0;
	mp4ff_get_decoder_config(mp4File, track, &tbuffer, &buffer_size);
	
	if (NeAACDecInit2(hDecoder, tbuffer, buffer_size, &samplerate, &tchannels) < 0) {
							/* If some error initializing occured, skip the file */
		logMsg("Error initializing decoder library.");
		NeAACDecClose(hDecoder);
		mp4ff_close(mp4File);
		free(mp4cb);
		fclose(inFile);
		return kErr;
	}
	timescale = mp4ff_time_scale(mp4File, track);
	framesize = 1024;
	useAacLength = 0;
	mFrameRate = samplerate;
	mNumChannels = tchannels;
	if (tbuffer) {
		if (NeAACDecAudioSpecificConfig(tbuffer, buffer_size, &mp4ASC) >= 0) {
			if (mp4ASC.frameLengthFlag == 1)
				framesize = 960;
			if (mp4ASC.sbr_present_flag == 1)
				framesize *= 2;
		}
		free(tbuffer);
	}
	numSamples = mp4ff_num_samples(mp4File, track);
	
	unsigned songLength;
//	if (gConfig->Verbose()) 
//		this->printMP4(mp4File, track, &songLength);

	this->addBuffer(mNumChannels, (songLength + mFrameRate));
											// IO loop
	for (sampleId = 0; sampleId < numSamples; sampleId++) {
		int rc;
		long dur;
		unsigned int sample_count;
		unsigned int delay = 0;
		tbuffer = NULL;
		buffer_size = 0;		
							/* get acces unit from MP4 file */
		dur = mp4ff_get_sample_duration(mp4File, track, sampleId);
		rc = mp4ff_read_sample(mp4File, track, sampleId, &tbuffer,  &buffer_size);
		if (rc == 0) {
			logMsg("Reading from MP4 file failed.");
			NeAACDecClose(hDecoder);
			mp4ff_close(mp4File);
			free(mp4cb);
			fclose(inFile);
			return kErr;
		}													// decode a block here, returns the sample buffer
		sample_buffer = NeAACDecDecode(hDecoder, &frameInfo, tbuffer, buffer_size);
		if (tbuffer) free(tbuffer);
		
		if (sampleId == 0) dur = 0;
		
		if (useAacLength || (timescale != samplerate)) {
			sample_count = frameInfo.samples;
		} else {
			sample_count = (unsigned int)(dur * frameInfo.channels);
			if (sample_count > frameInfo.samples)
				sample_count = frameInfo.samples;
			
			if (!useAacLength && !initial && (sampleId < numSamples/2) && (sample_count != frameInfo.samples)) {
				logMsg("MP4 seems to have incorrect frame duration, using values from AAC data.");
				useAacLength = 1;
				sample_count = frameInfo.samples;
			}
		}
		if (initial && (sample_count < framesize*frameInfo.channels) && (frameInfo.samples > sample_count))
			delay = frameInfo.samples - sample_count;
//		if ((sampleId == 0) && gConfig->Verbose() && !frameInfo.error) {	// print some channel info
//			print_channel_info(&frameInfo);
//		}
		if ((frameInfo.error == 0) && (sample_count > 0))
			if ( ! writeBuffer((float *) sample_buffer, sample_count))
				break;
		
		if (frameInfo.error > 0) {
			logMsg("Warning: %s", NeAACDecGetErrorMessage(frameInfo.error));
		}
	}
	NeAACDecClose(hDecoder);
	mp4ff_close(mp4File);
	free(mp4cb);
    return (frameInfo.error ? kErr : kOk);
}

#endif
