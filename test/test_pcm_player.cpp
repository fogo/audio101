#include <audio101/pcm_player.hpp>

#include <fakeasound/fakeasound.hpp>

#include <dlfcn.h>
#include <climits>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


using namespace std::chrono_literals;

static const char *device = "default";
static unsigned rate = 48000;
static unsigned channels = 2;

struct FakeAudioFile {
    std::string path;
    int fd;
};

class PcmPlayerFixture {
public:
    PcmPlayerFixture() {
        setup_fakeasound();
        fakeasound = get_fakeasound();
    }
    virtual ~PcmPlayerFixture() {
        dlclose(handle_);
    }

protected:
    FakeAsound* fakeasound = nullptr;

    audio101::PcmPlayer pcmplayer() {
        audio101::PcmPlayer player(device, rate, channels);
        return player;
    }

    void require_playback_ok(const audio101::PcmPlayer& player, std::chrono::seconds duration) {
        REQUIRE(player.total_seconds() == duration);
        REQUIRE(player.state()  == audio101::PlayerState::playing);
        REQUIRE(wait_predicate(
                [player=&player]() {
                    return player->state() == audio101::PlayerState::idle;
                },
                duration + 1s
        ));
        REQUIRE(fakeasound->call_count("snd_pcm_writei") == (duration / fakeasound->period_time()));
        REQUIRE(fakeasound->call_count("snd_pcm_drain") == 1);
    }

    FakeAudioFile create_audio_file(std::chrono::seconds duration) {
        long filesize = duration.count() * (rate * 2 * channels);

        char path[PATH_MAX] = {0};
        sprintf(path, "%s", ".pcmplayertestXXXXXX");
        int fd = mkstemp(path);
        std::unique_ptr<char[]> buffer{new char[filesize]};
        std::ofstream file(path);
        file.write(buffer.get(), filesize);
        FakeAudioFile audio_file{
                path,
                fd
        };
        return audio_file;
    };

    template <typename T>
    bool wait_predicate(T predicate, std::chrono::seconds timeout) {
        std::chrono::seconds step{1};
        while (timeout.count() > 0) {
            std::this_thread::sleep_for(step);
            if (predicate()) {
                return true;
            }
            timeout -= step;
        }
        return predicate();
    }

private:
    void* handle_ = nullptr;

    void setup_fakeasound() {
        handle_ = dlopen("libfakeasound.so", RTLD_NOW);
        REQUIRE(handle_ != nullptr);
        using Reset = void(*)();
        auto dlreset = reinterpret_cast<Reset>(dlsym(handle_, "reset_instance"));
        dlreset();
    }

    FakeAsound* get_fakeasound() {
        using Instance = FakeAsound*(*)();
        auto dlinstance = reinterpret_cast<Instance>(dlsym(handle_, "instance"));
        auto fakeasound = dlinstance();
        return fakeasound;
    }
};

TEST_CASE_METHOD(PcmPlayerFixture, "Construction", "[audio101::PcmPlayer]") {
    audio101::PcmPlayer player(pcmplayer());

    REQUIRE(player.device() == device);
    REQUIRE(player.sample_rate() == rate);
    REQUIRE(player.channels() == channels);
    REQUIRE(player.state() == audio101::PlayerState::idle);
    REQUIRE(player.total_seconds() == 0s);
    REQUIRE(player.filename().empty());
}

TEST_CASE_METHOD(PcmPlayerFixture, "Play fd w/ duration", "[audio101::PcmPlayer]") {
    audio101::PcmPlayer player(pcmplayer());

    FILE* file = fopen("/dev/random", "r");
    int fd = fileno(file);

    auto duration = 2s;
    player.play_for(fd, duration);
    REQUIRE(player.filename().empty());
    require_playback_ok(player, duration);
}

TEST_CASE_METHOD(PcmPlayerFixture, "Play file w/ duration", "[audio101::PcmPlayer]") {
    audio101::PcmPlayer player(pcmplayer());

    auto duration = 2s;
    const char *path = "/dev/random";
    player.play_for(path, duration);
    REQUIRE(player.filename() == path);
    require_playback_ok(player, duration);
}

TEST_CASE_METHOD(PcmPlayerFixture, "Play file w/o duration", "[audio101::PcmPlayer]") {
    audio101::PcmPlayer player(pcmplayer());

    auto duration = 2s;
    auto audio_file = create_audio_file(duration);

    player.play_file(audio_file.path);
    REQUIRE(player.filename() == audio_file.path);
    require_playback_ok(player, duration);
}

TEST_CASE_METHOD(PcmPlayerFixture, "Play fd w/o duration", "[audio101::PcmPlayer]") {
    audio101::PcmPlayer player(pcmplayer());

    auto duration = 2s;
    auto audio_file = create_audio_file(duration);

    player.play_file(audio_file.fd);
    REQUIRE(player.filename().empty());
    require_playback_ok(player, duration);
}

TEST_CASE_METHOD(PcmPlayerFixture, "Fail opening device", "[audio101::PcmPlayer]") {
    fakeasound->mark_as_failed("snd_pcm_open", true);
    REQUIRE_THROWS(pcmplayer());
}

// TODO: cover a bunch of other paths, like a lot of other failures, pause/resume, stop, etc...
