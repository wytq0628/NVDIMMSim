/*********************************************************************************
*  Copyright (c) 2011-2012, Paul Tschirhart
*                             Peter Enns
*                             Jim Stevens
*                             Ishwar Bhati
*                             Mu-Tien Chang
*                             Bruce Jacob
*                             University of Maryland 
*                             pkt3c [at] umd [dot] edu
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/

//Plane.cpp
//class file for plane object
//

#include "Plane.h"

using namespace NVDSim;
using namespace std;

Plane::Plane(Configuration &nv_cfg) :
	cfg(nv_cfg)
{
	dataReg= NULL;
	cacheReg= NULL;
}

void Plane::read(ChannelPacket *busPacket){
	if (blocks.find(busPacket->block) != blocks.end()){
	    busPacket->data= blocks[busPacket->block].read(busPacket->page);
	} else{
	    ERROR("Invalid read: Block "<<busPacket->block<<" hasn't been written to. Address was "<<busPacket->virtualAddress);
	}

	if(dataReg == NULL)
	{
	    dataReg = busPacket;
	}
	else
	{
	    // no more room in this inn, somebody fucked up and it was probably me
	    ERROR("tried to add read data to plane "<<busPacket->plane<<" but both of its registers were full");
	    abort();
	}
}

void Plane::write(ChannelPacket *busPacket){
        // if this block has not been accessed yet, construct a new block and add it to the blocks map
	if (blocks.find(busPacket->block) == blocks.end())
		blocks[busPacket->block] = Block(busPacket->block);

	if(busPacket->busPacketType == FAST_WRITE)
	{
		blocks[busPacket->block].write(cfg.GARBAGE_COLLECT, busPacket->page, NULL);
	}
	// if we're interleaving
	else if(cfg.RW_INTERLEAVE_ENABLE)
	{
		// move the data from the cacheReg to the dataReg for input into the flash array
		// safety first...
		if(cacheReg != NULL)
		{
			dataReg = cacheReg;
			cacheReg = NULL;
		}
		else
		{
			ERROR("Invalid write: cacheReg was NULL when block "<<busPacket->block<<"was written to");
			abort();
		}
	}
	// if we're not interleaving, then we don't need to do anything because everything is already in the data reg
}

void Plane::writeDone(ChannelPacket *busPacket)
{
	blocks[busPacket->block].write(cfg.GARBAGE_COLLECT, busPacket->page, dataReg->data);
	    
    // The data packet is now done being used, so it can be deleted.
    dataReg = NULL;
}

// should only ever erase blocks
void Plane::erase(ChannelPacket *busPacket){
	if (blocks.find(busPacket->block) != blocks.end()){
		blocks[busPacket->block].erase();
		blocks.erase(busPacket->block);
		cout << "Erased a block \n";
	}
}


void Plane::storeInData(ChannelPacket *busPacket){
    if(cfg.RW_INTERLEAVE_ENABLE && cacheReg == NULL)
    {
	    cacheReg= busPacket;
    }
    else if(!cfg.RW_INTERLEAVE_ENABLE && dataReg == NULL)
    {
	    dataReg = busPacket;
    }
    else
    {
	    // no more room in this inn, somebody fucked up and it was probably me
	    ERROR("tried to add write data to plane "<<busPacket->plane<<" but both of its registers were full");
	    abort();
    }
}

ChannelPacket *Plane::readFromData(void){
    if(cfg.RW_INTERLEAVE_ENABLE && cacheReg == NULL && dataReg != NULL)
    {
	//cout << "read data from the cacheReg \n";
	cacheReg = dataReg;
	dataReg = NULL;
	return cacheReg;
    }
    else if (dataReg != NULL)
    {
	    return dataReg;
    }
    // should never get here
    abort();
}

bool Plane::checkCacheReg(void)
{
    if(cacheReg == NULL)
    {
	return true;
    }
    return false;
}

bool Plane::checkDataReg(void)
{
    if(dataReg == NULL)
    {
	return true;
    }
    return false;
}

void Plane::dataGone(void)
{
    // read no longer needs this register
	if(cfg.RW_INTERLEAVE_ENABLE)
		cacheReg = NULL;
	else
		dataReg = NULL;
}

void Plane::clearWrite(void)
{
    if(dataReg != NULL)
    {
	dataReg = NULL;
    }
    else
    {
	ERROR("tried to clear write data but there was no data in the data register");
	abort();
    }
}
