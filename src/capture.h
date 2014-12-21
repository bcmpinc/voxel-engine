/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013,2014  B.J. Conijn <bcmpinc@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef CAPTURE_H
#define CAPTURE_H
#include <stdint.h>

class Capture {
    struct CaptureData * data;
public:
    Capture() : data(nullptr) {}
    Capture(const char * filename, uint8_t * data, int width, int height);
    Capture(Capture&& other) : data(other.data) { other.data=nullptr; }
    void operator=(Capture&& other) { end(); data=other.data; other.data=nullptr; }
    ~Capture();
    void shoot();
    void end();
};

#endif // CAPTURE_H
