#pragma once

struct Point
{
    Point() {}
    Point(int in_x) : x(in_x), y(in_x) {}
    Point(int in_x, int in_y) : x(in_x), y(in_y) {}

    int x = 0;
    int y = 0;
};