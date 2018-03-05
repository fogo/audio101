#ifndef AUDIO101_PCM_PLAYER_HPP
#define AUDIO101_PCM_PLAYER_HPP

#include <alsa/asoundlib.h>

#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <fstream>

namespace audio101 {

    enum class PlayerState {
        idle,
        playing,
        paused
    };

    class PcmPlayer {
    public:
        // TODO: sample rate/channels moved to play
        explicit PcmPlayer(const char *device, unsigned sample_rate, unsigned channels);
        /**
         * Not safe if already playing audio!
         */
        PcmPlayer(PcmPlayer&& player) :
            state_(std::move(player.state_)),
            file_(std::move(player.file_)),
            filename_(std::move(player.filename_)),
            stopped_(std::move(player.stopped_)),
            total_seconds_(std::move(player.total_seconds_)),
            sample_rate_(std::move(player.sample_rate_)),
            channels_(std::move(player.channels_)),
            pcm_handle_(std::move(player.pcm_handle_)),
            pcm_params_(std::move(player.pcm_params_)),
            player_thrd_(std::move(player.player_thrd_))
        {}
        ~PcmPlayer();

        /**
         * @return Device playing audio.
         */
        std::string device() const;
        /**
         * @return State of device playing audio.
         */
        PlayerState state() const;
        /**
         * @return Number of channels used by device.
         */
        unsigned channels() const;
        /**
         * @return Sample rate used by device.
         */
        unsigned sample_rate() const;
        /**
         * @return Bytes per sample in audio.
         */
        unsigned bytes_per_sample() const;
        /**
         * @return Number of seconds that playback is going to last.
         */
        std::chrono::seconds total_seconds() const;
        /**
         * @return Name of audio file currently playing. Empty if
         * open w/ file descriptor.
         */
        std::string filename() const;

        /**
         * Play all audio in file descriptor.
         *
         * When played from a file descriptor, file ownership is not claimed
         * and audio file must be opened/closed by client code.
         */
        void play_file(int fd);
        /**
         * Play all audio in file.
         *
         * Opens file for playback and closes it once done.
         */
        void play_file(const std::string &path);
        /**
         * Play audio in file descriptor for given number of seconds.
         *
         * When played from a file descriptor, file ownership is not claimed
         * and audio file must be opened/closed by client code.
         */
        void play_for(int fd, std::chrono::seconds duration);
        /**
         * Play audio file for given number of seconds.
         *
         * Opens file for playback and closes it once done.
         */
        void play_for(const std::string &path, std::chrono::seconds duration);

        /**
         * Pause audio. Call `resume` to resume playback.
         */
        void pause();
        /**
         * Resumes paused audio.
         */
        void resume();
        /**
         * Stops audio playback.
         */
        void stop();

    private:
        /**
         * Playback implementation of audio file descriptor.
         */
        void do_play(int fd, std::chrono::seconds duration);
        /**
         * Player thread loop. Basically reads audio content and writes
         * it to sound device.
         */
        void play_loop(int fd, std::chrono::seconds duration);
        /**
         * If already playing, throws.
         */
        void check_playing() const;
        /**
         * Duration is calculated like this:
         *
         * ```
         * duration = filesize_bytes / (sample_rate_hz * bytes_per_sample * channels)
         * ```
         *
         * Considering right now implementation always assumes 16-bit little
         * endian samples, `bytes_per_sample` is always 2.
         *
         * @param filesize Size in bytes.
         * @return Total duration of audio file.
         */
        std::chrono::seconds calc_duration(long filesize) const;
        /**
         * Attempt of recovery during playback.
         *
         * It is heavily inspired by example below:
         * https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
         *
         * Name comes from recovery from underrun/overrun conditions.
         *
         * @return If negative, unable to recover.
         */
        int xrun_recovery(int err) const;

        PlayerState state_ = PlayerState::idle;
        FILE* file_ = nullptr;
        std::string filename_;
        bool stopped_ = false;
        std::chrono::seconds total_seconds_{0};
        unsigned sample_rate_;
        unsigned channels_;

        snd_pcm_t *pcm_handle_ = nullptr;
        snd_pcm_hw_params_t *pcm_params_ = nullptr;

        mutable std::mutex mutex_;
        std::mutex pause_mutex_;
        std::thread player_thrd_;
    };

};

#endif //AUDIO101_PCM_PLAYER_HPP
