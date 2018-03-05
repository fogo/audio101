#include <audio101/pcm_player.hpp>

audio101::PcmPlayer::PcmPlayer(const char *device, unsigned sample_rate, unsigned channels)
: sample_rate_(sample_rate),
  channels_(channels)
{
    int err;
    
    // Open the PCM device in playback mode
    if ((err = snd_pcm_open(&pcm_handle_, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::string msg("Can't open PCM device \"");
        msg += device;
        msg += "\", error is: ";
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }

    // Allocate parameters object and fill it with default values.
    // Note that all configurations below are hardware (hw). ALSA
    // splits configuration in software (sw) and hardware.
    // Hardware covers format expected and way buffer is written/read.
    // Software, not used here, offers some control options like
    // threshold of samples in buffer to start playback, for example.
    if ((err = snd_pcm_hw_params_malloc(&pcm_params_)) < 0) {
        std::string msg("Unable to allocate memory for hw params, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }
    if ((err = snd_pcm_hw_params_any(pcm_handle_, pcm_params_)) < 0) {
        std::string msg("Unable to fill hw params configuration, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }

    // Here we start to configure the playback device to our desired
    // parameters.
    // First of all, I'm just going to use SND_PCM_ACCESS_RW_INTERLEAVED
    // to access buffer. In rough terms, this means:
    // * We are going to write a copy of our buffer to device. This is
    //   slower than some direct-access methods provided by ALSA, but it
    //   is simpler to use, so bear with me. The use of `snd_pcm_writei`
    //   in player thread loop is a direct consequence of this choice.
    // * Interleaved is important when audio is stereo. It means that
    //   samples of channel 0 and channel 1 come one after one in our
    //   file. Below an example how a stereo audio would look like when
    //   interleaved or not (--- = channel 0, *** = channel 1, | = end
    //   of sample):
    //
    //   |---|***|---|***|---|***| -> interleaved
    //   |---|---|---|***|***|***| -> non-interleaved
    //
    //   In my experience, interleaved is a lot more common. For instance,
    //   if you export a song to RAW/PCM using Audacity, this is the mode
    //   used.
    if ((err = snd_pcm_hw_params_set_access(pcm_handle_, pcm_params_, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        std::string msg("Can't set interleaved mode, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }

    // We are also going to assume PCM audio is always going to use
    // signed 16-bits little endian for every sample.
    if ((err = snd_pcm_hw_params_set_format(pcm_handle_, pcm_params_, SND_PCM_FORMAT_S16_LE)) < 0) {
        std::string msg("Can't set format, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }

    // Number of channels (mono=1 / stereo=2)
    if ((err = snd_pcm_hw_params_set_channels(pcm_handle_, pcm_params_, channels_)) < 0) {
        std::string msg("Can't set channels number, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }

    // Configure sample rate, in Hz
    if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle_, pcm_params_, &sample_rate_, 0)) < 0) {
        std::string msg("Can't set sample rate, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }

    // Write parameters to hardware
    if ((err = snd_pcm_hw_params(pcm_handle_, pcm_params_)) < 0) {
        std::string msg("Can't set hardware parameters, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }
}

audio101::PcmPlayer::~PcmPlayer()
{
    if (/*state_ == PlayerState::playing && */!stopped_) {
        stop();
    }
    snd_pcm_close(pcm_handle_);
    free(pcm_params_);
}

std::string audio101::PcmPlayer::device() const
{
    return snd_pcm_name(pcm_handle_);
}

audio101::PlayerState audio101::PcmPlayer::state() const
{
    return state_;
}

unsigned audio101::PcmPlayer::channels() const
{
    unsigned actual_channels;
    snd_pcm_hw_params_get_channels(pcm_params_, &actual_channels);
    return actual_channels;
}

unsigned audio101::PcmPlayer::sample_rate() const
{
    unsigned actual_rate;
    snd_pcm_hw_params_get_rate(pcm_params_, &actual_rate, 0);
    return actual_rate;
}

unsigned audio101::PcmPlayer::bytes_per_sample() const {
    // every sample is 2 bytes because format is signed 16 bits LE
    static const unsigned BYTES_PER_SAMPLE = 2;
    return BYTES_PER_SAMPLE;
}

std::chrono::seconds audio101::PcmPlayer::total_seconds() const {
    return std::chrono::seconds(total_seconds_);
}

std::string audio101::PcmPlayer::filename() const {
    return filename_;
}

void audio101::PcmPlayer::play_for(int fd, std::chrono::seconds duration)
{
    check_playing();

    total_seconds_ = duration;
    do_play(fd, duration);
}

void audio101::PcmPlayer::play_file(int fd) {
    check_playing();

    long filesize = lseek(fd, 0, SEEK_END);
    total_seconds_ = calc_duration(filesize);
    lseek(fd, 0, SEEK_SET);

    do_play(fd, total_seconds_);
}

void audio101::PcmPlayer::play_file(const std::string &path) {
    check_playing();

    file_ = fopen(path.c_str(), "rb");
    fseek(file_, 0, SEEK_END);
    long filesize = ftell(file_);
    total_seconds_ = calc_duration(filesize);
    fseek(file_, 0, SEEK_SET);

    filename_ = path;
    do_play(fileno(file_), total_seconds_);
}

void audio101::PcmPlayer::play_for(const std::string &path, std::chrono::seconds duration) {
    check_playing();

    total_seconds_ = duration;
    file_ = fopen(path.c_str(), "rb");

    filename_ = path;
    do_play(fileno(file_), duration);
}

void audio101::PcmPlayer::do_play(int fd, std::chrono::seconds duration)
{
    auto loop = &PcmPlayer::play_loop;
    state_ = PlayerState::playing;
    player_thrd_ = std::thread(loop, this, fd, duration);
}

void audio101::PcmPlayer::play_loop(int fd, std::chrono::seconds duration)
{
    pthread_setname_np(pthread_self(), "player");
    snd_pcm_uframes_t frames;
    pause_mutex_.unlock();
    stopped_ = false;

    // allocate buffer to hold single period
    snd_pcm_hw_params_get_period_size(pcm_params_, &frames, 0);
    size_t buff_size = frames * channels() * bytes_per_sample();
    std::unique_ptr<unsigned char[]> buffer(new unsigned char[buff_size]);
    unsigned char *bufferp = buffer.get();

    unsigned period_time;
    snd_pcm_hw_params_get_period_time(pcm_params_, &period_time, NULL);

    for (int loops = (duration.count() * 1000000) / period_time; loops > 0; loops--) {
        int err = 0;
        // we can't keep writing in buffer while is paused, wait until resumed again
        {
            std::lock_guard<std::mutex> pause_lock(pause_mutex_);
        }
        if (stopped_) {
            break;
        }
        if (read(fd, bufferp, buff_size) == 0) {
            // in an actual player, this would be better placed in a log
            fprintf(stderr, "Early end of file, duration longer than file?\n");
            break;
        }

        snd_pcm_sframes_t n_frames = snd_pcm_writei(pcm_handle_, bufferp, frames);
        err = static_cast<int>(n_frames);
        if (err == -EAGAIN) {
            continue;
        }
        if (err < 0) {
            if (xrun_recovery(err) < 0) {
                fprintf(stderr, "Write error: %s\n", snd_strerror(err));
                stopped_ = true;
                break;
            }
        }
    }
    if (stopped_) {
        snd_pcm_drop(pcm_handle_);
    } else {
        snd_pcm_drain(pcm_handle_);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = PlayerState::idle;
    }
}

int audio101::PcmPlayer::xrun_recovery(int err) const {
    if (err == -EPIPE) {
        // underrun: can happen when an application does not feed
        // new samples in time to device (due CPU usage, for instance).
        err = snd_pcm_prepare(pcm_handle_);
        if (err < 0) {
            fprintf(stderr, "Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
        }
        return 0;
    } else if (err == -ESTRPIPE) {
        // This error means that system has suspended drivers.
        // The application should wait in loop when snd_pcm_resume() != -EAGAIN
        // and then call snd_pcm_prepare() when snd_pcm_resume() return an
        // error code. If snd_pcm_resume() does not fail (a zero value is
        // returned), driver supports resume and the snd_pcm_prepare() call
        // can be omitted.
        while ((err = snd_pcm_resume(pcm_handle_)) == -EAGAIN) {
            // wait until the suspend flag is released, unless stopped
            if (stopped_) {
                return -ESTRPIPE;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (err < 0) {
            err = snd_pcm_prepare(pcm_handle_);
            if (err < 0) {
                fprintf(stderr, "Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
            }
        }
        return 0;
    }
    return err;
}

void audio101::PcmPlayer::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == PlayerState::idle) {
        throw std::runtime_error("Unable to pause, not playing anything");
    }
    pause_mutex_.lock();
    int err;
    if ((err = snd_pcm_pause(pcm_handle_, 1)) < 0) {
        std::string msg("Unable to pause, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }
    state_ = PlayerState::paused;
}

void audio101::PcmPlayer::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_== PlayerState::idle) {
        throw std::runtime_error("Unable to resume, not playing anything");
    }
    pause_mutex_.unlock();
    int err;
    if ((err = snd_pcm_pause(pcm_handle_, 0)) < 0) {
        std::string msg("Unable to resume, error is: ");
        msg += snd_strerror(err);
        throw std::runtime_error(msg.c_str());
    }
    state_ = PlayerState::playing;
}

void audio101::PcmPlayer::stop() {
    // TODO: I guess this isn't going to work if it started draining!
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != PlayerState::idle) {
            stopped_ = true;
        }
    }
    pause_mutex_.unlock();
    if (player_thrd_.joinable()) {
        player_thrd_.join();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = PlayerState::idle;
        if (file_ != nullptr) {
            fclose(file_);
            file_ = nullptr;
            filename_.clear();
        }
    }
}

void audio101::PcmPlayer::check_playing() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != PlayerState::idle) {
        throw std::runtime_error("Playback already in progress");
    }
}

std::chrono::seconds audio101::PcmPlayer::calc_duration(long filesize) const {
    return std::chrono::seconds((filesize) / (sample_rate_ * bytes_per_sample() * channels_));
}
