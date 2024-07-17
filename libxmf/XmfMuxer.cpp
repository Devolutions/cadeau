#include "XmfMuxer.h"

#include <iostream>
#include <string>
#include <mkvmuxer/mkvmuxer.h>
#include <mkvmuxer/mkvmuxerutil.h>
#include <mkvmuxer/mkvwriter.h>
#include <mkvparser/mkvparser.h>
#include <mkvparser/mkvreader.h>
#include <hdr_util.hpp>
#include <common/file_util.h>
#include <errno.h>

// Muxer structure holding necessary components
struct xmf_webm_muxer {};

bool CopyVideoProjection(const mkvparser::Projection& parser_projection,
	mkvmuxer::Projection* muxer_projection);

XmfWebMMuxer* XmfWebMMuxer_New() {
	auto muxer = new XmfWebMMuxer();
	return muxer;
}

int XmfWebMMuxer_Remux(XmfWebMMuxer* muxer, const char* inputPath, const char* outputPath) {

	int64_t projection_type = mkvparser::Projection::kTypeNotPresent;
	float projection_pose_roll = mkvparser::Projection::kValueNotPresent;
	float projection_pose_pitch = mkvparser::Projection::kValueNotPresent;
	float projection_pose_yaw = mkvparser::Projection::kValueNotPresent;
	const char* projection_file = 0;

	uint64_t display_width = 0;
	uint64_t display_height = 0;
	uint64_t pixel_width = 0;
	uint64_t pixel_height = 0;
	uint64_t stereo_mode = 0;

	int vp9_profile = -1;  // No profile set.
	int vp9_level = -1;  // No level set.

	mkvparser::MkvReader reader;

	// Open the file
	if (reader.Open(inputPath)) {
		printf("\n Filename is invalid or error while opening.\n");
		return EXIT_FAILURE;
	}

	long long pos = 0;
	mkvparser::EBMLHeader ebml_header;
	// parse the header, increment the position
	long long ret = ebml_header.Parse(&reader, pos);
	if (ret) {
		printf("\n EBMLHeader::Parse() failed.");
		return EXIT_FAILURE;
	}

	mkvparser::Segment* parser_segment_;
	ret = mkvparser::Segment::CreateInstance(&reader, pos, parser_segment_);
	if (ret) {
		printf("\n Segment::CreateInstance() failed.");
		return EXIT_FAILURE;
	}

	const std::unique_ptr<mkvparser::Segment> parser_segment(parser_segment_);
	ret = parser_segment->Load();
	if (ret < 0 && ret != -3) {
		printf("\n Segment::Load() failed.");
		return EXIT_FAILURE;
	}
	else if (ret == -3) {
		printf("\n Segment::Load() is reading incomplte input file");
	}

	const mkvparser::SegmentInfo* const segment_info = parser_segment->GetInfo();
	if (segment_info == NULL) {
		printf("\n Segment::GetInfo() failed.");
		return EXIT_FAILURE;
	}
	const long long timeCodeScale = segment_info->GetTimeCodeScale();

	mkvmuxer::MkvWriter writer;
	const std::string temp_file = outputPath;
	if (!writer.Open(temp_file.c_str())) {
		printf("\n Filename is invalid or error while opening.\n");
		return EXIT_FAILURE;
	}

	// Set Segment element attributes
	mkvmuxer::Segment muxer_segment;

	if (!muxer_segment.Init(&writer)) {
		printf("\n Could not initialize muxer segment!\n");
		return EXIT_FAILURE;
	}

	muxer_segment.AccurateClusterDuration(false);
	muxer_segment.UseFixedSizeClusterTimecode(false);
	muxer_segment.set_mode(mkvmuxer::Segment::kFile);
	muxer_segment.OutputCues(true);

	// Set SegmentInfo element attributes
	mkvmuxer::SegmentInfo* const info = muxer_segment.GetSegmentInfo();
	info->set_timecode_scale(timeCodeScale);
	info->set_writing_app("Devolutions Cadeau");

	// Copy Tags
	const mkvparser::Tags* const tags = parser_segment->GetTags();
	if (tags) {
		for (int i = 0; i < tags->GetTagCount(); i++) {
			const mkvparser::Tags::Tag* const tag = tags->GetTag(i);
			mkvmuxer::Tag* muxer_tag = muxer_segment.AddTag();

			for (int j = 0; j < tag->GetSimpleTagCount(); j++) {
				const mkvparser::Tags::SimpleTag* const simple_tag =
					tag->GetSimpleTag(j);
				muxer_tag->add_simple_tag(simple_tag->GetTagName(),
					simple_tag->GetTagString());
			}
		}
	}

	// Set Tracks element attributes
	const mkvparser::Tracks* const parser_tracks = parser_segment->GetTracks();
	unsigned long i = 0;
	uint64_t vid_track = 0;  // no track added
	uint64_t aud_track = 0;  // no track added

	using mkvparser::Track;


	// Set Tracks
	while (i != parser_tracks->GetTracksCount()) {
		unsigned long track_num = i++;

		const Track* const parser_track = parser_tracks->GetTrackByIndex(track_num);

		if (parser_track == NULL)
			continue;

		const char* const track_name = parser_track->GetNameAsUTF8();

		const long long track_type = parser_track->GetType();

		if (track_type == Track::kVideo) {
			// Get the video track from the parser
			const mkvparser::VideoTrack* const pVideoTrack =
				static_cast<const mkvparser::VideoTrack*>(parser_track);
			const long long width = pVideoTrack->GetWidth();
			const long long height = pVideoTrack->GetHeight();

			// Add the video track to the muxer
			vid_track = muxer_segment.AddVideoTrack(static_cast<int>(width),
				static_cast<int>(height),
				0); // video_track_number = 0
			if (!vid_track) {
				printf("\n Could not add video track.\n");
				return EXIT_FAILURE;
			}

			mkvmuxer::VideoTrack* const video = static_cast<mkvmuxer::VideoTrack*>(
				muxer_segment.GetTrackByNumber(vid_track));
			if (!video) {
				printf("\n Could not get video track.\n");
				return EXIT_FAILURE;
			}

			if (pVideoTrack->GetColour()) {
				mkvmuxer::Colour muxer_colour;
				if (!libwebm::CopyColour(*pVideoTrack->GetColour(), &muxer_colour))
					return EXIT_FAILURE;
				if (!video->SetColour(muxer_colour))
					return EXIT_FAILURE;
			}

			if (pVideoTrack->GetProjection() ||
				projection_type != mkvparser::Projection::kTypeNotPresent) {
				mkvmuxer::Projection muxer_projection;
				const mkvparser::Projection* const parser_projection =
					pVideoTrack->GetProjection();
				typedef mkvmuxer::Projection::ProjectionType MuxerProjType;
				if (parser_projection &&
					!CopyVideoProjection(*parser_projection, &muxer_projection)) {
					printf("\n Unable to copy video projection.\n");
					return EXIT_FAILURE;
				}
				// Override the values that came from parser if set on command line.
				if (projection_type != mkvparser::Projection::kTypeNotPresent) {
					muxer_projection.set_type(
						static_cast<MuxerProjType>(projection_type));
					if (projection_type == mkvparser::Projection::kRectangular &&
						projection_file != NULL) {
						printf("\n Rectangular projection must not have private data.\n");
						return EXIT_FAILURE;
					}
					else if ((projection_type == mkvparser::Projection::kCubeMap ||
						projection_type == mkvparser::Projection::kMesh) &&
						projection_file == NULL) {
						printf("\n Mesh or CubeMap projection must have private data.\n");
						return EXIT_FAILURE;
					}
					if (projection_file != NULL) {
						std::string contents;
						if (!libwebm::GetFileContents(projection_file, &contents) ||
							contents.size() == 0) {
							printf("\n Failed to read file \"%s\" or file is empty\n",
								projection_file);
							return EXIT_FAILURE;
						}
						if (!muxer_projection.SetProjectionPrivate(
							reinterpret_cast<uint8_t*>(&contents[0]),
							contents.size())) {
							printf("\n Failed to SetProjectionPrivate of length %zu.\n",
								contents.size());
							return EXIT_FAILURE;
						}
					}
				}
				const float kValueNotPresent = mkvparser::Projection::kValueNotPresent;
				if (projection_pose_yaw != kValueNotPresent)
					muxer_projection.set_pose_yaw(projection_pose_yaw);
				if (projection_pose_pitch != kValueNotPresent)
					muxer_projection.set_pose_pitch(projection_pose_pitch);
				if (projection_pose_roll != kValueNotPresent)
					muxer_projection.set_pose_roll(projection_pose_roll);

				if (!video->SetProjection(muxer_projection))
					return EXIT_FAILURE;
			}

			if (track_name)
				video->set_name(track_name);

			video->set_codec_id(pVideoTrack->GetCodecId());

			if (display_width > 0)
				video->set_display_width(display_width);
			if (display_height > 0)
				video->set_display_height(display_height);
			if (pixel_width > 0)
				video->set_pixel_width(pixel_width);
			if (pixel_height > 0)
				video->set_pixel_height(pixel_height);
			if (stereo_mode > 0)
				video->SetStereoMode(stereo_mode);

			const double rate = pVideoTrack->GetFrameRate();
			if (rate > 0.0) {
				video->set_frame_rate(rate);
			}

			size_t parser_private_size;
			const unsigned char* const parser_private_data =
				pVideoTrack->GetCodecPrivate(parser_private_size);

			if (!strcmp(video->codec_id(), mkvmuxer::Tracks::kAv1CodecId)) {
				if (parser_private_data == NULL || parser_private_size == 0) {
					printf("AV1 input track has no CodecPrivate. %s is invalid.", inputPath);
					return EXIT_FAILURE;
				}
			}

			if (!strcmp(video->codec_id(), mkvmuxer::Tracks::kVp9CodecId) &&
				(vp9_profile >= 0 || vp9_level >= 0)) {
				const int kMaxVp9PrivateSize = 6;
				unsigned char vp9_private_data[kMaxVp9PrivateSize];
				int vp9_private_size = 0;
				if (vp9_profile >= 0) {
					if (vp9_profile < 0 || vp9_profile > 3) {
						printf("\n VP9 profile(%d) is not valid.\n", vp9_profile);
						return EXIT_FAILURE;
					}
					const uint8_t kVp9ProfileId = 1;
					const uint8_t kVp9ProfileIdLength = 1;
					vp9_private_data[vp9_private_size++] = kVp9ProfileId;
					vp9_private_data[vp9_private_size++] = kVp9ProfileIdLength;
					vp9_private_data[vp9_private_size++] = vp9_profile;
				}

				if (vp9_level >= 0) {
					const int kNumLevels = 14;
					const int levels[kNumLevels] = { 10, 11, 20, 21, 30, 31, 40,
													41, 50, 51, 52, 60, 61, 62 };
					bool level_is_valid = false;
					for (int i = 0; i < kNumLevels; ++i) {
						if (vp9_level == levels[i]) {
							level_is_valid = true;
							break;
						}
					}
					if (!level_is_valid) {
						printf("\n VP9 level(%d) is not valid.\n", vp9_level);
						return EXIT_FAILURE;
					}
					const uint8_t kVp9LevelId = 2;
					const uint8_t kVp9LevelIdLength = 1;
					vp9_private_data[vp9_private_size++] = kVp9LevelId;
					vp9_private_data[vp9_private_size++] = kVp9LevelIdLength;
					vp9_private_data[vp9_private_size++] = vp9_level;
				}
				if (!video->SetCodecPrivate(vp9_private_data, vp9_private_size)) {
					printf("\n Could not add video private data.\n");
					return EXIT_FAILURE;
				}
			}
			else if (parser_private_data && parser_private_size > 0) {
				if (!video->SetCodecPrivate(parser_private_data, parser_private_size)) {
					printf("\n Could not add video private data.\n");
					return EXIT_FAILURE;
				}
			}
		}
		else if (track_type == Track::kAudio) {
			// Get the audio track from the parser
			const mkvparser::AudioTrack* const pAudioTrack =
				static_cast<const mkvparser::AudioTrack*>(parser_track);
			const long long channels = pAudioTrack->GetChannels();
			const double sample_rate = pAudioTrack->GetSamplingRate();

			// Add the audio track to the muxer
			aud_track = muxer_segment.AddAudioTrack(static_cast<int>(sample_rate),
				static_cast<int>(channels),
				0);
			if (!aud_track) {
				printf("\n Could not add audio track.\n");
				return EXIT_FAILURE;
			}

			mkvmuxer::AudioTrack* const audio = static_cast<mkvmuxer::AudioTrack*>(
				muxer_segment.GetTrackByNumber(aud_track));
			if (!audio) {
				printf("\n Could not get audio track.\n");
				return EXIT_FAILURE;
			}

			if (track_name)
				audio->set_name(track_name);

			audio->set_codec_id(pAudioTrack->GetCodecId());

			size_t private_size;
			const unsigned char* const private_data =
				pAudioTrack->GetCodecPrivate(private_size);
			if (private_size > 0) {
				if (!audio->SetCodecPrivate(private_data, private_size)) {
					printf("\n Could not add audio private data.\n");
					return EXIT_FAILURE;
				}
			}

			const long long bit_depth = pAudioTrack->GetBitDepth();
			if (bit_depth > 0)
				audio->set_bit_depth(bit_depth);

			if (pAudioTrack->GetCodecDelay())
				audio->set_codec_delay(pAudioTrack->GetCodecDelay());
			if (pAudioTrack->GetSeekPreRoll())
				audio->set_seek_pre_roll(pAudioTrack->GetSeekPreRoll());
		}
	}

	// Set Cues element attributes
	mkvmuxer::Cues* const cues = muxer_segment.GetCues();
	cues->set_output_block_number(true);
	if (vid_track)
		muxer_segment.CuesTrack(vid_track);
	if (aud_track)
		muxer_segment.CuesTrack(aud_track);

	// Write clusters
	unsigned char* data = NULL;
	long data_len = 0;

	const mkvparser::Cluster* cluster = parser_segment->GetFirst();

	while (cluster != NULL && !cluster->EOS()) {
		const mkvparser::BlockEntry* block_entry;

		long status = cluster->GetFirst(block_entry);

		if (status) {
			printf("\n Could not get first block of cluster.\n");
			return EXIT_FAILURE;
		}

		while (block_entry != NULL && !block_entry->EOS()) {
			const mkvparser::Block* const block = block_entry->GetBlock();
			const long long trackNum = block->GetTrackNumber();
			const mkvparser::Track* const parser_track =
				parser_tracks->GetTrackByNumber(static_cast<unsigned long>(trackNum));

			// When |parser_track| is NULL, it means that the track number in the
			// Block is invalid (i.e.) the was no TrackEntry corresponding to the
			// track number. So we reject the file.
			if (!parser_track) {
				return EXIT_FAILURE;
			}

			const long long track_type = parser_track->GetType();
			const long long time_ns = block->GetTime(cluster);

			if ((track_type == Track::kAudio) ||
				(track_type == Track::kVideo)) {
				const int frame_count = block->GetFrameCount();

				for (int i = 0; i < frame_count; ++i) {
					const mkvparser::Block::Frame& frame = block->GetFrame(i);

					if (frame.len > data_len) {
						delete[] data;
						data = new unsigned char[frame.len];
						if (!data)
							return EXIT_FAILURE;
						data_len = frame.len;
					}

					if (frame.Read(&reader, data))
						return EXIT_FAILURE;

					mkvmuxer::Frame muxer_frame;
					if (!muxer_frame.Init(data, frame.len))
						return EXIT_FAILURE;
					muxer_frame.set_track_number(track_type == Track::kAudio ? aud_track
						: vid_track);
					if (block->GetDiscardPadding())
						muxer_frame.set_discard_padding(block->GetDiscardPadding());
					muxer_frame.set_timestamp(time_ns);
					muxer_frame.set_is_key(block->IsKey());
					if (!muxer_segment.AddGenericFrame(&muxer_frame)) {
						printf("\n Could not add frame.\n");
						return EXIT_FAILURE;
					}
				}
			}

			status = cluster->GetNext(block_entry, block_entry);

			if (status) {
				printf("\n Could not get next block of cluster.\n");
				break;
			}
		}

		cluster = parser_segment->GetNext(cluster);
	}

	// We have exhausted all video and audio frames in the input file.
	// Flush any remaining metadata frames to the output file.

	const double input_duration =
		static_cast<double>(segment_info->GetDuration()) / timeCodeScale;
	muxer_segment.set_duration(input_duration);
	if (!muxer_segment.Finalize()) {
		printf("Finalization of segment failed.\n");
		return EXIT_FAILURE;
	}

	reader.Close();
	writer.Close();

	delete[] data;

	return EXIT_SUCCESS;
}

// Clean up the muxer
void XmfWebMMuxer_Cleanup(XmfWebMMuxer* muxer) {
}


bool CopyVideoProjection(const mkvparser::Projection& parser_projection,
	mkvmuxer::Projection* muxer_projection) {
	typedef mkvmuxer::Projection::ProjectionType MuxerProjType;
	const int kTypeNotPresent = mkvparser::Projection::kTypeNotPresent;
	if (parser_projection.type != kTypeNotPresent) {
		muxer_projection->set_type(
			static_cast<MuxerProjType>(parser_projection.type));
	}
	if (parser_projection.private_data &&
		parser_projection.private_data_length > 0) {
		if (!muxer_projection->SetProjectionPrivate(
			parser_projection.private_data,
			parser_projection.private_data_length)) {
			return false;
		}
	}

	const float kValueNotPresent = mkvparser::Projection::kValueNotPresent;
	if (parser_projection.pose_yaw != kValueNotPresent)
		muxer_projection->set_pose_yaw(parser_projection.pose_yaw);
	if (parser_projection.pose_pitch != kValueNotPresent)
		muxer_projection->set_pose_pitch(parser_projection.pose_pitch);
	if (parser_projection.pose_roll != kValueNotPresent)
		muxer_projection->set_pose_roll(parser_projection.pose_roll);
	return true;
}
