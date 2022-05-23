/*
 * Copyright (c) 2021 - 2022 Samsung Electronics Co., Ltd. All rights reserved.

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

#include <thorvg.h>
#include "catch.hpp"

using namespace tvg;


TEST_CASE("Basic initialization", "[tvgInitializer]")
{
    REQUIRE(Initializer::init(CanvasEngine::Sw, 0) == Result::Success);
    REQUIRE(Initializer::term(CanvasEngine::Sw) == Result::Success);
}

TEST_CASE("Multiple initialization", "[tvgInitializer]")
{
    REQUIRE(Initializer::init(CanvasEngine::Sw, 0) == Result::Success);
    REQUIRE(Initializer::init(CanvasEngine::Sw, 0) == Result::Success);
    REQUIRE(Initializer::term(CanvasEngine::Sw) == Result::Success);

    REQUIRE(Initializer::init(CanvasEngine::Sw, 0) == Result::Success);
    REQUIRE(Initializer::term(CanvasEngine::Sw) == Result::Success);
    REQUIRE(Initializer::term(CanvasEngine::Sw) == Result::Success);
}

TEST_CASE("Negative termination", "[tvgInitializer]")
{
    REQUIRE(Initializer::term(CanvasEngine::Sw) == Result::InsufficientCondition);
}

TEST_CASE("Invalid engine", "[tvgInitializer]")
{
    REQUIRE(Initializer::init(CanvasEngine(0), 0) == Result::InvalidArguments);
}
