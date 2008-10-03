// MediaParserFfmpeg.cpp: FFMPG media parsers, for Gnash
//
//   Copyright (C) 2007, 2008 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//


#include "MediaParserFfmpeg.h"
#include "GnashException.h"
#include "log.h"
#include "IOChannel.h" // for use

using namespace std;

//#define PADDING_BYTES 64
//#define READ_CHUNKS 64

namespace gnash {
namespace media {

namespace { // anonymous namespace

	// Used to calculate a decimal value from a ffmpeg fraction
	inline double as_double(AVRational time)
	{
		return time.num / (double) time.den;
	}

} // anonymous namespace


int
MediaParserFfmpeg::readPacketWrapper(void* opaque, boost::uint8_t* buf, int buf_size)
{
	MediaParserFfmpeg* p = static_cast<MediaParserFfmpeg*>(opaque);
	return p->readPacket(buf, buf_size);
}

offset_t
MediaParserFfmpeg::seekMediaWrapper(void *opaque, offset_t offset, int whence)
{
	MediaParserFfmpeg* p = static_cast<MediaParserFfmpeg*>(opaque);
	return p->seekMedia(offset, whence);
}

AVInputFormat*
MediaParserFfmpeg::probeStream()
{
	boost::scoped_array<boost::uint8_t> buffer(new boost::uint8_t[2048]);

	// Probe the file to detect the format
	AVProbeData probe_data;
	probe_data.filename = "";
	probe_data.buf = buffer.get();
	probe_data.buf_size = 2048;

	assert(_stream->tell() == 0);
	size_t actuallyRead = _stream->read(probe_data.buf, probe_data.buf_size);
	_stream->seek(0);

	if (actuallyRead < 1)
	{
 		throw IOException(_("MediaParserFfmpeg could not read probe data from input"));
 		return NULL;
	}

	probe_data.buf_size = actuallyRead; // right ?
	AVInputFormat* ret = av_probe_input_format(&probe_data, 1);
	return ret;
}

bool
MediaParserFfmpeg::seek(boost::uint32_t& pos)
{
	LOG_ONCE(log_unimpl("MediaParserFfmpeg::seek()"));
	return false;

	log_debug("MediaParserFfmpeg::seek(%d) TESTING", pos);

	AVStream* videostream = _formatCtx->streams[_videoStreamIndex];
    	double timebase = static_cast<double>(videostream->time_base.num / videostream->time_base.den);
	long newpos = static_cast<long>(pos / timebase);
		
	if (av_seek_frame(_formatCtx, _videoStreamIndex, newpos, 0) < 0)
	{
		log_error(_("%s: seeking failed"), __FUNCTION__);
		return 0;
	}

	AVPacket Packet;
	av_init_packet(&Packet);
	double newtime = 0;
	while (newtime == 0)
	{
		if (av_read_frame(_formatCtx, &Packet) < 0) 
		{
			log_error("Error in av_read_frame (while seeking)");
			av_seek_frame(_formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
			//av_free_packet( &Packet );
			return 0; // ??
		}

		newtime = timebase * (double)_formatCtx->streams[_videoStreamIndex]->cur_dts;
	}

	//av_free_packet( &Packet );
	av_seek_frame(_formatCtx, _videoStreamIndex, newpos, 0);

	newtime = static_cast<boost::int32_t>(newtime / 1000.0);
	return newtime;
}

bool
MediaParserFfmpeg::parseVideoFrame(AVPacket& packet)
{
	assert(packet.stream_index == _videoStreamIndex);
	assert(_videoStream);

	// packet.dts is "decompression" timestamp
	// packet.pts is "presentation" timestamp
	// Dunno why we use dts, and don't understand the magic formula either...
	//
	// From ffmpeg dox:
	//    pkt->pts can be AV_NOPTS_VALUE if the video format has B frames,
	//    so it is better to rely on pkt->dts if you do not decompress the payload.
	//
	boost::uint64_t timestamp = static_cast<boost::uint64_t>(packet.dts * as_double(_videoStream->time_base) * 1000.0); 

#if 0
	LOG_ONCE( log_unimpl("%s", __PRETTY_FUNCTION__) );
	return false;
#else

	// flags, for keyframe
	//bool isKeyFrame = packet.flags&PKT_FLAG_KEY;

	// TODO: FIXME: *2 is an hack to avoid libavcodec reading past end of allocated space
	//       we might do proper padding or (better) avoid the copy as a whole by making
	//       EncodedVideoFrame virtual.
	size_t allocSize = packet.size*2;
	boost::uint8_t* data = new boost::uint8_t[allocSize];
	std::copy(packet.data, packet.data+packet.size, data);
	std::auto_ptr<EncodedVideoFrame> frame(new EncodedVideoFrame(data, packet.size, 0, timestamp));

	pushEncodedVideoFrame(frame);

	return true;
#endif
}

bool
MediaParserFfmpeg::parseAudioFrame(AVPacket& packet)
{
	assert(packet.stream_index == _audioStreamIndex);
	assert(_audioStream);

	// packet.dts is "decompression" timestamp
	// packet.pts is "presentation" timestamp
	// Dunno why we use dts, and don't understand the magic formula either...
	//
	// From ffmpeg dox:
	//    pkt->pts can be AV_NOPTS_VALUE if the video format has B frames,
	//    so it is better to rely on pkt->dts if you do not decompress the payload.
	//
	boost::uint64_t timestamp = static_cast<boost::uint64_t>(packet.dts * as_double(_audioStream->time_base) * 1000.0); 

#if 0
	LOG_ONCE( log_unimpl("%s", __PRETTY_FUNCTION__) );
	return false;
#else
	std::auto_ptr<EncodedAudioFrame> frame ( new EncodedAudioFrame );

	// TODO: FIXME: *2 is an hack to avoid libavcodec reading past end of allocated space
	//       we might do proper padding or (better) avoid the copy as a whole by making
	//       EncodedVideoFrame virtual.
	size_t allocSize = packet.size*2;
	boost::uint8_t* data = new boost::uint8_t[allocSize];
	std::copy(packet.data, packet.data+packet.size, data);

	frame->data.reset(data); 
	frame->dataSize = packet.size;
	frame->timestamp = timestamp;

	pushEncodedAudioFrame(frame); 

	return true;
#endif
}

bool
MediaParserFfmpeg::parseNextFrame()
{
	// lock the stream while reading from it, so actionscript
	// won't mess with the parser on seek  or on getBytesLoaded
	boost::mutex::scoped_lock streamLock(_streamMutex);

	if ( _parsingComplete )
	{
		//log_debug("MediaParserFfmpeg::parseNextFrame: parsing complete, nothing to do");
		return false;
	}

	// position the stream where we left parsing as
	// it could be somewhere else for reading a specific
	// or seeking.
	//_stream->seek(_lastParsedPosition);

	assert(_formatCtx);

  	AVPacket packet;

	//log_debug("av_read_frame call");
  	int rc = av_read_frame(_formatCtx, &packet);
	//log_debug("av_read_frame returned %d", rc);
	if ( rc < 0 )
	{
		log_error(_("MediaParserFfmpeg::parseNextChunk: Problems parsing next frame"));
		// Should we set parsingComplete here ?
		_parsingComplete=true;
		return false;
	}

	bool ret=false;

	if ( packet.stream_index == _videoStreamIndex )
	{
		ret = parseVideoFrame(packet);
	}
	else if ( packet.stream_index == _audioStreamIndex )
	{
		ret = parseAudioFrame(packet);
	}
	else
	{
		ret = false; // redundant..
		log_debug("MediaParserFfmpeg::parseNextFrame: unknown stream index %d", packet.stream_index);
	}

	av_free_packet(&packet);

	// Check if EOF was reached
	if ( _stream->eof() )
	{
		log_debug("MediaParserFfmpeg::parseNextFrame: at eof after av_read_frame");
		_parsingComplete=true;
	}

	// Update _lastParsedPosition
	boost::uint64_t curPos = _stream->tell();
	if ( curPos > _lastParsedPosition )
	{
		_lastParsedPosition = curPos;
	}

	return ret;
}

bool
MediaParserFfmpeg::parseNextChunk()
{
	if ( ! parseNextFrame() ) return false;
	return true;
}

boost::uint64_t
MediaParserFfmpeg::getBytesLoaded() const
{
	//log_unimpl("%s", __PRETTY_FUNCTION__);
	return _lastParsedPosition;
}

MediaParserFfmpeg::MediaParserFfmpeg(std::auto_ptr<IOChannel> stream)
	:
	MediaParser(stream),
	_nextVideoFrame(0),
	_nextAudioFrame(0),
	_inputFmt(0),
	_formatCtx(0),
	_videoStreamIndex(-1),
	_videoStream(0),
	_audioStreamIndex(-1),
	_audioStream(0),
	_lastParsedPosition(0)
{
	initializeParser();

	startParserThread();
}

/*private*/
void
MediaParserFfmpeg::initializeParser()
{
	av_register_all(); // TODO: needs to be invoked only once ?

	_byteIOCxt.buffer = NULL;

	_inputFmt = probeStream();
	if ( ! _inputFmt )
	{
		throw GnashException("MediaParserFfmpeg couldn't figure out input format");
	}

	_formatCtx = av_alloc_format_context();
	assert(_formatCtx);

	// Setup the filereader/seeker mechanism. 7th argument (NULL) is the writer function,
	// which isn't needed.
	_byteIOBuffer.reset( new unsigned char[byteIOBufferSize] );
	init_put_byte(&_byteIOCxt,
		_byteIOBuffer.get(), // buffer
		byteIOBufferSize, // buffer size
		0, // write flags
		this, // opaque pointer to pass to the callbacks
		MediaParserFfmpeg::readPacketWrapper, // packet reader callback
		NULL, // packet writer callback
		MediaParserFfmpeg::seekMediaWrapper // seeker callback
		);

	_byteIOCxt.is_streamed = 1;

	// Open the stream. the 4th argument is the filename, which we ignore.
	if(av_open_input_stream(&_formatCtx, &_byteIOCxt, "", _inputFmt, NULL) < 0)
	{
		throw IOException("MediaParserFfmpeg couldn't open input stream");
	}

	log_debug("Parsing FFMPEG media file: %d streams", _formatCtx->nb_streams);
	if ( _formatCtx->title[0] )     log_debug(_("  Title:'%s'"), _formatCtx->title);
	if ( _formatCtx->author[0] )    log_debug(_("  Author:'%s'"), _formatCtx->author);
	if ( _formatCtx->copyright[0] ) log_debug(_("  Copyright:'%s'"), _formatCtx->copyright);
	if ( _formatCtx->comment[0] )   log_debug(_("  Comment:'%s'"), _formatCtx->comment);
	if ( _formatCtx->album[0] )     log_debug(_("  Album:'%s'"), _formatCtx->album);

	// Find first audio and video stream
	for (unsigned int i = 0; i < (unsigned)_formatCtx->nb_streams; i++)
	{
		AVCodecContext* enc = _formatCtx->streams[i]->codec; 

		switch (enc->codec_type)
		{
			case CODEC_TYPE_AUDIO:
				if (_audioStreamIndex < 0)
				{
					_audioStreamIndex = i;
					_audioStream = _formatCtx->streams[i];
					log_debug(_("  Using stream %d for audio: codec id %d"),
						i, _audioStream->codec->codec_id);
					// codec_name will only be filled by avcodec_find_decoder (later);
				}
				break;

			case CODEC_TYPE_VIDEO:
				if (_videoStreamIndex < 0)
				{
					_videoStreamIndex = i;
					_videoStream = _formatCtx->streams[i];
					log_debug(_("  Using stream %d for video: codec id %d"),
						i, _videoStream->codec->codec_id);
					// codec_name will only be filled by avcodec_find_decoder (later);
				}
				break;
			default:
				break;
		}
	}

	// Create VideoInfo
	if ( _videoStream)
	{
		int codec = static_cast<int>(_videoStream->codec->codec_id); // originally an enum CodecID 
		boost::uint16_t width = _videoStream->codec->width;
		boost::uint16_t height = _videoStream->codec->height;
		boost::uint16_t frameRate = static_cast<boost::uint16_t>(as_double(_videoStream->r_frame_rate));
#if !defined(HAVE_LIBAVFORMAT_AVFORMAT_H) && !defined(HAVE_FFMPEG_AVCODEC_H)
		boost::uint64_t duration = _videoStream->codec_info_duration;
#else
		boost::uint64_t duration = _videoStream->duration;
#endif
		_videoInfo.reset( new VideoInfo(codec, width, height, frameRate, duration, FFMPEG /*codec type*/) );
		
		_videoInfo->extra.reset(new ExtraVideoInfoFfmpeg(
			// NOTE: AVCodecContext.extradata : void* for 51.11.0, uint8_t* for 51.38.0
			(uint8_t*)_videoStream->codec->extradata,
			_videoStream->codec->extradata_size));

		//log_debug("EXTRA: %d bytes of video extra data", _videoStream->codec->extradata_size);
	}

	// Create AudioInfo
	if ( _audioStream)
	{
		int codec = static_cast<int>(_audioStream->codec->codec_id); // originally an enum CodecID 
		boost::uint16_t sampleRate = _audioStream->codec->sample_rate;
		boost::uint16_t sampleSize = SampleFormatToSampleSize(_audioStream->codec->sample_fmt);
		bool stereo = (_audioStream->codec->channels == 2);
#if !defined(HAVE_LIBAVFORMAT_AVFORMAT_H) && !defined(HAVE_FFMPEG_AVCODEC_H)
		boost::uint64_t duration = _audioStream->codec_info_duration;
#else
		boost::uint64_t duration = _audioStream->duration;
#endif
		_audioInfo.reset( new AudioInfo(codec, sampleRate, sampleSize, stereo, duration, FFMPEG /*codec type*/) );

		_audioInfo->extra.reset(new ExtraAudioInfoFfmpeg(
			// NOTE: AVCodecContext.extradata : void* for 51.11.0, uint8_t* for 51.38.0
			(uint8_t*)_audioStream->codec->extradata,
			_audioStream->codec->extradata_size));

		//log_debug("EXTRA: %d bytes of audio extra data", _videoStream->codec->extradata_size);
	}


}


MediaParserFfmpeg::~MediaParserFfmpeg()
{

	if ( _formatCtx )
	{
		// TODO: check if this is correct (should we create RIIA classes for ffmpeg stuff?)
		//av_close_input_file(_formatCtx); // NOTE: this one triggers a mismatched free/delete on _byteIOBuffer !
		av_free(_formatCtx);
	}

	if ( _inputFmt )
	{
		// TODO: check if this is correct (should we create RIIA classes for ffmpeg stuff?)
		//av_free(_inputFmt); // it seems this one blows up, could be due to av_free(_formatCtx) above
	}

}

int 
MediaParserFfmpeg::readPacket(boost::uint8_t* buf, int buf_size)
{
	//GNASH_REPORT_FUNCTION;
	//log_debug("readPacket(%d)", buf_size);

	size_t ret = _stream->read(static_cast<void*>(buf), buf_size);

	return ret;

}

offset_t 
MediaParserFfmpeg::seekMedia(offset_t offset, int whence)
{
	GNASH_REPORT_FUNCTION;

	assert(_stream.get());

	// Offset is absolute new position in the file
	if (whence == SEEK_SET)
	{	
		_stream->seek(offset);
		// New position is offset + old position
	}
	else if (whence == SEEK_CUR)
	{
		_stream->seek(_stream->tell() + offset);
		// New position is offset + end of file
	}
	else if (whence == SEEK_END)
	{
		// This is (most likely) a streamed file, so we can't seek to the end!
		// Instead we seek to byteIOBufferSize bytes... seems to work fine...
		_stream->seek(byteIOBufferSize);

	}

	return _stream->tell(); 
}

boost::uint16_t
MediaParserFfmpeg::SampleFormatToSampleSize(SampleFormat fmt)
{
	switch (fmt)
	{
		case SAMPLE_FMT_U8: // unsigned 8 bits
			return 1;

		case SAMPLE_FMT_S16: // signed 16 bits
		case SAMPLE_FMT_FLT: // float
			return 2;

#if !defined (LIBAVCODEC_VERSION_MAJOR) || LIBAVCODEC_VERSION_MAJOR < 52
// Was dropped for version 52.0.0
		case SAMPLE_FMT_S24: // signed 24 bits
			return 3;
#endif

		case SAMPLE_FMT_S32: // signed 32 bits
			return 4;

		case SAMPLE_FMT_NONE:
		default:
			return 8; // arbitrary value
	}
}


} // end of gnash::media namespace
} // end of gnash namespace

#undef PADDING_BYTES
#undef READ_CHUNKS 
