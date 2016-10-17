#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include "FFTAudioStream.h"

std::string get_file_contents(const char *filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize((unsigned int) in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return (contents);
    }
    return "";
}

#define WIDTH 1280
#define HEIGHT 720
//#define SHADER_FILE "D:/Code/Projects/sound-visualizer/Shader.frag"
#define SHADER_FILE "D:/Code/Projects/sound-visualizer/Shader_equalizer.frag"
#define SONG_FILE "D:/Code/Projects/sound-visualizer/BTO.ogg"

enum VILUZLIZATION_MODE {
    WAVE, SPECTRUM, EQUALIZER
};

static const int WAVE_DATA_SIZE = 256;
static const int EQUALIZER_COLUMNS = 32;
static const int EQUALIZER_INNERTION = 500;
static const int COLUMNS_MARGIN = 4;
VILUZLIZATION_MODE viluzlizationMode = WAVE;
float waveData[WAVE_DATA_SIZE];
float spectrumData[WAVE_DATA_SIZE];
float previousSpectrumData[WAVE_DATA_SIZE];
float visualSpectrumData[WAVE_DATA_SIZE];
int spectrumIterator = 0;
int columnsInnertion[EQUALIZER_COLUMNS];

// Vertex shader
const GLchar *vertexSource =
        "#version 150 core\n"
                "in vec2 position;"
                "void main() {"
                "   gl_Position = vec4(position, 0.0, 1.0);"
                "}";

struct ToGLStr {
    const char *p;

    ToGLStr(const std::string &s) : p(s.c_str()) {}

    operator const char **() { return &p; }
};

int main() {

    // load an audio soundBuffer from a sound file
    sf::SoundBuffer soundBuffer;
    soundBuffer.loadFromFile(SONG_FILE);

    // initialize and play our custom stream
    FFTAudioStream fftAudioStream;
    fftAudioStream.load(soundBuffer);

    // Create window
    sf::Window window(sf::VideoMode(WIDTH, HEIGHT), "Visualizer", sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    glewExperimental = GL_TRUE;
    glewInit();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    float vertices[] = {
            -1.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            -1.0f, -1.0f,
            1.0f, -1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    std::string shader = get_file_contents(SHADER_FILE);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, ToGLStr(shader), NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "gl_FragColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray((GLuint) posAttrib);
    glVertexAttribPointer((GLuint) posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint timeLoc = glGetUniformLocation(shaderProgram, "iGlobalTime");

    GLint sampleLoc = glGetUniformLocation(shaderProgram, "Spectrum");
    GLint waveLoc = glGetUniformLocation(shaderProgram, "Wavedata");

    GLint resLoc = glGetUniformLocation(shaderProgram, "iResolution");

    glUniform3f(resLoc, WIDTH, HEIGHT, WIDTH * HEIGHT);

    while (window.isOpen()) {
        sf::Event windowEvent;
        while (window.pollEvent(windowEvent)) {
            switch (windowEvent.type) {
                case sf::Event::Closed:
                    fftAudioStream.stop();
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch (windowEvent.key.code) {
                        case sf::Keyboard::P:
                            // Play Sound
                            fftAudioStream.getStatus() != sf::SoundSource::Status::Playing
                            ? fftAudioStream.play()
                            : fftAudioStream.pause();
                            break;
                        case sf::Keyboard::M:
//                          Mode switching
                            break;
                        case sf::Keyboard::R: //Reload the shader
                            //hopefully this is safe
                            glDeleteProgram(shaderProgram);
                            glDeleteShader(fragmentShader);
                            glDeleteShader(vertexShader);

                            vertexShader = glCreateShader(GL_VERTEX_SHADER);
                            glShaderSource(vertexShader, 1, &vertexSource, NULL);
                            glCompileShader(vertexShader);

                            // Create and compile the fragment shader
                            shader = get_file_contents(SHADER_FILE);

                            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                            glShaderSource(fragmentShader, 1, ToGLStr(shader), NULL);
                            glCompileShader(fragmentShader);

                            // Link the vertex and fragment shader into a shader program
                            shaderProgram = glCreateProgram();
                            glAttachShader(shaderProgram, vertexShader);
                            glAttachShader(shaderProgram, fragmentShader);
                            glBindFragDataLocation(shaderProgram, 0, "gl_FragColor");
                            glLinkProgram(shaderProgram);
                            glUseProgram(shaderProgram);

                            // Specify the layout of the vertex data
                            posAttrib = glGetAttribLocation(shaderProgram, "position");
                            glEnableVertexAttribArray((GLuint) posAttrib);
                            glVertexAttribPointer((GLuint) posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

                            timeLoc = glGetUniformLocation(shaderProgram, "iGlobalTime");

                            sampleLoc = glGetUniformLocation(shaderProgram, "Spectrum");
                            waveLoc = glGetUniformLocation(shaderProgram, "Wavedata");

                            resLoc = glGetUniformLocation(shaderProgram, "iResolution");

                            glUniform3f(resLoc, WIDTH, HEIGHT, WIDTH * HEIGHT);
                            break;
                        case sf::Keyboard::Escape:
                            window.close();
                            break;
                    }
                    break;
            }
        }

        GLfloat time = (GLfloat) clock() / (GLfloat) CLOCKS_PER_SEC;
        glUniform1f(timeLoc, time);

        const auto filteredSpectrumDataVector = fftAudioStream.getCurrentSampleSpectrumVector();

//        if (filteredSpectrumDataVector.data() != NULL) {
//            int picker = FFTAudioStream::SAMPLES_TO_STREAM / 4 / WAVE_DATA_SIZE;
//            for (int i = 0; i < WAVE_DATA_SIZE; i++) {
//                spectrumData[i] = (float) filteredSpectrumDataVector[i * picker].re();
//                spectrumData[i] *= 0.000001;
////                spectrumData[i] = 0;
//            }
//        }

        if (filteredSpectrumDataVector.data() != NULL) {
            int picker = FFTAudioStream::SAMPLES_TO_STREAM / 4 / WAVE_DATA_SIZE;
            float sum = 0;
            int columnWidth = WAVE_DATA_SIZE / EQUALIZER_COLUMNS;
            for (int columnNumber = 0; columnNumber < EQUALIZER_COLUMNS; columnNumber++) {
                for (int i = 0; i < columnWidth - COLUMNS_MARGIN; i++) {
                    int currentIndex = columnWidth * columnNumber + i;
                    sum += filteredSpectrumDataVector[currentIndex * picker].re();
                }
                sum *= 0.00000015;
                for (int i = 0; i < columnWidth - COLUMNS_MARGIN; i++) {
                    int currentIndex = columnWidth * columnNumber + i;
                    if (sum > previousSpectrumData[currentIndex]) {
                        columnsInnertion[columnNumber] = EQUALIZER_INNERTION;
                        spectrumData[currentIndex] = sum;
                    } else {
                        spectrumData[currentIndex] = previousSpectrumData[currentIndex] -
                                                     previousSpectrumData[currentIndex] *
                                                     ((float) EQUALIZER_INNERTION + 1 -
                                                      columnsInnertion[columnNumber]) / ((float) EQUALIZER_INNERTION);
                    }
                }
                sum = 0;
            }
        }

        for (int i = 0; i < EQUALIZER_COLUMNS; ++i) {
            if (columnsInnertion[i] != 0) {
                columnsInnertion[i]--;
            }
        }

        for (int i = 0; i < WAVE_DATA_SIZE; ++i) {
            previousSpectrumData[i] = spectrumData[i];
        }

        glUniform1fv(sampleLoc, WAVE_DATA_SIZE, spectrumData);

        const auto filteredWaveDataVector = fftAudioStream.getCurrentSampleWaveVector();

//        if (filteredWaveDataVector.data() != NULL) {
//            int picker = FFTAudioStream::SAMPLES_TO_STREAM / WAVE_DATA_SIZE;
//            for (int i = 0; i < WAVE_DATA_SIZE; i++) {
//                waveData[i] = (float) filteredWaveDataVector[i * picker].re();
//                waveData[i] *= 0.0001;
////                waveData[i] *= 0.0;
//            }
//        }

        glUniform1fv(waveLoc, WAVE_DATA_SIZE, spectrumData);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        window.display();
    }

    // Clean up
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, &vbo);

    glDeleteVertexArrays(1, &vao);
}
