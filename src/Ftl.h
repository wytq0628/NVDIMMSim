#ifndef FTL_H
#define FTL_H
//Ftl.h
//header file for the ftl

#include "SimObj.h"
#include "FlashConfiguration.h"
#include "ChannelPacket.h"
#include "FlashTransaction.h"
#include "Controller.h"

namespace NVDSim{
        class NVDIMMl;
	class Ftl : public SimObj{
		public:
                        Ftl(Controller *c, NVDIMM *parent);
			ChannelPacket *translate(ChannelPacketType type, uint64_t vAddr, uint64_t pAddr);
			bool addTransaction(FlashTransaction &t);
			void update(void);

#if GC
			bool checkGC(void); 
			void runGC(void); 
#endif
			void returnIdlePower(void);
			void returnAccessPower(void);
#if GC
			void returnErasePower(void);
#endif

			uint64_t get_ptr(void); 
			void inc_ptr(void); 
			Controller *controller;
			NVDIMM *parentNVDIMM;
		private:
			uint offset,  pageBitWidth, blockBitWidth, planeBitWidth, dieBitWidth, packageBitWidth;
			uint channel, die, plane, lookupCounter;
			uint64_t used_page_count;
			FlashTransaction currentTransaction;
			uint busy;
			std::unordered_map<uint64_t,uint64_t> addressMap;
			
#if GC
			std::vector<vector<bool>> dirty;
#endif

			std::vector<vector<bool>> used;
			std::list<FlashTransaction> transactionQueue;
			std::unordered_map<uint64_t,uint64_t> erase_counter;

			// Power Stuff
			// This is computed per package
			std::vector<uint64_t> idle_energy;
			std::vector<uint64_t> access_energy;
#if GC
			std::vector<uint64_t> erase_energy;
#endif
			
	};
}
#endif
