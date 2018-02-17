/**
 * Plays a PCM file to your speakers.
 *
 * Usage:
 * // to play a file for 5 seconds using standard input
 * $ playpcm -r8000 -c1 -d5 < /dev/random
 * // to play a whole file given its path
 * $ playpcm -r16000 -c2 audio.raw
 *
 * You will need Advanced Linux Sound Architecture (ALSA) dev libraries.
 * If you are lazy like me, on Ubuntu you can install `libasound2-dev` to
 * have necessary dependencies. Maybe I'll try to come up with a conan ALSA
 * lib package and have this improved :)
 *
 * References:
 * - [ALSA lib reference](https://www.alsa-project.org/alsa-doc/alsa-lib/index.html)
 * - [ALSA project about PCM](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html)
 * - [ALSA examples](https://www.alsa-project.org/alsa-doc/alsa-lib/examples.html)
 * - [Simple sound playback using ALSA API and libasound by Alessandro Ghedini](https://gist.github.com/ghedo/963382)
 */

#include <audio101/pcm_player.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
#include <cmath>
#include <signal.h>
#include <getopt.h>
#include <limits.h>

// going to play in whatever is default audio device
static const char *device = "default";

// flag toggled when audio should be paused/resumed
static bool toggle_pause_audio = false;
static bool stopped = false;

/**
 * Loop that handles playback control. Basically pauses/resumes/stops
 * audio if requested by user.
 */
void control_loop(audio101::PcmPlayer &player) {
    pthread_setname_np(pthread_self(), "control");

    // could be improved by using event to listen to pause/resume.
    // for simplicity, right now kept as polling.
    audio101::PlayerState state;
    while ((state = player.state()) != audio101::psIdle) {
        if (stopped) {
            player.stop();
            stopped = false;
            break;
        }
        if (state == audio101::psPlaying) {
            if (toggle_pause_audio) {
                toggle_pause_audio = false;
                player.pause();
            }
        } else if (state == audio101::psPaused) {
            if (toggle_pause_audio) {
                toggle_pause_audio = false;
                player.resume();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

/**
 * Loop that prints a simple dashboard in stdout showing a few
 * properties about audio being played plus a timer showing
 * audio progress.
 */
void dashboard_loop(audio101::PcmPlayer &player) {
    pthread_setname_np(pthread_self(), "dashboard");

    auto start = std::chrono::steady_clock::now();

    std::string filename(player.filename());
    if (!filename.empty()) {
        printf("filename: '%s'\n", basename(filename.c_str()));
    }
    printf("device: '%s'\n", player.device().c_str());
    unsigned int channels_ = player.channels();
    assert(channels_ == 1 || channels_ == 2);
    printf("channels: %i ", channels_);
    if (channels_ == 1)
        printf("(mono)\n");
    else if (channels_ == 2)
        printf("(stereo)\n");
    printf("rate: %u Hz\n", player.sample_rate());
    printf("duration: %ld s\n", player.total_seconds().count());

    audio101::PlayerState state;
    int former_delta = 0;
    unsigned delta = 0;

    std::string paused("(paused)");
    std::string nothing;
    std::string rollback(paused.size(), ' ');
    rollback += std::string(paused.size(), '\b');
    std::string *suffix = &nothing;

    // could be improved by using event to listen to done.
    // for simplicity, right now kept as polling.
    while ((state = player.state()) != audio101::psIdle) {
        const auto current = std::chrono::steady_clock::now();
        if (state == audio101::psPlaying) {
            delta = static_cast<unsigned >(
                    std::ceil(
                            std::chrono::duration<double, std::milli>(
                                    current - start).count() / 1000));
            // In case it was paused then resumed,  we need to sum old
            // duration to current
            delta += former_delta;
            suffix = (suffix == &paused) ? &rollback : &nothing;
        } else if (state == audio101::psPaused) {
            // Store old duration and keep updating start until resumed
            start = current;
            // TODO: this isn't ideal, it still messes timer after some pauses/resumes
            former_delta = delta - 1;
            suffix = &paused;
        }

        unsigned minutes = delta / 60;
        unsigned seconds = delta % 60;
        printf("\rtime: %d:%02d%s", minutes, seconds, suffix->c_str());
        fflush(stdout);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    printf("\n");
}

/**
 * Loop that handles asynchronous signals.
 *
 * For now, basically handles SIGUSR1, which is reserved for
 * pausing/resuming, and SIGUSR2, used to stop audio (a ctrl+c
 * also does the job but this is more gracious!).
 */
void signal_loop() {
    pthread_setname_np(pthread_self(), "signal");
    sigset_t sigs_to_catch;
    int caught;

    sigemptyset(&sigs_to_catch);
    sigaddset(&sigs_to_catch, SIGUSR1);
    sigaddset(&sigs_to_catch, SIGUSR2);
    for (;;) {
        sigwait(&sigs_to_catch, &caught);
        switch (caught) {
            case SIGUSR1: {
                toggle_pause_audio = true;
                break;
            }
            case SIGUSR2: {
                stopped = true;
                break;
            }
            default: {
                // not interested in this signal
            }
        }
    }
}

/**
 * Command line help.
 */
void help(char *name)
{
    printf("Usage:\n");
    printf(" %s -r <INT> -c <INT> [-d <INT>] <PCM_FILE>\n", name);
    printf(" %s -r <INT> -c <INT> [-d <INT>] < <PCM_FILE>\n", name);
    printf("\n");
    printf("Plays a PCM audio file.\n");
    printf("\n");
    printf("To pause/resume send a SIGUSR1 signal to %s.\n", name);
    printf("To stop send a SIGUSR2 signal to %s.\n", name);
    printf("\nGeneral Options:\n");
    printf(" -h, --help           This help\n");
    printf(" -r, --rate           Sample rate (Hz)\n");
    printf(" -c, --channels       Number of channels (1=mono, 2=stereo)\n");
    printf(" -d, --duration       Duration of playback (seconds). If omitted plays whole file.\n");
    printf("\n");
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        help(argv[0]);
        return 1;
    }

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"rate", required_argument, NULL, 's'},
        {"channels", required_argument, NULL, 'c'},
        {"duration", optional_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };

    int option, option_index;
    unsigned rate = 0;
    unsigned channels = 0;
    unsigned total_seconds = 0;
    while ((option = getopt_long(argc, argv, "hr:c:d:", long_options, &option_index)) != -1) {
        switch (option) {
            case 'r': {
                rate = static_cast<unsigned>(std::stoul(optarg));
                break;
            }
            case 'c': {
                channels = static_cast<unsigned>(std::stoul(optarg));
                break;
            }
            case 'd': {
                total_seconds = static_cast<unsigned>(std::stoul(optarg));
                break;
            }
            case 'h':
            default: {
                help(argv[0]);
                return (option =='h') ? 0 : 1;
            }
        }
    }
    assert(rate > 0);
    assert(channels > 0);

    bool from_stdin = optind == argc;

    // main thread and its threads block all asynchronous signals
    // we are interested, as otherwise *any* thread could end up
    // handling it and it is far from what we desired. let worker
    // threads do their job and signal thread handle signals.
    sigset_t async_signals;
    sigemptyset(&async_signals);
    sigaddset(&async_signals, SIGUSR1);
    sigaddset(&async_signals, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &async_signals, nullptr);

    // signal thread alone will handle our asynchronous signals
    std::thread signal_thrd(signal_loop);
    signal_thrd.detach();

    audio101::PcmPlayer player(device, rate, channels);
    if (from_stdin) {
        if (total_seconds > 0) {
            player.play_for(0, std::chrono::seconds(total_seconds));
        } else {
            player.play_file(0);
        }
    } else {
        if (total_seconds > 0) {
            player.play_for(argv[optind], std::chrono::seconds(total_seconds));
        } else {
            player.play_file(argv[optind]);
        }
    }
    std::thread dashboard_thrd(dashboard_loop, std::ref(player));
    std::thread control_thrd(control_loop, std::ref(player));
    dashboard_thrd.join();
    control_thrd.join();

    return 0;
}
