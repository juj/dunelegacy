/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2010 Simon Peter, <dn.tlp@gmx.net>, et al.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * surroundopl.cpp - Wrapper class to provide a surround/harmonic effect
 *   for another OPL emulator, by Adam Nielsen <malvineous@shikadi.net>
 *
 * Stereo harmonic algorithm by Adam Nielsen <malvineous@shikadi.net>
 * Please give credit if you use this algorithm elsewhere :-)
 */

#include <FileClasses/adl/surroundopl.h>
#include <cmath>   // for pow()
#include <cstring> // for memset()
//#include "debug.h"

CSurroundopl::CSurroundopl(std::unique_ptr<Copl> a, std::unique_ptr<Copl> b, bool use16bit)
    : use16bit(use16bit),
      a(std::move(a)), b(std::move(b)) {
    currType = TYPE_OPL2;

    lbuf.resize(default_bufsize);
    rbuf.resize(default_bufsize);

    memset(iFMReg, 0, sizeof(iFMReg));
    memset(iTweakedFMReg, 0, sizeof(iTweakedFMReg));
    memset(iCurrentTweakedBlock, 0, sizeof(iCurrentTweakedBlock));
    memset(iCurrentFNum, 0, sizeof(iCurrentFNum));
}

CSurroundopl::~CSurroundopl() = default;

void CSurroundopl::update(short* buf, int samples) {
    if (samples * 2 > rbuf.size()) {
        // Need to resize the buffer
        rbuf.resize(samples * 2);
        lbuf.resize(rbuf.size());
    }

    a->update(&lbuf[0], samples);
    b->update(&rbuf[0], samples);

    // Copy the two mono OPL buffers into the stereo buffer
    for (int i = 0; i < samples; i++) {
        if (this->use16bit) {
            buf[i * 2]     = this->lbuf[i];
            buf[i * 2 + 1] = this->rbuf[i];
        } else {
            auto* char_buf = reinterpret_cast<char*>(buf);

            char_buf[i * 2]     = reinterpret_cast<char*>(&lbuf[0])[i];
            char_buf[i * 2 + 1] = reinterpret_cast<char*>(&rbuf[0])[i];
        }
    }
}

void CSurroundopl::write(int reg, int val) {
    a->write(reg, val);

    // Transpose the other channel to produce the harmonic effect
    int iChannel  = -1;
    int iRegister = reg; // temp
    int iValue    = val; // temp
    if ((iRegister >> 4 == 0xA) || (iRegister >> 4 == 0xB))
        iChannel = iRegister & 0x0F;

    // Remember the FM state, so that the harmonic effect can access
    // previously assigned register values.
    this->iFMReg[this->currChip][iRegister] = iValue;

    if ((iChannel >= 0)) { // && (i == 1)) {
        const uint8_t iBlock = (this->iFMReg[this->currChip][0xB0 + iChannel] >> 2) & 0x07;
        const uint16_t iFNum = ((this->iFMReg[this->currChip][0xB0 + iChannel] & 0x03) << 8) | this->iFMReg[this->currChip][0xA0 + iChannel];
        // double dbOriginalFreq = 50000.0 * (double)iFNum * pow(2, iBlock - 20);
        const double dbOriginalFreq = 49716.0 * static_cast<double>(iFNum) * pow(2, iBlock - 20);

        uint8_t iNewBlock = iBlock;
        uint16_t iNewFNum = 0;

// Adjust the frequency and calculate the new FNum
// double dbNewFNum = (dbOriginalFreq+(dbOriginalFreq/FREQ_OFFSET)) / (50000.0 * pow(2, iNewBlock - 20));
//#define calcFNum() ((dbOriginalFreq+(dbOriginalFreq/FREQ_OFFSET)) / (50000.0 * pow(2, iNewBlock - 20)))
#define calcFNum() ((dbOriginalFreq + (dbOriginalFreq / FREQ_OFFSET)) / (49716.0 * pow(2, iNewBlock - 20)))
        const double dbNewFNum = calcFNum();

        // Make sure it's in range for the OPL chip
        if (dbNewFNum > 1023 - NEWBLOCK_LIMIT) {
            // It's too high, so move up one block (octave) and recalculate

            if (iNewBlock > 6) {
                // Uh oh, we're already at the highest octave!
                //              AdPlug_LogWrite("OPL WARN: FNum %d/B#%d would need block 8+ after being transposed (new FNum is %d)\n",
                //                  iFNum, iBlock, (int)dbNewFNum);
                // The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
                // sound *too* bad (hopefully it will just miss out on the nice harmonic.)
                iNewBlock = iBlock;
                iNewFNum  = iFNum;
            } else {
                iNewBlock++;
                iNewFNum = static_cast<uint16_t>(calcFNum());
            }
        } else if (dbNewFNum < 0 + NEWBLOCK_LIMIT) {
            // It's too low, so move down one block (octave) and recalculate

            if (iNewBlock == 0) {
                // Uh oh, we're already at the lowest octave!
                //              AdPlug_LogWrite("OPL WARN: FNum %d/B#%d would need block -1 after being transposed (new FNum is %d)!\n",
                //                  iFNum, iBlock, (int)dbNewFNum);
                // The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
                // sound *too* bad (hopefully it will just miss out on the nice harmonic.)
                iNewBlock = iBlock;
                iNewFNum  = iFNum;
            } else {
                iNewBlock--;
                iNewFNum = static_cast<uint16_t>(calcFNum());
            }
        } else {
            // Original calculation is within range, use that
            iNewFNum = static_cast<uint16_t>(dbNewFNum);
        }

        // Sanity check
        if (iNewFNum > 1023) {
            // Uh oh, the new FNum is still out of range! (This shouldn't happen)
            //          AdPlug_LogWrite("OPL ERR: Original note (FNum %d/B#%d is still out of range after change to FNum %d/B#%d!\n",
            //              iFNum, iBlock, iNewFNum, iNewBlock);
            // The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
            // sound *too* bad (hopefully it will just miss out on the nice harmonic.)
            iNewBlock = iBlock;
            iNewFNum  = iFNum;
        }

        if ((iRegister >= 0xB0) && (iRegister <= 0xB8)) {

            // Overwrite the supplied value with the new F-Number and Block.
            iValue = (iValue & ~0x1F) | (iNewBlock << 2) | ((iNewFNum >> 8) & 0x03);

            this->iCurrentTweakedBlock[this->currChip][iChannel] = iNewBlock; // save it so we don't have to update register 0xB0 later on
            this->iCurrentFNum[this->currChip][iChannel]         = static_cast<uint8_t>(iNewFNum);

            if (this->iTweakedFMReg[this->currChip][0xA0 + iChannel] != (iNewFNum & 0xFF)) {
                // Need to write out low bits
                const uint8_t iAdditionalReg   = 0xA0 + iChannel;
                const uint8_t iAdditionalValue = iNewFNum & 0xFF;
                b->write(iAdditionalReg, iAdditionalValue);
                this->iTweakedFMReg[this->currChip][iAdditionalReg] = iAdditionalValue;
            }
        } else if ((iRegister >= 0xA0) && (iRegister <= 0xA8)) {

            // Overwrite the supplied value with the new F-Number.
            iValue = iNewFNum & 0xFF;

            // See if we need to update the block number, which is stored in a different register
            uint8_t iNewB0Value = (this->iFMReg[this->currChip][0xB0 + iChannel] & ~0x1F) | (iNewBlock << 2) | ((iNewFNum >> 8) & 0x03);
            if (
                (iNewB0Value & 0x20) &&                                               // but only update if there's a note currently playing (otherwise we can just wait
                (this->iTweakedFMReg[this->currChip][0xB0 + iChannel] != iNewB0Value) // until the next noteon and update it then)
            ) {
                //              AdPlug_LogWrite("OPL INFO: CH%d - FNum %d/B#%d -> FNum %d/B#%d == keyon register update!\n",
                //                  iChannel, iFNum, iBlock, iNewFNum, iNewBlock);
                // The note is already playing, so we need to adjust the upper bits too
                const uint8_t iAdditionalReg = 0xB0 + iChannel;
                b->write(iAdditionalReg, iNewB0Value);
                this->iTweakedFMReg[this->currChip][iAdditionalReg] = iNewB0Value;
            } // else the note is not playing, the upper bits will be set when the note is next played

        } // if (register 0xB0 or 0xA0)

    } // if (a register we're interested in)

    // Now write to the original register with a possibly modified value
    b->write(iRegister, iValue);
    this->iTweakedFMReg[this->currChip][iRegister] = iValue;
}

void CSurroundopl::init() {
    a->init();
    b->init();
    for (int c = 0; c < 2; c++) {
        for (int i = 0; i < 256; i++) {
            this->iFMReg[c][i]        = 0;
            this->iTweakedFMReg[c][i] = 0;
        }
        for (int i = 0; i < 9; i++) {
            this->iCurrentTweakedBlock[c][i] = 0;
            this->iCurrentFNum[c][i]         = 0;
        }
    }
}

void CSurroundopl::setchip(int n) noexcept {
    a->setchip(n);
    b->setchip(n);
}
