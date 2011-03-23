#include "PCMLogger.h"

using namespace NVDSim;
using namespace std;

PCMLogger::PCMLogger()
  : Logger()
{
	vpp_idle_energy = vector<double>(NUM_PACKAGES, 0.0); 
	vpp_access_energy = vector<double>(NUM_PACKAGES, 0.0); 
}

void PCMLogger::update()
{
    	//update idle energy
	//since this is already subtracted from the access energies we just do it every time
	for(uint i = 0; i < (NUM_PACKAGES); i++)
	{
	  idle_energy[i] += STANDBY_I;
	  vpp_idle_energy[i] += VPP_STANDBY_I;
	}

	this->step();
}

// Using virtual addresses here right now
void PCMLogger::access_process(uint64_t addr, uint package, ChannelPacketType op)
{
        // Get entry off of the access_queue.
	uint64_t start_cycle = 0;
	bool found = false;
	list<pair <uint64_t, uint64_t>>::iterator it;
	for (it = access_queue.begin(); it != access_queue.end(); it++)
	{
		uint64_t cur_addr = (*it).first;
		uint64_t cur_cycle = (*it).second;

		if (cur_addr == addr)
		{
			start_cycle = cur_cycle;
			found = true;
			access_queue.erase(it);
			break;
		}
	}

	if (!found)
	{
		cerr << "ERROR: NVLogger.access_process() called with address not in the access_queue. address=0x" << hex << addr << "\n" << dec;
		abort();
	}

	if (access_map.count(addr) != 0)
	{
		cerr << "ERROR: NVLogger.access_process() called with address already in access_map. address=0x" << hex << addr << "\n" << dec;
		abort();
	}

	AccessMapEntry a;
	a.start = start_cycle;
	a.op = op;
	a.process = this->currentClockCycle;
	a.package = package;
	access_map[addr] = a;

	this->queue_latency(a.process - a.start);
}

void PCMLogger::access_stop(uint64_t addr, uint64_t paddr)
{
	if (access_map.count(addr) == 0)
	{
		cerr << "ERROR: NVLogger.access_stop() called with address not in access_map. address=" << hex << addr << "\n" << dec;
		abort();
	}

	AccessMapEntry a = access_map[addr];
	a.stop = this->currentClockCycle;
	access_map[addr] = a;

	// Log cache event type.
	if (a.op == READ)
	{
	    //update access energy figures
	    access_energy[a.package] += (READ_I - STANDBY_I) * READ_TIME/2;
	    //update access energy figure with PCM stuff (if applicable)
	    vpp_access_energy[a.package] += (VPP_READ_I - VPP_STANDBY_I) * READ_TIME/2;
	    this->read();
	    this->read_latency(a.stop - a.start);
	}
	else if (a.op == WRITE)
	{
	    //update access energy figures
	    //without garbage collection PCM write and erase are the same
	    //this is due to the time it takes to set a bit
	    access_energy[a.package] += (ERASE_I - STANDBY_I) * ERASE_TIME/2;
	    //update access energy figure with PCM stuff (if applicable)
	    vpp_access_energy[a.package] += (VPP_ERASE_I - VPP_STANDBY_I) * ERASE_TIME/2;
	    this->write();
	    this->write_latency(a.stop - a.start);
	    if(writes_per_address.count(paddr) == 0)
	    {
		writes_per_address[paddr] = 1;
	    }
	    else
	    {
		writes_per_address[paddr]++;
	    }
	}
		
	access_map.erase(addr);
}

void PCMLogger::save(uint64_t cycle, uint epoch) 
{
        // Power stuff
	// Total power used
	vector<double> total_energy = vector<double>(NUM_PACKAGES, 0.0);
	
        // Average power used
	vector<double> ave_idle_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> ave_access_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> ave_vpp_idle_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> ave_vpp_access_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> average_power = vector<double>(NUM_PACKAGES, 0.0);

	for(uint i = 0; i < NUM_PACKAGES; i++)
	{
	     if(cycle != 0)
	     {
		 total_energy[i] = ((idle_energy[i] + access_energy[i]) * VCC)
	                           + ((vpp_idle_energy[i] + vpp_access_energy[i]) * VPP);
		 ave_idle_power[i] = (idle_energy[i] * VCC) / cycle;
		 ave_access_power[i] = (access_energy[i] * VCC) / cycle;
		 ave_vpp_idle_power[i] = (vpp_idle_energy[i] * VPP) / cycle;
		 ave_vpp_access_power[i] = (vpp_access_energy[i] * VPP) / cycle;
		 average_power[i] = total_energy[i] / cycle;
	     }
	     else
	     {
		 total_energy[i] = 0;
		 ave_idle_power[i] = 0;
		 ave_access_power[i] = 0;
		 ave_vpp_idle_power[i] = 0;
		 ave_vpp_access_power[i] = 0;
		 average_power[i] = 0;
	     }
	}

        
	savefile.open("NVDIMM.log", ios_base::out | ios_base::trunc);

	if (!savefile) 
	{
	    ERROR("Cannot open PowerStats.log");
	    exit(-1); 
	}

	savefile<<"NVDIMM Log \n";
	savefile<<"\nData for Full Simulation: \n";
	savefile<<"========================\n";
	savefile<<"\nAccess Data: \n";
	savefile<<"========================\n";	
	savefile<<"Cycles Simulated: "<<cycle<<"\n";
	savefile<<"Accesses: "<<num_accesses<<"\n";
	savefile<<"Reads completed: "<<num_reads<<"\n";
	savefile<<"Writes completed: "<<num_writes<<"\n";
	savefile<<"Number of Unmapped Accesses: " <<num_unmapped<<"\n";
	savefile<<"Number of Mapped Accesses: " <<num_mapped<<"\n";
	savefile<<"Number of Unmapped Reads: " <<num_read_unmapped<<"\n";
	savefile<<"Number of Mapped Reads: " <<num_read_mapped<<"\n";
	savefile<<"Number of Unmapped Writes: " <<num_write_unmapped<<"\n";
	savefile<<"Number of Mapped Writes: " <<num_write_mapped<<"\n";
	savefile<<"Unmapped Rate: " <<unmapped_rate()<<"\n";
	savefile<<"Read Unmapped Rate: " <<read_unmapped_rate()<<"\n";
	savefile<<"Write Unmapped Rate: " <<write_unmapped_rate()<<"\n";

	savefile<<"\nThroughput and Latency Data: \n";
	savefile<<"========================\n";
	savefile<<"Average Read Latency: " <<(divide((float)average_read_latency,(float)num_reads))<<" cycles";
	savefile<<" (" <<(divide((float)average_read_latency,(float)num_reads)*CYCLE_TIME)<<" ns)\n";
	savefile<<"Average Write Latency: " <<divide((float)average_write_latency,(float)num_writes)<<" cycles";
	savefile<<" (" <<(divide((float)average_write_latency,(float)num_writes))*CYCLE_TIME<<" ns)\n";
	savefile<<"Average Queue Latency: " <<divide((float)average_queue_latency,(float)num_accesses)<<" cycles";
	savefile<<" (" <<(divide((float)average_queue_latency,(float)num_accesses))*CYCLE_TIME<<" ns)\n";
	savefile<<"Total Throughput: " <<this->calc_throughput(cycle, num_accesses)<<" KB/sec\n";
	savefile<<"Read Throughput: " <<this->calc_throughput(cycle, num_reads)<<" KB/sec\n";
	savefile<<"Write Throughput: " <<this->calc_throughput(cycle, num_writes)<<" KB/sec\n";

	savefile<<"\nQueue Length Data: \n";
	savefile<<"========================\n";
	savefile<<"Length of Ftl Queue: " <<ftl_queue_length<<"\n";
	for(uint i = 0; i < ctrl_queue_length.size(); i++)
	{
	    savefile<<"Length of Controller Queue for Package " << i << " : "<<ctrl_queue_length[i]<<"\n";
	}
	
	savefile<<"\nWrite Frequency Data\n";
	savefile<<"========================\n";
	unordered_map<uint64_t, uint64_t>::iterator it;
	for (it = writes_per_address.begin(); it != writes_per_address.end(); it++)
	{
	    savefile<<"Address "<<(*it).first<<" had "<<(*it).second<<" writes\n";
	}

	savefile<<"\nPower Data: \n";
	savefile<<"========================\n";

	for(uint i = 0; i < NUM_PACKAGES; i++)
	{
	    savefile<<"Package: "<<i<<"\n";
	    savefile<<"Accumulated Idle Energy: "<<(idle_energy[i] * VCC * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    savefile<<"Accumulated Access Energy: "<<(access_energy[i] * VCC * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    savefile<<"Accumulated VPP Idle Energy: "<<(vpp_idle_energy[i] * VPP * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    savefile<<"Accumulated VPP Access Energy: "<<(vpp_access_energy[i] * VPP * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    savefile<<"Total Energy: "<<(total_energy[i] * (CYCLE_TIME * 0.000000001))<<"mJ\n\n";
	 
	    savefile<<"Average Idle Power: "<<ave_idle_power[i]<<"mW\n";
	    savefile<<"Average Access Power: "<<ave_access_power[i]<<"mW\n";
	    savefile<<"Average VPP Idle Power: "<<ave_vpp_idle_power[i]<<"mW\n";
	    savefile<<"Average VPP Access Power: "<<ave_vpp_access_power[i]<<"mW\n";
	    savefile<<"Average Power: "<<average_power[i]<<"mW\n\n";
	}

	if(USE_EPOCHS)
	{
	    list<EpochEntry>::iterator it;
	    for (it = epoch_queue.begin(); it != epoch_queue.end(); it++)
	    {
		for(uint i = 0; i < NUM_PACKAGES; i++)
		{
		    if((*it).cycle != 0)
		    {
			total_energy[i] = (((*it).idle_energy[i] + (*it).access_energy[i]) * VCC)
			                  + (((*it).vpp_idle_energy[i] + (*it).vpp_access_energy[i]) * VPP);
			ave_idle_power[i] = ((*it).idle_energy[i] * VCC) / (*it).cycle;
			ave_access_power[i] = ((*it).access_energy[i] * VCC) / (*it).cycle;
			ave_vpp_idle_power[i] = ((*it).vpp_idle_energy[i] * VPP) / (*it).cycle;
			ave_vpp_access_power[i] = ((*it).vpp_access_energy[i] * VPP) / (*it).cycle;
			average_power[i] = total_energy[i] / (*it).cycle;
		    }
		    else
		    {
			total_energy[i] = 0;
			ave_idle_power[i] = 0;
			ave_access_power[i] = 0;
			ave_vpp_idle_power[i] = 0;
			ave_vpp_access_power[i] = 0;
			average_power[i] = 0;
		    }
		}

		savefile<<"\nData for Epoch "<<(*it).epoch<<"\n";
		savefile<<"========================\n";;
		savefile<<"\nAccess Data: \n";
		savefile<<"========================\n";	
		savefile<<"Cycles Simulated: "<<(*it).cycle<<"\n";
		savefile<<"Accesses: "<<(*it).num_accesses<<"\n";
		savefile<<"Reads completed: "<<(*it).num_reads<<"\n";
		savefile<<"Writes completed: "<<(*it).num_writes<<"\n";
		savefile<<"Number of Unmapped Accesses: " <<(*it).num_unmapped<<"\n";
		savefile<<"Number of Mapped Accesses: " <<(*it).num_mapped<<"\n";
		savefile<<"Number of Unmapped Reads: " <<(*it).num_read_unmapped<<"\n";
		savefile<<"Number of Mapped Reads: " <<(*it).num_read_mapped<<"\n";
		savefile<<"Number of Unmapped Writes: " <<(*it).num_write_unmapped<<"\n";
		savefile<<"Number of Mapped Writes: " <<(*it).num_write_mapped<<"\n";
		
		savefile<<"\nThroughput and Latency Data: \n";
		savefile<<"========================\n";
		savefile<<"Average Read Latency: " <<(divide((float)(*it).average_read_latency,(float)(*it).num_reads))<<" cycles";
		savefile<<" (" <<(divide((float)(*it).average_read_latency,(float)(*it).num_reads)*CYCLE_TIME)<<" ns)\n";
		savefile<<"Average Write Latency: " <<divide((float)(*it).average_write_latency,(float)(*it).num_writes)<<" cycles";
		savefile<<" (" <<(divide((float)(*it).average_write_latency,(float)(*it).num_writes))*CYCLE_TIME<<" ns)\n";
		savefile<<"Average Queue Latency: " <<divide((float)(*it).average_queue_latency,(float)(*it).num_accesses)<<" cycles";
		savefile<<" (" <<(divide((float)(*it).average_queue_latency,(float)(*it).num_accesses))*CYCLE_TIME<<" ns)\n";
		savefile<<"Total Throughput: " <<this->calc_throughput((*it).cycle, (*it).num_accesses)<<" KB/sec\n";
		savefile<<"Read Throughput: " <<this->calc_throughput((*it).cycle, (*it).num_reads)<<" KB/sec\n";
		savefile<<"Write Throughput: " <<this->calc_throughput((*it).cycle, (*it).num_writes)<<" KB/sec\n";
		
		savefile<<"\nQueue Length Data: \n";
		savefile<<"========================\n";
		savefile<<"Length of Ftl Queue: " <<(*it).ftl_queue_length<<"\n";
		for(uint i = 0; i < (*it).ctrl_queue_length.size(); i++)
		{
		    savefile<<"Length of Controller Queue for Package " << i << " : "<<(*it).ctrl_queue_length[i]<<"\n";
		}

		savefile<<"\nWrite Frequency Data\n";
		savefile<<"========================\n";
		unordered_map<uint64_t, uint64_t>::iterator it2;
		for (it2 = (*it).writes_per_address.begin(); it2 != (*it).writes_per_address.end(); it2++)
		{
		    savefile<<"Address "<<(*it2).first<<" had "<<(*it2).second<<" writes\n";
		}

		savefile<<"\nPower Data: \n";
		savefile<<"========================\n";

		for(uint i = 0; i < NUM_PACKAGES; i++)
		{
		    savefile<<"Package: "<<i<<"\n";
		    savefile<<"Accumulated Idle Energy: "<<((*it).idle_energy[i] * VCC * (CYCLE_TIME * 0.000000001))<<"mJ\n";
		    savefile<<"Accumulated Access Energy: "<<((*it).access_energy[i] * VCC * (CYCLE_TIME * 0.000000001))<<"mJ\n";
		    savefile<<"Accumulated VPP Idle Energy: "<<((*it).vpp_idle_energy[i] * VPP * (CYCLE_TIME * 0.000000001))<<"mJ\n";
		    savefile<<"Accumulated VPP Access Energy: "<<((*it).vpp_access_energy[i] * VPP * (CYCLE_TIME * 0.000000001))<<"mJ\n";
		    savefile<<"Total Energy: "<<(total_energy[i] * (CYCLE_TIME * 0.000000001))<<"mJ\n\n";
	 
		    savefile<<"Average Idle Power: "<<ave_idle_power[i]<<"mW\n";
		    savefile<<"Average Access Power: "<<ave_access_power[i]<<"mW\n";
		    savefile<<"Average VPP Idle Power: "<<ave_vpp_idle_power[i]<<"mW\n";
		    savefile<<"Average VPP Access Power: "<<ave_vpp_access_power[i]<<"mW\n";
		    savefile<<"Average Power: "<<average_power[i]<<"mW\n\n";
	        }
	    }
	}

	savefile.close();
}

void PCMLogger::print(uint64_t cycle) {
	// Power stuff
	// Total power used
	vector<double> total_energy = vector<double>(NUM_PACKAGES, 0.0);

        // Average power used
	vector<double> ave_idle_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> ave_access_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> ave_vpp_idle_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> ave_vpp_access_power = vector<double>(NUM_PACKAGES, 0.0);
	vector<double> average_power = vector<double>(NUM_PACKAGES, 0.0);

	for(uint i = 0; i < NUM_PACKAGES; i++)
	{
	  total_energy[i] = ((idle_energy[i] + access_energy[i]) * VCC)
	                    + ((vpp_idle_energy[i] + vpp_access_energy[i]) * VPP);
	  ave_idle_power[i] = (idle_energy[i] * VCC) / cycle;
	  ave_access_power[i] = (access_energy[i] * VCC) / cycle;
	  ave_vpp_idle_power[i] = (vpp_idle_energy[i] * VPP) / cycle;
	  ave_vpp_access_power[i] = (vpp_access_energy[i] * VPP) / cycle;
	  average_power[i] = total_energy[i] / cycle;
	}

	cout<<"Reads completed: "<<num_reads<<"\n";
	cout<<"Writes completed: "<<num_writes<<"\n";

	cout<<"\nPower Data: \n";
	cout<<"========================\n";

	for(uint i = 0; i < NUM_PACKAGES; i++)
	{
	    cout<<"Package: "<<i<<"\n";
	    cout<<"Accumulated Idle Energy: "<<(idle_energy[i] * VCC * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    cout<<"Accumulated Access Energy: "<<(access_energy[i] * VCC * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    cout<<"Accumulated VPP Idle Energy: "<<(vpp_idle_energy[i] * VPP * (CYCLE_TIME * 0.000000001))<<"mJ\n";
	    cout<<"Accumulated VPP Access Energy: "<<(vpp_access_energy[i] * VPP * (CYCLE_TIME * 0.000000001))<<"mJ\n";
		 
	    cout<<"Total Energy: "<<(total_energy[i] * (CYCLE_TIME * 0.000000001))<<"mJ\n\n";
	 
	    cout<<"Average Idle Power: "<<ave_idle_power[i]<<"mW\n";
	    cout<<"Average Access Power: "<<ave_access_power[i]<<"mW\n";
	    cout<<"Average VPP Idle Power: "<<ave_vpp_idle_power[i]<<"mW\n";
	    cout<<"Average VPP Access Power: "<<ave_vpp_access_power[i]<<"mW\n";
		  
	    cout<<"Average Power: "<<average_power[i]<<"mW\n\n";
	}
}

vector<vector<double> > PCMLogger::getEnergyData(void)
{
    vector<vector<double> > temp = vector<vector<double> >(4, vector<double>(NUM_PACKAGES, 0.0));
    for(int i = 0; i < NUM_PACKAGES; i++)
    {
	temp[0][i] = idle_energy[i];
	temp[1][i] = access_energy[i];
	temp[2][i] = vpp_idle_energy[i];
	temp[3][i] = vpp_access_energy[i];
    }
    return temp;
}

void PCMLogger::save_epoch(uint64_t cycle, uint epoch)
{    
    EpochEntry this_epoch;
    this_epoch.cycle = cycle;
    this_epoch.epoch = epoch;

    this_epoch.num_accesses = num_accesses;
    this_epoch.num_reads = num_reads;
    this_epoch.num_writes = num_writes;
	
    this_epoch.num_unmapped = num_unmapped;
    this_epoch.num_mapped = num_mapped;

    this_epoch.num_read_unmapped = num_read_unmapped;
    this_epoch.num_read_mapped = num_read_mapped;
    this_epoch.num_write_unmapped = num_write_unmapped;
    this_epoch.num_write_mapped = num_write_mapped;
		
    this_epoch.average_latency = average_latency;
    this_epoch.average_read_latency = average_read_latency;
    this_epoch.average_write_latency = average_write_latency;
    this_epoch.average_queue_latency = average_queue_latency;

    this_epoch.ftl_queue_length = ftl_queue_length;

    this_epoch.writes_per_address = writes_per_address;

    for(int i = 0; i < ctrl_queue_length.size(); i++)
    {
	this_epoch.ctrl_queue_length[i] = ctrl_queue_length[i];
    }

    for(int i = 0; i < NUM_PACKAGES; i++)
    {	
	this_epoch.idle_energy[i] = idle_energy[i]; 
	this_epoch.access_energy[i] = access_energy[i]; 
	
	this_epoch.vpp_idle_energy[i] = vpp_idle_energy[i]; 
	this_epoch.vpp_access_energy[i] = vpp_access_energy[i]; 
    }
    
    EpochEntry temp_epoch;

    temp_epoch = this_epoch;

    if(!epoch_queue.empty())
    {
	this_epoch.cycle -= last_epoch.cycle;

	this_epoch.num_accesses -= last_epoch.num_accesses;
	this_epoch.num_reads -= last_epoch.num_reads;
	this_epoch.num_writes -= last_epoch.num_writes;
	
	this_epoch.num_unmapped -= last_epoch.num_unmapped;
	this_epoch.num_mapped -= last_epoch.num_mapped;
	
	this_epoch.num_read_unmapped -= last_epoch.num_read_unmapped;
	this_epoch.num_read_mapped -= last_epoch.num_read_mapped;
	this_epoch.num_write_unmapped -= last_epoch.num_write_unmapped;
	this_epoch.num_write_mapped -= last_epoch.num_write_mapped;
	
	this_epoch.average_latency -= last_epoch.average_latency;
	this_epoch.average_read_latency -= last_epoch.average_read_latency;
	this_epoch.average_write_latency -= last_epoch.average_write_latency;
	this_epoch.average_queue_latency -= last_epoch.average_queue_latency;
	
	for(int i = 0; i < NUM_PACKAGES; i++)
	{	
	    this_epoch.idle_energy[i] -= last_epoch.idle_energy[i]; 
	    this_epoch.access_energy[i] -= last_epoch.access_energy[i]; 

	    this_epoch.vpp_idle_energy[i] -= last_epoch.vpp_idle_energy[i]; 
	    this_epoch.vpp_access_energy[i] -= last_epoch.vpp_access_energy[i]; 
	}
    }
    
    epoch_queue.push_front(this_epoch);

    last_epoch = temp_epoch;
}
