/********************************************************************************
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

#ifndef NVDIMM_UTIL_H
#define NVDIMM_UTIL_H

#include <string>
#include <iostream>
#include <sstream>
#include <list>

using namespace std;

typedef unsigned int uint;
   
// Utilites used by NVDIMM
uint64_t convert_uint64_t(string value);
void convert_uint64_t(uint64_t &var, string value, string infostring = "");
void convert_bool(bool &var, string value, string infostring = "");
void convert_float(float &var, string value, string infostring = "");

uint divide_params(uint num, uint denom);
uint divide_params(uint64_t num, uint denom);
uint divide_params(uint num, uint64_t denom);
uint divide_params(uint64_t num, uint64_t denom);
uint divide_params(float num, float denom);
uint64_t divide_params_64b(uint64_t num, uint denom);
uint64_t divide_params_64b(uint num, uint64_t denom);
uint64_t divide_params_64b(uint64_t num, uint64_t denom);
uint64_t divide_params_64b(uint64_t num, float denom);
uint64_t divide_params_64b(float num, float denom);

uint64_t subtract_params(uint64_t a, uint64_t b);

string strip(string input, string chars = " \t\f\v\n\r");
list<string> split(string input, string chars = " \t\f\v\n\r", size_t maxsplit=string::npos);

// Utilities borrowed from DRAMSim2
unsigned inline nvdimm_log2(unsigned value)
{
	unsigned logbase2 = 0;
	unsigned orig = value;
	value>>=1;
	while (value>0)
	{
		value >>= 1;
		logbase2++;
	}
	if (1U<<logbase2 < orig)
		logbase2++;
	return logbase2;
}

bool inline nvdimm_check_power2(unsigned value)
{
	unsigned logbase2 = 0;
	unsigned orig = value;

	value>>=1;
	while (value>0)
	{
		value >>= 1;
		logbase2++;
	}
	if(1U<<logbase2 != orig)
	{
		return false;
	}
	return true;
}

#endif
