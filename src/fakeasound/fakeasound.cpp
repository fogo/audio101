#include <thread>

#include <fakeasound/fakeasound.hpp>

#define FAKEASSOUND_INC_CALL_COUNT() {auto fakeasound = instance(); fakeasound->inc_call_count(__FUNCTION__);}
#define FAKEASSOUND_FAIL_IF_MARKED() {auto fakeasound = instance(); if (fakeasound->should_fail(__FUNCTION__)) return -1;}

static std::unique_ptr<FakeAsound> instance_;

FakeAsound* instance() {
    return instance_.get();
}

void reset_instance() {
    instance_.reset(new FakeAsound);
}

int snd_pcm_open(snd_pcm_t **pcm, const char *name,
                 snd_pcm_stream_t stream, int mode) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    fakeasound->set_device(name);
    return 0;
}

int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t **ptr) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    fakeasound->set_channels(val);
    return 0;
}

int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    fakeasound->set_sample_rate(*val);
    return 0;
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

const char *snd_pcm_name(snd_pcm_t *pcm) {
    FAKEASSOUND_INC_CALL_COUNT();
    auto fakeasound = instance();
    return fakeasound->device().c_str();
}

int snd_pcm_hw_params_get_rate(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    *val = fakeasound->sample_rate();
    return 0;
}

int snd_pcm_hw_params_get_channels(const snd_pcm_hw_params_t *params, unsigned int *val) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    *val = fakeasound->channels();
    return 0;
}

int snd_pcm_close(snd_pcm_t *pcm) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

int snd_pcm_hw_params_get_period_size(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    *frames = 4096;
    return 0;
}

int snd_pcm_hw_params_get_period_time(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    *val = static_cast<unsigned>(fakeasound->period_time().count());
    return 0;
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    auto fakeasound = instance();
    std::this_thread::sleep_for(fakeasound->period_time());
    return 0;
}

int snd_pcm_drop(snd_pcm_t *pcm) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}

int snd_pcm_drain(snd_pcm_t *pcm) {
    FAKEASSOUND_INC_CALL_COUNT();
    FAKEASSOUND_FAIL_IF_MARKED();
    return 0;
}
