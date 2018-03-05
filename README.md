[![Build Status](https://travis-ci.org/fogo/audio101.svg?branch=master)](https://travis-ci.org/fogo/audio101)

Audio 101
=========

Hello there. In recent times I've worked with transcoding audio for VoIP
telephony. This contact w/ audio in network communications only made stronger
a desire I already had for some time: to understand better how to actually
playback audio in a computer. I've dabbled w/ networks, images, etc before but
 never w/ audio so it seemed something cool to learn more about.

Anyway, now that my motivation is out of way, what you will find here is a
simple but functional audio player capable of playing signed 16-bit little 
endian PCM audio files.

I'll try to keep it as helpful and educational as possible,
but be warned I'm not an expert! I'm just a guy that used some ALSA examples and
other snippets around the web to develop a simple audio player. I added a bunch
of comments throughout the code to share what I learned so far.

## About PCM

By the way, if you don't know what PCM format is, it is essentially the most 
fundamental form of representing analog sounds in digital form. I recommend these
articles to learn about how audio is represented:
[part 1](https://blogs.msdn.microsoft.com/dawate/2009/06/22/intro-to-audio-programming-part-1-how-audio-data-is-represented/) and
[part 2](https://blogs.msdn.microsoft.com/dawate/2009/06/23/intro-to-audio-programming-part-2-demystifying-the-wav-format/).
You may note it is talking about WAV files, but don't worry: WAV are nothing than
PCM audio data preceded by a simple header.

To keep things simple, this audio player only supports
signed 16-bit little endian sampled PCM audio. It would be simple to make this
configurable, but since this is the format I was most used to I ended up
using it here too.

## Player features

If you build `playpcm` bin (you can use `cmake` with provided files to
help you, refer [Travis configuration](.travis.yml) to see how), you will
have access to a command-line player that has the following features:

* it can play signed 16-bit little endian sampled PCM to your default audio device
* it shows a small dashboard about w/ audio information
* it can pause/resume playback
* it can stop playback

Its command-line options are:

```bash
Usage:
 playpcm -r <INT> -c <INT> [-d <INT>] <PCM_FILE>
 playpcm -r <INT> -c <INT> [-d <INT>] < <PCM_FILE>

Plays a PCM audio file.

To pause/resume send a SIGUSR1 signal to playpcm.
To stop send a SIGUSR2 signal to playpcm.

General Options:
 -h, --help           This help
 -r, --rate           Sample rate (Hz)
 -c, --channels       Number of channels (1=mono, 2=stereo)
 -d, --duration       Duration of playback (seconds). If omitted plays whole file.
```

So you can play audio files like this:

```bash
$ playpcm -r 44100 -c 2 samples/free-44k-stereo.raw
filename: 'free-44k-stereo.raw'
device: 'default'
channels: 2 (stereo)
rate: 44100 Hz
duration: 5 s
time: 0:02
```

Or, if you are feeling courageous, like this:

```bash
$ playpcm -r 8000 -c 1 -d 15 < /dev/random
device: 'default'
channels: 1 (mono)
rate: 8000 Hz
duration: 15 s
time: 0:11
```

It isn't a perfect player, as it was just a weekend project, but I guess
it works fine for its modest goals.

## How is this tested?

I took advantage that ALSA library, `libasound`, is linked dynamically
to create a "fake" shared library that is able to replace its symbols.
This shared library is `fakeasound`, and you can find its code in this
repo too.

It is intended to be used w/ `LD_PRELOAD` environment
variable, so its symbols are loaded first and replace the actual ALSA
symbols. You can see this in action when tests in `cmake` configuration
are executed.

I'm far from covering all paths and conditions in player (sorry!) but
I just wanted to see how something like audio playback could be tested
without messing with devices and operating system (and, of course,
avoiding playing audio in tests because w/ automated tests that would
quickly become tiresome).

By the way, the unit tests were created using `fakeasound` used
[Catch2 test framework](https://github.com/catchorg/Catch2).

## How could this evolve to an actual player?

Basically, we'd have to add a bunch of codecs so we could decode all
formats we desired to play. Then instead of straight reading input and
writing it to ALSA buffer, we'd have first to decode it from original
format and only then write it to device buffer.

## How can you create your PCM files

I usually use [Audacity](https://www.audacityteam.org/) for this purpose:

* open an audio file on Audacity
* go to `File -> Export Audio ...`
* in that screen select `Other uncompressed files` option
* set header option to `RAW (header-less)` and encoding `Signed 16-bit PCM`
* save it

You can do the same from command line with `ffmpeg`:

```bash
$ ffmpeg -i song.mp3 -f s16le -acodec pcm_s16le song.raw
```

## References
* [ALSA lib reference](https://www.alsa-project.org/alsa-doc/alsa-lib/index.html)
* [ALSA project about PCM](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html)
* [ALSA examples](https://www.alsa-project.org/alsa-doc/alsa-lib/examples.html)
* [Simple sound playback using ALSA API and libasound by Alessandro Ghedini](https://gist.github.com/ghedo/963382)
* [Intro to Audio Programming, Part 1: How Audio Data is Represented](https://blogs.msdn.microsoft.com/dawate/2009/06/22/intro-to-audio-programming-part-1-how-audio-data-is-represented/)
* [Intro to Audio Programming, Part 2: Demystifying the WAV Format](https://blogs.msdn.microsoft.com/dawate/2009/06/23/intro-to-audio-programming-part-2-demystifying-the-wav-format/)
