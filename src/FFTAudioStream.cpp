//  Copyright © 2016 Fatih Gazimzyanov. Contacts: virgil7g@gmail.com

//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//          http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and

#include <SFML/Audio.hpp>
#include "FFTAudioStream.h"
#include "fft.h"

void FFTAudioStream::load(const sf::SoundBuffer &buffer) {
    // extract the audio samples from the sound buffer to our own container
    samplesVector.assign(buffer.getSamples(), buffer.getSamples() + buffer.getSampleCount());

    lowFilterValue = 0;
    highFilterValue = SAMPLES_TO_STREAM / 4;
    generateFilterVector();

    currentSampleWaveVector.resize(SAMPLES_TO_STREAM);

    // reset the current playing position
    currentSample = 0;

    // initialize the base class
    initialize(buffer.getChannelCount(), buffer.getSampleRate());
}

void FFTAudioStream::generateFilterVector() {
    complex temporaryComplex;
    filterShortComplexVector.resize(SAMPLES_TO_STREAM / 4);
    for (int i = 0; i < SAMPLES_TO_STREAM / 4; i++) {
        temporaryComplex = (i > lowFilterValue && i < highFilterValue) ? 1.0 : 0.0;
        filterShortComplexVector[i] = temporaryComplex;
    }
}

bool FFTAudioStream::onGetData(sf::SoundStream::Chunk &data) {
    // number of samples to stream every time the function is called;
    // in a more robust implementation, it should be a fixed
    // amount of time rather than an arbitrary number of samples

    getStreamSamples();

    CFFT::Forward(currentSampleWaveVector.data(), SAMPLES_TO_STREAM);

    currentSampleCleanSpectrumVector = currentSampleWaveVector;

    applyFilterToSpectrum(true);

    currentSampleSpectrumVector = currentSampleWaveVector;

    CFFT::Inverse(currentSampleWaveVector.data(), SAMPLES_TO_STREAM);

    filteredWaveDataVector = currentSampleWaveVector;

    applyFilteredSignalToSound(true);

    // set the pointer to the next audio samples to be played
    data.samples = &samplesVector[currentSample];

    // have we reached the end of the sound?
    if (currentSample + SAMPLES_TO_STREAM <= samplesVector.size()) {
        // end not reached: stream the samples and continue
        data.sampleCount = SAMPLES_TO_STREAM;
        currentSample += SAMPLES_TO_STREAM;
        return true;
    } else {
        // end of stream reached: stream the remaining samples and stop playback
        data.sampleCount = samplesVector.size() - currentSample;
        currentSample = samplesVector.size();
        return false;
    }
}

void FFTAudioStream::getStreamSamples() {
    for (int i = 0; i < SAMPLES_TO_STREAM; i++) {
        temporaryShortComplex = samplesVector[currentSample + i];
        currentSampleWaveVector[i] = temporaryShortComplex;
    }
}

void FFTAudioStream::applyFilterToSpectrum(bool isApplied) {
    if (isApplied) {
        for (int T = 0; T < 4; T++) {
            for (int i = 0; i < SAMPLES_TO_STREAM / 4; i++) {
                currentSampleWaveVector[SAMPLES_TO_STREAM / 4 * T + i] =
                        currentSampleWaveVector[SAMPLES_TO_STREAM / 4 * T + i] *
                        filterShortComplexVector[(T % 2 == 0 ? i : SAMPLES_TO_STREAM / 4 - i)];
            }
        }
    }
}

void FFTAudioStream::applyFilteredSignalToSound(bool isApplied) {
    if (isApplied) {
        for (int i = 0; i < SAMPLES_TO_STREAM; i++) {
            temporaryShortComplex = currentSampleWaveVector[i];
            samplesVector[currentSample + i] = (sf::Int16) temporaryShortComplex.re();
        }
    }
}

void FFTAudioStream::onSeek(sf::Time timeOffset) {
    // compute the corresponding sample index according to the sample rate and channel count
    currentSample = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
}

const std::vector<complex> &FFTAudioStream::getCurrentSampleWaveVector() const {
    return filteredWaveDataVector;
}

const std::vector<complex> &FFTAudioStream::getCurrentSampleSpectrumVector() const {
    return currentSampleSpectrumVector;
}

const std::vector<complex> &FFTAudioStream::getCurrentSampleCleanSpectrumVector() const {
    return currentSampleCleanSpectrumVector;
}

float FFTAudioStream::getLowFilterValue() {
    return lowFilterValue;
}

void FFTAudioStream::setLowFilterValue(float lowFilterValue) {
    if ((lowFilterValue >= 0) && (lowFilterValue <= SAMPLES_TO_STREAM / 4)) {
        FFTAudioStream::lowFilterValue = lowFilterValue;
        generateFilterVector();
    }
}

float FFTAudioStream::getHighFilterValue() {
    return highFilterValue;
}

void FFTAudioStream::setHighFilterValue(float highFilterValue) {
    if ((highFilterValue >= 0) && (highFilterValue <= SAMPLES_TO_STREAM / 4)) {
        FFTAudioStream::highFilterValue = highFilterValue;
        generateFilterVector();
    }
}
