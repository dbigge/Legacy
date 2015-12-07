/*******************************************************************************************
 
			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++
   
 [Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php
  
*******************************************************************************************/

#include "unifiedtime.h"
#include "../LLP/client.h"


using namespace std;


/** Flag Declarations **/
bool fTimeUnified = false;
bool fIsTimeSeed = false;
bool fIsSeedNode = false;


int UNIFIED_AVERAGE_OFFSET = 0;
int UNIFIED_MOVING_ITERATOR = 0;


/** Unified Time Declarations **/
vector<int> UNIFIED_TIME_DATA;
vector<Net::CAddress> TIME_SEEDS;
vector<Net::CAddress> SEED_NODES;


/** Declarations for DNS Time Seed Nodes. **/
const char* DNS_TimeSeeds[] = 
{ 
	"69.195.148.34", 
	"69.195.148.42" 
};

/** Declarations for DNS Time Seed Nodes. **/
const char* DNS_TimeSeeds_Testnet[] = 
{
	"204.27.62.226"
};


/** Declarations for the DNS Seed Nodes. **/
const char* DNS_SeedNodes[] = 
{
	"69.195.149.114",
	"204.27.62.242",
	"204.27.62.234",
	"204.27.62.226"
};

/** Declarations for the DNS Seed Nodes. **/
const char* DNS_SeedNodes_Testnet[] = 
{
	"69.195.149.114"
};



/** Baseline Maximum Values for Unified Time. **/
int MAX_UNIFIED_DRIFT   = 20;
int MAX_UNIFIED_SAMPLES = 20;


/** Gets the UNIX Timestamp from your Local Clock **/
int64 GetLocalTimestamp(){ return time(NULL); }


/** Gets the UNIX Timestamp from the Nexus Network **/
int64 GetUnifiedTimestamp(){ return GetLocalTimestamp() + UNIFIED_AVERAGE_OFFSET; }


/** Called from Thread Time Regulator.
    This keeps time keeping separate from regular processing. **/
void InitializeUnifiedTime()
{
	Wallet::CTimeDB init("cr"); ///init("cw")
	fTimeUnified = init.ReadTimeData(UNIFIED_AVERAGE_OFFSET);
    init.Close();
	
	printf("***** Unified Time Initialized to %i\n", UNIFIED_AVERAGE_OFFSET);
}


/** Calculate the Average Unified Time. Called after Time Data is Added **/
int GetUnifiedAverage()
{
	if(UNIFIED_TIME_DATA.empty())
		return UNIFIED_AVERAGE_OFFSET;
		
	int nAverage = 0;
	for(int index = 0; index < std::min(MAX_UNIFIED_SAMPLES, (int)UNIFIED_TIME_DATA.size()); index++)
		nAverage += UNIFIED_TIME_DATA[index];
		
	return round(nAverage / (double) std::min(MAX_UNIFIED_SAMPLES, (int)UNIFIED_TIME_DATA.size()));
}


/** Regulator of the Unified Clock **/
void ThreadTimeRegulator(void* parg)
{
	/** Compile the Seed Nodes into a set of Vectors. **/
	TIME_SEEDS    = DNS_Lookup(fTestNet ? DNS_TimeSeeds_Testnet : DNS_TimeSeeds);
	SEED_NODES    = DNS_Lookup(fTestNet ? DNS_SeedNodes_Testnet : DNS_SeedNodes);
	
	
	/** Iterator to be used to ensure every time seed is giving an equal weight towards the Global Seeds. **/
	int nIterator = 0;
	
	/** Track the Connection Attempts **/
	int nAttempts = 0;
	
	/** Compile all the Seeds into one Vector. **/
	printf("Compiling List...\n");
	vector<string> SEEDS;
	for(int nIndex = 0; nIndex < TIME_SEEDS.size(); nIndex++)
		SEEDS.push_back(TIME_SEEDS[nIndex].ToStringIP());
	
	/** The Entry Client Loop for Core LLP. **/
	string ADDRESS = "";
	LLP::CoreOutbound SERVER("", strprintf("%u", (fTestNet ? TESTNET_CORE_LLP_PORT : Nexus_CORE_LLP_PORT)));
	loop
	{
		try
		{
		
			/** Increment the Time Seed Connection Iterator. **/
			nIterator++;
			
			
			/** Reset the ITerator if out of Seeds. **/
			if(nIterator == SEEDS.size())
			   nIterator = 0;
				
				
			/** Connect to the Next Seed in the Iterator. **/
			SERVER.IP = SEEDS[nIterator];
			SERVER.Connect();
			
			
			/** If the Core LLP isn't connected, Retry in 10 Seconds. **/
			if(!SERVER.Connected())
			{
				printf("***** Core LLP: Failed To Connect To %s:%s\n", SERVER.IP.c_str(), SERVER.PORT.c_str());
				
				/** Only Sleep for 10 Seconds if there are more than 2 consecutive Failed Connections. **/
				if(nAttempts > 1)
					Sleep(10000);
				
				nAttempts++;
				continue;
			}
			
			
			/** Reset the Connection Attempts. **/
			nAttempts = 0;

			
			/** Use a CMajority to Find the Sample with the Most Weight. **/
			CMajority<int> nSamples;
			
			
			/** Get 10 Samples From Server. **/
			SERVER.GetOffset((unsigned int)GetLocalTimestamp());
				
				
			/** Read the Samples from the Server. **/
			while(SERVER.Connected() && !SERVER.Errors() && !SERVER.Timeout(3))
			{
				Sleep(1);
			
				SERVER.ReadPacket();
				if(SERVER.PacketComplete())
				{
					LLP::Packet PACKET = SERVER.NewPacket();
					
					/** Add a New Sample each Time Packet Arrives. **/
					if(PACKET.HEADER == SERVER.TIME_OFFSET)
					{
						int nOffset = bytes2int(PACKET.DATA);
						nSamples.Add(nOffset);
						
						SERVER.GetOffset((unsigned int)GetLocalTimestamp());
						printf("***** Core LLP: Added Sample %i | Seed %s\n", nOffset, SERVER.IP.c_str());
					}
					
					SERVER.ResetPacket();
				}
				
				/** Close the Connection Gracefully if Received all Packets. **/
				if(nSamples.Samples() == 9)
				{
					SERVER.Close();
					break;
				}
			}
			
			
			/** If there are no Successful Samples, Try another Connection. **/
			if(nSamples.Samples() == 0)
			{
				printf("***** Core LLP: Failed To Get Time Samples.\n");
				SERVER.Close();
				
				continue;
			}
			
			
			/** If the Unified Clock Changes past Max Drift, Reset Unified Moving Average. **/
			if(nSamples.Majority() > (UNIFIED_AVERAGE_OFFSET + MAX_UNIFIED_DRIFT) ||
			   nSamples.Majority() < (UNIFIED_AVERAGE_OFFSET - MAX_UNIFIED_DRIFT) )
			{
			
			   printf("***** Unified Clock Changed by Maximum Drift. Resetting Samples...\n");
			   UNIFIED_TIME_DATA.clear();
			}
			
			
			/** If the Moving Average is filled with samples, continue iterating to keep it moving. **/
			if(UNIFIED_TIME_DATA.size() >= MAX_UNIFIED_SAMPLES)
			{
				if(UNIFIED_MOVING_ITERATOR >= MAX_UNIFIED_SAMPLES)
					UNIFIED_MOVING_ITERATOR = 0;
								
				UNIFIED_TIME_DATA[UNIFIED_MOVING_ITERATOR] = nSamples.Majority();
				UNIFIED_MOVING_ITERATOR ++;
			}
			
			
			/** If The Moving Average is filling, move the iterator along with the Time Data Size. **/
			else
			{
				UNIFIED_MOVING_ITERATOR = UNIFIED_TIME_DATA.size();
				UNIFIED_TIME_DATA.push_back(nSamples.Majority());
			}
			

			/** Update Iterators and Flags. **/
			if((UNIFIED_TIME_DATA.size() > 0))
			{
				fTimeUnified = true;
				UNIFIED_AVERAGE_OFFSET = GetUnifiedAverage();
				
				Wallet::CTimeDB DB("cw");
				DB.WriteTimeData(UNIFIED_AVERAGE_OFFSET);
				DB.Close();
				
				printf("***** %i Iterator | %i Offset | %i Current | %I64d\n", UNIFIED_MOVING_ITERATOR, nSamples.Majority(), UNIFIED_AVERAGE_OFFSET, GetUnifiedTimestamp());
				
				
				/** Regulate the Clock while Waiting, and Break if the Clock Changes. **/
				unsigned int nSeconds = 0;
				while(nSeconds < 600)
				{
					int64 nTimestamp = GetLocalTimestamp();
					Sleep(1000);
					
					int64 nElapsed = GetLocalTimestamp() - nTimestamp;
					if(nElapsed > (MAX_UNIFIED_DRIFT + 1) || nElapsed < ((MAX_UNIFIED_DRIFT - 1) * -1))
					{
						UNIFIED_TIME_DATA.clear();
						UNIFIED_AVERAGE_OFFSET -= nElapsed;
						
						printf("***** LLP Clock Regulator: Time Changed by %I64d Seconds. New Offset %i\n", nElapsed, UNIFIED_AVERAGE_OFFSET);
						
						break;
					}
					
					nSeconds++;
				}
			}
		}
		catch(std::exception& e){ printf("UTM ERROR: %s\n", e.what()); }
	}
}

/** DNS Query of Domain Names Associated with Seed Nodes **/
vector<Net::CAddress> DNS_Lookup(const char* DNS_Seed[])
{
	vector<Net::CAddress> vNodes;
    for (unsigned int seed = 0; seed < ARRAYLEN(DNS_Seed); seed++)
	{
		printf("Host: %s\n", DNS_Seed[seed]);
        vector<Net::CNetAddr> vaddr;
        if (Net::LookupHost(DNS_Seed[seed], vaddr))
        {
            BOOST_FOREACH(Net::CNetAddr& ip, vaddr)
            {
                Net::CAddress addr = Net::CAddress(Net::CService(ip, Net::GetDefaultPort()));
                vNodes.push_back(addr);
				
				Net::addrman.Add(addr, ip, true);
            }
        }
    }
	
	return vNodes;
}
