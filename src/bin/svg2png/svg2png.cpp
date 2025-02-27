/*
 * Copyright (c) 2020 - 2022 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <thread>
#include <thorvg.h>
#include <vector>
#include "lodepng.h"
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

using namespace std;

struct PngBuilder
{
    void build(const string& fileName, const uint32_t width, const uint32_t height, uint32_t* buffer)
    {
        //Used ARGB8888 so have to move pixels now
        vector<unsigned char> image;
        image.resize(width * height * 4);
        for (unsigned y = 0; y < height; y++) {
            for (unsigned x = 0; x < width; x++) {
                uint32_t n = buffer[y * width + x];
                image[4 * width * y + 4 * x + 0] = (n >> 16) & 0xff;
                image[4 * width * y + 4 * x + 1] = (n >> 8) & 0xff;
                image[4 * width * y + 4 * x + 2] = n & 0xff;
                image[4 * width * y + 4 * x + 3] = (n >> 24) & 0xff;
            }
        }

        unsigned error = lodepng::encode(fileName, image, width, height);

        //if there's an error, display it
        if (error) cout << "encoder error " << error << ": " << lodepng_error_text(error) << endl;
    }
};

struct Renderer
{
public:
    int render(const char* path, int w, int h, const string& dst, uint32_t bgColor)
    {
        //Canvas
        if (!canvas) createCanvas();
        if (!canvas) {
            cout << "Error: Canvas failure" << endl;
            return 1;
        }

        //Picture
        auto picture = tvg::Picture::gen();
        if (picture->load(path) != tvg::Result::Success) {
            cout << "Error: Couldn't load image " << path << endl;
            return 1;
        }

        if (w == 0 || h == 0) {
            float fw, fh;
            picture->size(&fw, &fh);
            w = static_cast<uint32_t>(fw);
            h = static_cast<uint32_t>(fh);
        } else {
            picture->size(w, h);
        }

        //Buffer
        createBuffer(w, h);
        if (!buffer) {
            cout << "Error: Buffer failure" << endl;
            return 1;
        }

        if (canvas->target(buffer, w, w, h, tvg::SwCanvas::ARGB8888_STRAIGHT) != tvg::Result::Success) {
            cout << "Error: Canvas target failure" << endl;
            return 1;
        }

        //Background color if needed
        if (bgColor != 0xffffffff) {
            uint8_t r = (uint8_t)((bgColor & 0xff0000) >> 16);
            uint8_t g = (uint8_t)((bgColor & 0x00ff00) >> 8);
            uint8_t b = (uint8_t)((bgColor & 0x0000ff));

            auto shape = tvg::Shape::gen();
            shape->appendRect(0, 0, w, h, 0, 0);
            shape->fill(r, g, b, 255);

            if (canvas->push(move(shape)) != tvg::Result::Success) return 1;
        }

        //Drawing
        canvas->push(move(picture));
        canvas->draw();
        canvas->sync();

        //Build Png
        PngBuilder builder;
        builder.build(dst, w, h, buffer);

        cout << "Generated PNG file: " << dst << endl;

        canvas->clear(true);

        return 0;
    }

    void terminate()
    {
        //Terminate ThorVG Engine
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        free(buffer);
    }

private:
    void createCanvas()
    {
        //Canvas Engine
        tvg::CanvasEngine tvgEngine = tvg::CanvasEngine::Sw;

        //Threads Count
        auto threads = thread::hardware_concurrency();

        //Initialize ThorVG Engine
        if (tvg::Initializer::init(tvgEngine, threads) != tvg::Result::Success) {
            cout << "Error: Engine is not supported" << endl;
        }

        //Create a Canvas
        canvas = tvg::SwCanvas::gen();
    }

    void createBuffer(int w, int h)
    {
        uint32_t size = w * h;
        //Reuse old buffer if size is enough
        if (buffer && bufferSize >= size) return;

        //Alloc or realloc buffer
        buffer = (uint32_t*) realloc(buffer, sizeof(uint32_t) * size);
        bufferSize = size;
    }

private:
    unique_ptr<tvg::SwCanvas> canvas = nullptr;
    uint32_t* buffer = nullptr;
    uint32_t bufferSize = 0;
};

struct App
{
public:
    int setup(int argc, char** argv)
    {
        vector<const char*> paths;

        for (int i = 1; i < argc; i++) {
            const char* p = argv[i];
            if (*p == '-') {
                //flags
                const char* p_arg = (i + 1 < argc) ? argv[++i] : nullptr;
                if (p[1] == 'r') {
                    //image resolution
                    if (!p_arg) {
                        cout << "Error: Missing resolution attribute. Expected eg. -r 200x200." << endl;
                        return 1;
                    }

                    const char* x = strchr(p_arg, 'x');
                    if (x) {
                        width = atoi(p_arg);
                        height = atoi(x + 1);
                    }
                    if (!x || width <= 0 || height <= 0) {
                        cout << "Error: Resolution (" << p_arg << ") is corrupted. Expected eg. -r 200x200." << endl;
                        return 1;
                    }

                } else if (p[1] == 'b') {
                    //image background color
                    if (!p_arg) {
                        cout << "Error: Missing background color attribute. Expected eg. -b fa7410." << endl;
                        return 1;
                    }

                    bgColor = (uint32_t) strtol(p_arg, NULL, 16);

                } else {
                    cout << "Warning: Unknown flag (" << p << ")." << endl;
                }
            } else {
                //arguments
                paths.push_back(p);
            }
        }

        int ret = 0;
        if (paths.empty()) {
            //no attributes - print help
            return help();

        } else {
            for (auto path : paths) {
                auto real_path = realFile(path);
                if (real_path) {
                    DIR* dir = opendir(real_path);
                    if (dir) {
                        //load from directory
                        cout << "Trying load from directory \"" << real_path << "\"." << endl;
                        if ((ret = handleDirectory(real_path, dir))) break;

                    } else if (svgFile(path)) {
                        //load single file
                        if ((ret = renderFile(real_path))) break;
                    } else {
                        //not a directory and not .svg file
                        cout << "Error: File \"" << path << "\" is not a proper svg file." << endl;
                    }

                } else {
                    cout << "Error: Invalid file or path name: \"" << path << "\"" << endl;
                }
            }
        }

        //Terminate renderer
        renderer.terminate();

        return ret;
    }

private:
    Renderer renderer;
    uint32_t bgColor = 0xffffffff;
    uint32_t width = 0;
    uint32_t height = 0;
    char full[PATH_MAX];

private:
    int help()
    {
        cout << "Usage:\n   svg2png [SVG files] [-r resolution] [-b bgColor]\n\nFlags:\n    -r set the output image resolution.\n    -b set the output image background color.\n\nExamples:\n    $ svg2png input.svg\n    $ svg2png input.svg -r 200x200\n    $ svg2png input.svg -r 200x200 -b ff00ff\n    $ svg2png input1.svg input2.svg -r 200x200 -b ff00ff\n    $ svg2png . -r 200x200\n\n";
        return 1;
    }

    bool svgFile(const char* path)
    {
        size_t length = strlen(path);
        return length > 4 && (strcmp(&path[length - 4], ".svg") == 0);
    }

    const char* realFile(const char* path)
    {
        //real path
#ifdef _WIN32
        path = fullpath(full, path, PATH_MAX);
#else
        path = realpath(path, full);
#endif
        return path;
    }

    int renderFile(const char* path)
    {
        if (!path) return 1;

        //destination png file
        const char* dot = strrchr(path, '.');
        if (!dot) return 1;
        string dst(path, dot - path);
        dst += ".png";

        return renderer.render(path, width, height, dst, bgColor);
    }

    int handleDirectory(const string& path, DIR* dir)
    {
        //open directory
        if (!dir) {
            dir = opendir(path.c_str());
            if (!dir) {
                cout << "Couldn't open directory \"" << path.c_str() << "\"." << endl;
                return 1;
            }
        }

        //list directory
        int ret = 0;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (*entry->d_name == '.' || *entry->d_name == '$') continue;
            if (entry->d_type == DT_DIR) {
                string subpath = string(path);
#ifdef _WIN32
                subpath += '\\';
#else
                subpath += '/';
#endif
                subpath += entry->d_name;
                ret = handleDirectory(subpath, nullptr);
                if (ret) break;

            } else {
                if (!svgFile(entry->d_name)) continue;
                string fullpath = string(path);
#ifdef _WIN32
                fullpath += '\\';
#else
                fullpath += '/';
#endif
                fullpath += entry->d_name;
                ret = renderFile(fullpath.c_str());
                if (ret) break;
            }
        }
        closedir(dir);
        return ret;
    }
};

int main(int argc, char** argv)
{
    App app;
    return app.setup(argc, argv);
}
