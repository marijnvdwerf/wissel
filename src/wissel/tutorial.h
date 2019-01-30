#pragma once

namespace wissel::tutorial
{
    enum class tutorial_state
    {
        none,
        playing,
        recording,
    };

    tutorial_state state();

    void stop();
}
