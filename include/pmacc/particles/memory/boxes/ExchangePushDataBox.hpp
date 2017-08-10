/* Copyright 2013-2017 Heiko Burau, Rene Widera, Benjamin Worpitz
 *
 * This file is part of PMacc.
 *
 * PMacc is free software: you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with PMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pmacc/particles/memory/dataTypes/ExchangeMemoryIndex.hpp"
#include "pmacc/memory/boxes/DataBox.hpp"
#include "pmacc/particles/memory/boxes/TileDataBox.hpp"
#include "pmacc/particles/memory/boxes/PushDataBox.hpp"

#include "pmacc/memory/boxes/DataBox.hpp"
#include "pmacc/memory/boxes/PitchedBox.hpp"

namespace pmacc
{


/**
 * @tparam TYPE type for addresses
 * @tparam VALUE type for actual data
 * @tparam DIM dimension
 */
template<class TYPE, class VALUE, unsigned DIM>
class ExchangePushDataBox : public DataBox<PitchedBox<VALUE, DIM1> >
{
public:

    typedef ExchangeMemoryIndex<TYPE, DIM> PushType;

    HDINLINE ExchangePushDataBox(VALUE *data, TYPE *currentSizePointer, TYPE maxSize,
                                PushDataBox<TYPE, PushType > virtualMemory) :
    DataBox<PitchedBox<VALUE, DIM1> >(PitchedBox<VALUE, DIM1>(data, DataSpace<DIM1>())),
    currentSizePointer(currentSizePointer),
    maxSize(maxSize),
    virtualMemory(virtualMemory)
    {
    }

    template< typename T_Acc >
    HDINLINE TileDataBox<VALUE> pushN(
        T_Acc const & acc,
        TYPE count,
        const DataSpace<DIM> &superCell
    )
    {
        TYPE oldSize = atomicAdd(currentSizePointer, count, ::alpaka::hierarchy::Grids{}); //get count VALUEs

        if (oldSize + count > maxSize)
        {
            atomicExch(currentSizePointer, maxSize); //reset size to maxsize
            if (oldSize >= maxSize)
            {
                return TileDataBox<VALUE > (nullptr,
                                            DataSpace<DIM1 > (0),
                                            0);
            }
            else
                count = maxSize - oldSize;
        }

        TileDataBox<PushType> tmp = virtualMemory.pushN(acc, 1);
        tmp[0].setSuperCell(superCell);
        tmp[0].setCount(count);
        tmp[0].setStartIndex(oldSize);
        return TileDataBox<VALUE > (this->fixedPointer,
                                    DataSpace<DIM1 > (oldSize),
                                    count);
    }



protected:
    PMACC_ALIGN8(virtualMemory, PushDataBox<TYPE, PushType >);
    PMACC_ALIGN(maxSize, TYPE);
    PMACC_ALIGN(currentSizePointer, TYPE*);
};

}
