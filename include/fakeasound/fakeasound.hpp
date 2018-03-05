/**
 * `fakeasound` is (still incomplete) drop-in replacement for `libasound`
 * dynamic library. The goal here is to provide a way to be able to test PCM
 * player without using an actual audio device.
 *
 * The library created with code below is intended to be used together with
 * `LD_PRELOAD` environment variable. This way is possible to replace symbols
 * normally implemented by `libasound` by the ones here.
 *
 * Some of the features of `fakeasound` library are:
 * - replaced functions don't play audio and can be test avoiding coupling
 *   to a environment/devices setup (and avoiding audio playback during
 *   automated tests also is a plus, I guess)
 * - it is able to configure some values returned by replaced functions
 * - it is able to track if a `libasound` replaced function was called (see
 *   `call_count`)
 * - it is possible to mark a replaced function to fail, enabling tests to
 *   validate error conditionals without doing crazy things to its system
 *   devices (see `mark_as_failed`)
 *
 * These features are implemented in terms of `FakeAsound` singleton object,
 * which can accessed w/ `instance` function by tests. I also recommend
 * calling `reset_instance` between tests, to clean its state.
 *
 * Right now I only implemented the symbols necessary for my PCM player tests,
 * but it'd be perfectly possible to expand this library to be a complete test
 * replacement for `libasound`.
 */

#ifndef AUDIO101_FAKE_ASOUND_HPP
#define AUDIO101_FAKE_ASOUND_HPP

#include <alsa/asoundlib.h>

#include <string>
#include <unordered_map>
#include <set>

using namespace std::chrono_literals;

class FakeAsound;

extern "C" {
    FakeAsound* instance();
    void reset_instance();
}

class FakeAsound {
public:
    FakeAsound(const FakeAsound&) = delete;
    FakeAsound& operator=(const FakeAsound&) = delete;

    /**
     * Configures device name returned by calls like `snd_pcm_name'.
     */
    template <typename T>
    void set_device(T&& name) {
        device_ = std::forward<T>(name);
    }

    /**
     * Configured device name.
     */
    const std::string& device() const {
        return device_;
    }

    /**
     * Configures sample rate returned by calls like
     * `snd_pcm_hw_params_get_rate`.
     */
    void set_sample_rate(unsigned rate) {
        sample_rate_ = rate;
    }

    /**
     * Configured sample rate.
     */
    unsigned sample_rate() const {
        return sample_rate_;
    }

    /**
     * Configures number of channels returned by calls like
     * `snd_pcm_hw_params_get_channels`.
     */
    void set_channels(unsigned channels) {
        channels_ = channels;
    }

    /**
     * Configured number of channels.
     */
    unsigned channels() const {
        return channels_;
    }

    /**
     * Configured period time used by calls like
     * `snd_pcm_hw_params_get_period_time`.
     */
    void set_period_time(std::chrono::microseconds us) {
        period_time_ = us;
    }

    /**
     * Configured period time.
     */
    std::chrono::microseconds period_time() const {
        return period_time_;
    }

    /**
     * Increments number of calls of a function by one.
     */
    void inc_call_count(std::string fn) {
        if (call_counts_.count(fn) > 0) {
            call_counts_[fn] += 1;
        } else {
            call_counts_[fn] = 1;
        }
    }

    /**
     * Gets number of calls to a function.
     */
    unsigned call_count(std::string fn) const {
        auto count = call_counts_.find(fn);
        if (count != call_counts_.end()) {
            return count->second;
        }
    }

    /**
     * Can mark a function to fail (or not, depending on flag). If a function
     * is marked to fail, it will always return -1, providing a way to
     * reliably test errors.
     */
    void mark_as_failed(const char* fn, bool flag) {
        std::string fn_(fn);
        if (flag) {
            failures_.insert(fn_);
        } else {
            failures_.erase(fn_);
        }
    }

    /**
     * Returns if a function was marked to fail.
     */
    bool should_fail(const char* fn) const {
        return failures_.find(fn) != failures_.end();
    }

private:
    friend FakeAsound* instance();
    friend void reset_instance();
    FakeAsound() = default;

    unsigned sample_rate_{0};
    unsigned channels_ {0};
    unsigned writes_{0};
    std::chrono::microseconds period_time_{1000000us};

    std::string device_;
    std::unordered_map<std::string, unsigned> call_counts_;
    std::set<std::string> failures_;

};

#endif //AUDIO101_FAKE_ASOUND_HPP
