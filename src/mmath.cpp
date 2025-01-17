/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mmath.h>

#include "globals.h"

int world2zoomedWorld(int x) {
    const auto zoom = dune::globals::currentZoomlevel;

    if (x < 0) {
        switch (zoom) {
            case 0: return (x - 3) / 4;
            case 1: return (x - 1) / 2;
            case 2: return ((x - 1) * 3) / 4;
            case 3:
            default: return x;
        }
    }

    switch (zoom) {
        case 0: return x / 4;
        case 1: return x / 2;
        case 2: return (x * 3) / 4;
        case 3:
        default: return x;
    }
}

int world2zoomedWorld(float x) {
    switch (dune::globals::currentZoomlevel) {
        case 0: return lround(x * 0.25f);
        case 1: return lround(x * 0.5f);
        case 2: return lround(x * 0.75f);
        case 3:
        default: return lround(x);
    }
}

Coord world2zoomedWorld(const Coord& coord) {
    return {world2zoomedWorld(coord.x), world2zoomedWorld(coord.y)};
}

int zoomedWorld2world(int x) {
    switch (dune::globals::currentZoomlevel) {
        case 0: return x * 4;
        case 1: return x * 2;
        case 2: return (x * 4) / 3;
        case 3:
        default: return x;
    }
}

Coord zoomedWorld2world(const Coord& coord) {
    return {zoomedWorld2world(coord.x), zoomedWorld2world(coord.y)};
}
