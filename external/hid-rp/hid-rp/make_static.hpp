#pragma once

template <auto Data>
consteval const auto& make_static()
{
    // source: Hana Dusíková and https://youtu.be/ABg4_EV5L3w?si=eSD-TwF0MEwsopt3&t=1117
    return Data;
}
