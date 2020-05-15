// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "webrtc/details/webrtc_connection.h"

#include "rtc_base/thread.h"
#include "api/scoped_refptr.h"
#include "api/peer_connection_interface.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
#include "api/peer_connection_interface.h"
#include "api/video_track_source_proxy.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/win/core_audio_utility_win.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "media/engine/webrtc_media_engine.h"
#include "pc/video_track_source.h"
#include "sdk/media_constraints.h"
#include "test/vcm_capturer.h"

#define _MATH_DEFINES_DEFINED

#include "ffmpeg/ffmpeg_utility.h"
#include <QtGui/QImage>
#include <QtGui/QPainter>

namespace Webrtc::details {

class PeerConnectionObserver : public webrtc::PeerConnectionObserver {
public:
	PeerConnectionObserver(
		Fn<void(const IceCandidate &data)> iceCandidateDiscovered,
		Fn<void(bool)> connectionStateChanged);

	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) override;
	void OnAddStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	void OnRemoveStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
	void OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
	void OnRenegotiationNeeded() override;
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
	void OnStandardizedIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
	void OnConnectionChange(
		webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;
	void OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
	void OnIceCandidate(
		const webrtc::IceCandidateInterface* candidate) override;
	void OnIceCandidateError(
		const std::string& host_candidate,
		const std::string& url,
		int error_code,
		const std::string& error_text) override;
	void OnIceCandidateError(
		const std::string& address,
		int port,
		const std::string& url,
		int error_code,
		const std::string& error_text) override;
	void OnIceCandidatesRemoved(
		const std::vector<cricket::Candidate> &candidates) override;
	void OnIceConnectionReceivingChange(bool receiving) override;
	void OnIceSelectedCandidatePairChanged(
		const cricket::CandidatePairChangeEvent& event) override;
	void OnAddTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;
	void OnTrack(
		rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
	void OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
	void OnInterestingUsage(int usage_pattern) override;

private:
	Fn<void(const IceCandidate &data)> _iceCandidateDiscovered;
	Fn<void(bool)> _connectionStateChanged;

};

PeerConnectionObserver::PeerConnectionObserver(
	Fn<void(const IceCandidate &data)> iceCandidateDiscovered,
	Fn<void(bool)> connectionStateChanged)
: _iceCandidateDiscovered(std::move(iceCandidateDiscovered))
, _connectionStateChanged(std::move(connectionStateChanged)) {
}

void PeerConnectionObserver::OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) {
	bool isConnected = false;
	if (new_state == webrtc::PeerConnectionInterface::SignalingState::kStable) {
		isConnected = true;
	}
	_connectionStateChanged(isConnected);
}

void PeerConnectionObserver::OnAddStream(
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
}

void PeerConnectionObserver::OnRemoveStream(
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
}

void PeerConnectionObserver::OnDataChannel(
	rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
}

void PeerConnectionObserver::OnRenegotiationNeeded() {
}

void PeerConnectionObserver::OnIceConnectionChange(
	webrtc::PeerConnectionInterface::IceConnectionState new_state) {
}

void PeerConnectionObserver::OnStandardizedIceConnectionChange(
	webrtc::PeerConnectionInterface::IceConnectionState new_state) {
}

void PeerConnectionObserver::OnConnectionChange(
	webrtc::PeerConnectionInterface::PeerConnectionState new_state) {
}

void PeerConnectionObserver::OnIceGatheringChange(
	webrtc::PeerConnectionInterface::IceGatheringState new_state) {
}

void PeerConnectionObserver::OnIceCandidate(
		const webrtc::IceCandidateInterface* candidate) {
	if (candidate) {
		auto sdp = std::string();
		if (candidate->ToString(&sdp)) {
			const auto data = IceCandidate{
				.sdp = QString::fromStdString(sdp),
				.sdpMid = QString::fromStdString(candidate->sdp_mid()),
				.mLineIndex = candidate->sdp_mline_index(),
			};
			_iceCandidateDiscovered(data);
		}
	}
}

void PeerConnectionObserver::OnIceCandidateError(
	const std::string& host_candidate,
	const std::string& url,
	int error_code,
	const std::string& error_text) {
}

void PeerConnectionObserver::OnIceCandidateError(
	const std::string& address,
	int port,
	const std::string& url,
	int error_code,
	const std::string& error_text) {
}

void PeerConnectionObserver::OnIceCandidatesRemoved(
	const std::vector<cricket::Candidate>& candidates) {
}

void PeerConnectionObserver::OnIceConnectionReceivingChange(bool receiving) {
}

void PeerConnectionObserver::OnIceSelectedCandidatePairChanged(
	const cricket::CandidatePairChangeEvent& event) {
}

void PeerConnectionObserver::OnAddTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) {
}

void PeerConnectionObserver::OnTrack(
	rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
}

void PeerConnectionObserver::OnRemoveTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
}

void PeerConnectionObserver::OnInterestingUsage(int usage_pattern) {
}

class CreateSessionDescriptionObserver
	: public webrtc::CreateSessionDescriptionObserver {
public:
	explicit CreateSessionDescriptionObserver(
		Fn<void(DescriptionWithType)> done);

	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;

private:
	Fn<void(DescriptionWithType)> _done;

};

CreateSessionDescriptionObserver::CreateSessionDescriptionObserver(
	Fn<void(DescriptionWithType)> done)
: _done(std::move(done)) {
}

void CreateSessionDescriptionObserver::OnSuccess(
		webrtc::SessionDescriptionInterface* desc) {
	if (desc) {
		auto sdp = std::string();
		desc->ToString(&sdp);
		if (_done) {
			const auto data = DescriptionWithType{
				.sdp = QString::fromStdString(sdp),
				.type = QString::fromStdString(desc->type()),
			};
			_done(data);
		}
	}
	_done = nullptr;
}

void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
	_done = nullptr;
}

class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
	explicit SetSessionDescriptionObserver(Fn<void()> done);

	void OnSuccess() override;
	void OnFailure(webrtc::RTCError error) override;

private:
	Fn<void()> _done;

};

SetSessionDescriptionObserver::SetSessionDescriptionObserver(
	Fn<void()> done)
: _done(std::move(done)) {
}

void SetSessionDescriptionObserver::OnSuccess() {
	if (_done) {
		_done();
	}
	_done = nullptr;
}

void SetSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
	_done = nullptr;
}

class CapturerTrackSource : public webrtc::VideoTrackSource {
public:
	static rtc::scoped_refptr<CapturerTrackSource> Create();

protected:
	explicit CapturerTrackSource(
		std::unique_ptr<webrtc::test::VcmCapturer> capturer)
		: VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

private:
	rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
		return capturer_.get();
	}
	std::unique_ptr<webrtc::test::VcmCapturer> capturer_;

};

rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create() {
	const size_t kWidth = 640;
	const size_t kHeight = 480;
	const size_t kFps = 30;
	std::unique_ptr<webrtc::test::VcmCapturer> capturer;
	std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
		webrtc::VideoCaptureFactory::CreateDeviceInfo());
	if (!info) {
		return nullptr;
	}
	int num_devices = info->NumberOfDevices();
	for (int i = 0; i < num_devices; ++i) {
		capturer = std::unique_ptr<webrtc::test::VcmCapturer>(
			webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
		if (capturer) {
			return new rtc::RefCountedObject<CapturerTrackSource>(
				std::move(capturer));
		}
	}
	return nullptr;
}

class VideoRendererAdapter : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
	explicit VideoRendererAdapter(Fn<void(QImage)> frame)
	: _sendFrame(std::move(frame)) {
	}

	void OnFrame(const webrtc::VideoFrame &nativeVideoFrame) override {
		if (decodeFrame(nativeVideoFrame)) {
			renderDecodedFrame(nativeVideoFrame.rotation());
			_sendFrame(_cache);
		}
	}

private:
	bool decodeFrame(const webrtc::VideoFrame &nativeVideoFrame);
	void renderDecodedFrame(int rotation);
	void renderRotatedFrame(int rotation);

	QImage _original, _cache;
	FFmpeg::SwscalePointer _decodeContext;
	Fn<void(QImage)> _sendFrame;

};

bool VideoRendererAdapter::decodeFrame(const webrtc::VideoFrame &nativeVideoFrame) {
	const auto frame = nativeVideoFrame.video_frame_buffer()->ToI420();
	const auto size = QSize{ frame->width(), frame->height() };
	if (size.isEmpty()) {
		return false;
	}
	if (!FFmpeg::GoodStorageForFrame(_original, size)) {
		_original = FFmpeg::CreateFrameStorage(size);
	}
	_decodeContext = FFmpeg::MakeSwscalePointer(
		size,
		AV_PIX_FMT_YUV420P,
		size,
		AV_PIX_FMT_BGRA,
		&_decodeContext);
	Assert(_decodeContext != nullptr);

	// AV_NUM_DATA_POINTERS defined in AVFrame struct
	const uint8_t *src[AV_NUM_DATA_POINTERS] = {
		frame->DataY(),
		frame->DataU(),
		frame->DataV(),
		nullptr
	};
	int srcLineSize[AV_NUM_DATA_POINTERS] = {
		frame->StrideY(),
		frame->StrideU(),
		frame->StrideV(),
		0
	};
	uint8_t *dst[AV_NUM_DATA_POINTERS] = { _original.bits(), nullptr };
	int dstLineSize[AV_NUM_DATA_POINTERS] = { _original.bytesPerLine(), 0 };

	const auto lines = sws_scale(
		_decodeContext.get(),
		src,
		srcLineSize,
		0,
		_original.height(),
		dst,
		dstLineSize);

	Ensures(lines == _original.height());
	return true;
}

void VideoRendererAdapter::renderDecodedFrame(int rotation) {
	if (rotation) {
		renderRotatedFrame(rotation);
	} else {
		_cache = _original;
	}
}

void VideoRendererAdapter::renderRotatedFrame(int rotation) {
	const auto size = _original.size();
	const auto cacheSize = FFmpeg::RotationSwapWidthHeight(rotation)
		? size.transposed()
		: size;
	_cache = FFmpeg::CreateFrameStorage(cacheSize);
	const auto rotated = [](QRect rect, int rotation) {
		switch (rotation) {
		case 0: return rect;
		case 90: return QRect(
			rect.y(),
			-rect.x() - rect.width(),
			rect.height(),
			rect.width());
		case 180: return QRect(
			-rect.x() - rect.width(),
			-rect.y() - rect.height(),
			rect.width(),
			rect.height());
		case 270: return QRect(
			-rect.y() - rect.height(),
			rect.x(),
			rect.height(),
			rect.width());
		}
		Unexpected("Rotation in PaintFrameInner.");
	};
	QPainter p(&_cache);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	p.setRenderHint(QPainter::HighQualityAntialiasing);
	if (rotation) {
		p.rotate(rotation);
	}
	const auto rect = rotated(_cache.rect(), rotation);
	p.drawImage(rect, _original);
}

Connection::Connection(crl::weak_on_queue<Connection> weak)
: _weak(std::move(weak)) {
	init();
}

Connection::~Connection() = default;

void Connection::init() {
	_networkThread = rtc::Thread::CreateWithSocketServer();
	_networkThread->SetName("network_thread", _networkThread.get());
	auto result = _networkThread->Start();
	Assert(result);

	_workerThread = rtc::Thread::Create();
	_workerThread->SetName("worker_thread", _workerThread.get());
	result = _workerThread->Start();
	Assert(result);

	_signalingThread = rtc::Thread::Create();
	_signalingThread->SetName("signaling_thread", _signalingThread.get());
	result = _signalingThread->Start();
	Assert(result);

	webrtc::PeerConnectionFactoryDependencies dependencies;
	dependencies.network_thread = _networkThread.get();
	dependencies.worker_thread = _workerThread.get();
	dependencies.signaling_thread = _signalingThread.get();
	dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
	cricket::MediaEngineDependencies media_deps;

	//media_deps.adm = webrtc::webrtc_win::CreateWindowsCoreAudioAudioDeviceModuleFromInputAndOutput(
	//	std::make_unique<webrtc::webrtc_win::CoreAudioInput>(true),
	//	std::make_unique<webrtc::webrtc_win::CoreAudioOutput>(true),
	//	dependencies.task_queue_factory.get());
	//
	// AudioDeviceModule::Create must be called on worker_thread.
	// It has checks inside that Init() is called on the same thread as
	// the constructor and Init() is always called on the worker_thread.
	//
	//media_deps.adm = webrtc::AudioDeviceModule::Create(
	//	webrtc::AudioDeviceModule::kPlatformDefaultAudio,
	//	dependencies.task_queue_factory.get());
	media_deps.task_queue_factory = dependencies.task_queue_factory.get();
	media_deps.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
	media_deps.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
	media_deps.video_encoder_factory = webrtc::CreateBuiltinVideoEncoderFactory();
	media_deps.video_decoder_factory = webrtc::CreateBuiltinVideoDecoderFactory();
	media_deps.audio_processing = webrtc::AudioProcessingBuilder().Create();
	dependencies.media_engine = cricket::CreateMediaEngine(std::move(media_deps));
	dependencies.call_factory = webrtc::CreateCallFactory();
	dependencies.event_log_factory =
		std::make_unique<webrtc::RtcEventLogFactory>(dependencies.task_queue_factory.get());
	dependencies.network_controller_factory = nullptr;
	dependencies.media_transport_factory = nullptr;
	_nativeFactory = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	config.continual_gathering_policy = webrtc::PeerConnectionInterface::ContinualGatheringPolicy::GATHER_CONTINUALLY;
	webrtc::PeerConnectionInterface::IceServer iceServer;
	iceServer.uri = "stun:stun.l.google.com:19302";
	config.servers.push_back(iceServer);

	const auto runner = _weak.runner();
	_observer = std::make_unique<PeerConnectionObserver>([=](
			const IceCandidate &data) {
		runner([=] { _iceCandidateDiscovered.fire_copy(data); });
	}, [=](bool connected) {
		runner([=] { _connectionStateChanged.fire_copy(connected); });
	});
	_peerConnection = _nativeFactory->CreatePeerConnection(
		config,
		nullptr,
		nullptr,
		_observer.get());
	Assert(_peerConnection != nullptr);

	std::vector<std::string> streamIds;
	streamIds.push_back("stream");

	cricket::AudioOptions options;
	rtc::scoped_refptr<webrtc::AudioSourceInterface> audioSource = _nativeFactory->CreateAudioSource(options);
	_localAudioTrack = _nativeFactory->CreateAudioTrack("audio0", audioSource);
	_peerConnection->AddTrack(_localAudioTrack, streamIds);

	_videoTrackSource = CapturerTrackSource::Create();
	_nativeVideoSource = webrtc::VideoTrackSourceProxy::Create(
		_signalingThread.get(),
		_workerThread.get(),
		_videoTrackSource);

	_localVideoTrack = _nativeFactory->CreateVideoTrack("video0", _nativeVideoSource);
	_peerConnection->AddTrack(_localVideoTrack, streamIds);

	startRemoteVideo();
}

rpl::producer<IceCandidate> Connection::iceCandidateDiscovered() const {
	return _iceCandidateDiscovered.events();
}

rpl::producer<bool> Connection::connectionStateChanged() const {
	return _connectionStateChanged.events();
}

rpl::producer<QImage> Connection::frameReceived() const {
	return _frames.events();
}

void Connection::close() {
	_peerConnection->Close();
}

void Connection::setIsMuted(bool muted) {
	_localAudioTrack->set_enabled(!muted);
}

void Connection::getOffer(Fn<void(DescriptionWithType)> done) {
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
	options.offer_to_receive_audio = 1;
	options.offer_to_receive_video = 1;

	rtc::scoped_refptr<CreateSessionDescriptionObserver> observer(
		new rtc::RefCountedObject<CreateSessionDescriptionObserver>(
			std::move(done)));
	_peerConnection->CreateOffer(observer, options);
}

void Connection::getAnswer(Fn<void(DescriptionWithType)> done) {
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
	options.offer_to_receive_audio = 1;
	options.offer_to_receive_video = 1;

	rtc::scoped_refptr<CreateSessionDescriptionObserver> observer(
		new rtc::RefCountedObject<CreateSessionDescriptionObserver>(
			std::move(done)));
	_peerConnection->CreateAnswer(observer, options);
}

void Connection::setLocalDescription(
		const DescriptionWithType &data,
		Fn<void()> done) {
	webrtc::SdpParseError error;
	const auto sessionDescription = webrtc::CreateSessionDescription(
		data.type.toStdString(),
		data.sdp.toStdString(),
		&error);
	if (sessionDescription) {
		rtc::scoped_refptr<SetSessionDescriptionObserver> observer(
			new rtc::RefCountedObject<SetSessionDescriptionObserver>(
				std::move(done)));
		_peerConnection->SetLocalDescription(observer, sessionDescription);
	}
}

void Connection::setRemoteDescription(
		const DescriptionWithType &data,
		Fn<void()> done) {
	webrtc::SdpParseError error;
	const auto sessionDescription = webrtc::CreateSessionDescription(
		data.type.toStdString(),
		data.sdp.toStdString(),
		&error);
	if (sessionDescription) {
		rtc::scoped_refptr<SetSessionDescriptionObserver> observer(
			new rtc::RefCountedObject<SetSessionDescriptionObserver>(
				std::move(done)));
		_peerConnection->SetRemoteDescription(observer, sessionDescription);
	}
}

void Connection::addIceCandidate(const IceCandidate &data) {
	webrtc::SdpParseError error;
	const auto iceCandidate = webrtc::CreateIceCandidate(
		data.sdpMid.toStdString(),
		data.mLineIndex,
		data.sdp.toStdString(),
		&error);
	if (iceCandidate) {
		auto nativeCandidate = std::unique_ptr<webrtc::IceCandidateInterface>(
			iceCandidate);
		_peerConnection->AddIceCandidate(std::move(nativeCandidate), [](const webrtc::RTCError &error) {
			const auto type = error.type();
		});
	}
}

void Connection::startRemoteVideo() {
	if (_remoteVideoTrack == nullptr) {
		for (auto &it : _peerConnection->GetTransceivers()) {
			if (it->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO) {
				_remoteVideoTrack = static_cast<webrtc::VideoTrackInterface*>(
					it->receiver()->track().get());
				break;
			}
		}
	}
	const auto runner = _weak.runner();
	_sink = std::make_unique<VideoRendererAdapter>([=](QImage frame) {
		runner([this, frame = std::move(frame)]() mutable {
			_frames.fire(std::move(frame));
		});
	});
	_remoteVideoTrack->AddOrUpdateSink(_sink.get(), rtc::VideoSinkWants());
}

} // namespace Webrtc::details
