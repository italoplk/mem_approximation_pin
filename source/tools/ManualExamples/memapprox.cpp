/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2016 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for injecting memory faults.
 */

#include <stdio.h>
#include "pin.H"
#include <set>
#include <iostream>
#include <algorithm>

///////////////////////////////////////////////////////
#if 0
  #define LOGGING(arg) printf arg
#else
  #define LOGGING(arg) (VOID)0
#endif
///////////////////////////////////////////////////////
bool enable_read_error_inection = true;
bool enable_write_error_inection = true;

double read_ber = 0;
double write_ber = 0;
///////////////////////////////////////////////////////
FILE * trace;

typedef std::pair<int, int> Range;
struct RangeCompare
{
    //overlapping ranges are considered equivalent
    bool operator()(const Range& lhv, const Range& rhv) const
    {   
        return lhv.second < rhv.first;
    } 
};

bool in_range(const std::set<Range, RangeCompare>& ranges, int value)
{
    return ranges.find(Range(value, value)) != ranges.end();
}

std::set<Range, RangeCompare> ranges;

std::map<unsigned long long, std::pair<unsigned char*,int> > reads;
std::set<unsigned long long> pending_writes;
///////////////////////////////////////////////////////

VOID printHex(unsigned long long addr, int size)
{
    for (int byte = size - 1; byte >=0 ; byte--)
        LOGGING(("%x ", ((unsigned char *)addr)[byte]));
}

VOID InvalidatePendingReads(unsigned long long addr)
{
    map<unsigned long long, std::pair<unsigned char*,int> >::iterator it;
    it = reads.find(addr);
    if (it != reads.end()) {
        reads.erase(it);
    }
}

/* ===================================================================== */
/* Fault Injection Handling                                              */
/* ===================================================================== */

bool InjectFault(uint8_t *data, int len, double ber)
{
    // std::mt19937_64 rng;
    // //std::default_random_engine generator;
    // // initialize the random number generator with time-dependent seed
    // uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    // std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
    // rng.seed(ss);
    // std::uniform_real_distribution<double> distribution(0.0,1.0);

    bool fault_injected = false;
    for (int byte = 0; byte < len; byte++) {
        unsigned char bit_mask = 0x80;
        for (int bit = 0; bit < 8; bit++) {
            double random_probability = ((double) rand() / (RAND_MAX));
            if(random_probability < ber) {
                uint8_t fault_mask = bit_mask >> bit;
                if (fault_mask != 0) {
                    data[byte] = data[byte] ^ fault_mask;
                    fault_injected = true;
                }
            }
        }
    }
    return fault_injected;
}

VOID RecordFaultyRead(unsigned long long addr, int size)
{
    // record current value and inject fault
    unsigned char *temp_date = (unsigned char *)malloc(size);
    memcpy(temp_date, (unsigned char *)addr, size);
    LOGGING(("R-apply %llu ", addr));
    printHex(addr, size);
    if (InjectFault((uint8_t*) addr, size, read_ber)) {
        LOGGING((" => "));
        printHex(addr, size);
        reads[addr] = std::make_pair(temp_date, size);
        // reads.insert(std::pair<unsigned long long,unsigned char*>(addr, temp_date));
    }
    else {
        free(temp_date);
    }
    LOGGING(("\n"));
}

VOID ReverseFaultyRead(unsigned long long addr)
{
    std::map<unsigned long long, std::pair<unsigned char*,int> >::iterator it;
    // if faulty retrieve the accurate value
    it = reads.find(addr);
    int size = (it->second).second;
    unsigned char * data_ptr = (it->second).first;
    if (it != reads.end()) {
        LOGGING(("R-reverse %llu ", addr));
        printHex(addr, size);
        memcpy((unsigned char *)addr, data_ptr, size);
        LOGGING((" => "));
        printHex(addr, size);
        LOGGING(("\n"));
        free(data_ptr);
        reads.erase(it);
    }
}

VOID RecordFaultyWrite(unsigned long long addr)
{
    // records the address to inject fault later during the first read
    pending_writes.insert(addr);
    LOGGING(("W-record %llu\n", addr));
}

VOID ApplyFaultyWrite(unsigned long long addr, int size)
{
    // if a faulty write is pending, it applies it during the first read
    std::set<unsigned long long>::iterator it;
    it = pending_writes.find(addr);
    if (it != pending_writes.end()) {
        pending_writes.erase(it);
        LOGGING(("W-apply %llu ", addr));
        printHex(addr, size);
        InjectFault((uint8_t*) addr, size, write_ber);
        LOGGING((" => "));
        printHex(addr, size);
        LOGGING(("\n"));
    }
}

/* ===================================================================== */
/* Investigate each memory read and write                                */
/* ===================================================================== */

// memory read
VOID HandleMemRead(VOID * ip, VOID * addr, UINT32 size)
{
    bool is_approx = in_range(ranges, (unsigned long long)addr);
    // fprintf(trace,"%p: R %p %d %s\n", ip, addr, size, is_approx ? "approx" : "exact");

    if (is_approx) {
        if (enable_read_error_inection && read_ber)
            ReverseFaultyRead((unsigned long long)addr);
        if (enable_write_error_inection && write_ber)
            ApplyFaultyWrite((unsigned long long)addr, size);
        if (enable_read_error_inection && read_ber)
            RecordFaultyRead((unsigned long long)addr, size);
    }
}

// memory write
VOID HandleMemWrite(VOID * ip, VOID * addr, UINT32 size)
{
    bool is_approx = in_range(ranges, (unsigned long long)addr);
    // fprintf(trace,"%p: W %p %d %s\n", ip, addr, size, is_approx ? "approx" : "exact");

    if (is_approx) {
        InvalidatePendingReads((unsigned long long)addr);
        if (enable_write_error_inection && write_ber)
            RecordFaultyWrite((unsigned long long)addr);
    }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)HandleMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp, IARG_MEMORYREAD_SIZE,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)HandleMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp, IARG_MEMORYWRITE_SIZE,
                IARG_END);
        }
    }
}

/* ===================================================================== */
/* Approx memory region declare/undeclare                                */
/* ===================================================================== */

// This function is called before every instruction is executed
VOID add_approx(unsigned long long start_address, unsigned long long end_address)
{
    ranges.insert(Range(start_address, end_address));
    fprintf(trace, "%s %llu %llu\n", "add_approx", start_address, end_address);
}

VOID remove_approx(unsigned long long start_address, unsigned long long end_address)
{
    // reverse all faults injected by reads
    for(map<unsigned long long, std::pair<unsigned char*,int> >::iterator it = reads.begin(); it != reads.end(); ++it) {
        if ((it->first) >= start_address && (it->first) <= end_address) {
            ReverseFaultyRead((it->first));
        }
    }

    ranges.erase(Range(start_address, end_address));
    fprintf(trace, "%s %llu %llu\n", "remove_approx", start_address, end_address);
}

/* ===================================================================== */
/* Ber values settings                                                   */
/* ===================================================================== */

VOID set_read_ber(double _read_ber) 
{
    read_ber = _read_ber;
    fprintf(trace, "%s %f\n", "set_read_ber", read_ber);
}

VOID set_write_ber(double _write_ber) 
{
    write_ber = _write_ber;
    fprintf(trace, "%s %f\n", "set_write_ber", write_ber);
}

VOID get_read_ber(double *_read_ber) 
{
    *_read_ber = read_ber;
    fprintf(trace, "%s %f\n", "get_read_ber", *_read_ber);
}

VOID get_write_ber(double *_write_ber) 
{
    *_write_ber = write_ber;
    fprintf(trace, "%s %f\n", "get_write_ber", *_write_ber);
}

/* ===================================================================== */
/* Register functions to track                                           */
/* ===================================================================== */

VOID Routine(RTN rtn, VOID *v)
{
    string rtnName = RTN_Name(rtn);

    // Insert a call at the entry point of routines
    if (rtnName.find("add_approx") != string::npos) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)add_approx, 
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, 
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_END);
        RTN_Close(rtn);
    }

    if (rtnName.find("remove_approx") != string::npos) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)remove_approx,  
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, 
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_END);
        RTN_Close(rtn);
    }

    if (rtnName.find("set_read_ber") != string::npos) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)set_read_ber, 
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(rtn);
    }

    if (rtnName.find("set_write_ber") != string::npos) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)set_write_ber,  
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(rtn);
    }

    if (rtnName.find("get_read_ber") != string::npos) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)get_read_ber, 
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(rtn);
    }

    if (rtnName.find("get_write_ber") != string::npos) {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)get_write_ber,  
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(rtn);
    }

    // printf("%s\n", RTN_Name(rtn).c_str());
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool inejcts memory faults at addresses registered by calling add_approx()\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<double>   ReadBER(KNOB_MODE_WRITEONCE,    "pintool",
    "rd_ber", "0", "write bit error rate");
KNOB<double>   WriteBER(KNOB_MODE_WRITEONCE,    "pintool",
    "wr_ber", "0", "read bit error rate");


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    srand((unsigned)time(0));   

    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();

    read_ber = ReadBER.Value();
    write_ber = WriteBER.Value();

    trace = fopen("memapprox.log", "w");

    // Register Routine to be called to instrument rtn
    RTN_AddInstrumentFunction(Routine, 0);

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
