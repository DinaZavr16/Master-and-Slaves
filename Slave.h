#pragma once
#pragma warning(disable:4996)

#include <stdio.h>
#include <stdlib.h>
#include "Structures.h"
#include "Checks.h"
#include "Master.h"

#define SLAVE_DATA "slave.fl"
#define SLAVE_GARBAGE "slave_garbage.txt"
#define SLAVE_SIZE sizeof(struct Slave)

void reopenDatabase(FILE* database)
{
	fclose(database);
	database = fopen(SLAVE_DATA, "r+b");
}

void linkAddresses(FILE* database, struct Master master, struct Slave slave)
{
	reopenDatabase(database);								

	struct Slave previous;

	fseek(database, master.firstSlaveAddress, SEEK_SET);

	for (int i = 0; i < master.slavesCount; i++)		    
	{
		fread(&previous, SLAVE_SIZE, 1, database);
		fseek(database, previous.nextAddress, SEEK_SET);
	}

	previous.nextAddress = slave.selfAddress;				
	fwrite(&previous, SLAVE_SIZE, 1, database);				
}

void relinkAddresses(FILE* database, struct Slave previous, struct Slave slave, struct Master* master)
{
	if (slave.selfAddress == master->firstSlaveAddress)		
	{
		if (slave.selfAddress == slave.nextAddress)			
		{
			master->firstSlaveAddress = -1;					
		}
		else                                                
		{
			master->firstSlaveAddress = slave.nextAddress;  
		}
	}
	else                                                    
	{
		if (slave.selfAddress == slave.nextAddress)			
		{
			previous.nextAddress = previous.selfAddress;    
		}
		else                                                
		{
			previous.nextAddress = slave.nextAddress;		
		}

		fseek(database, previous.selfAddress, SEEK_SET);	
		fwrite(&previous, SLAVE_SIZE, 1, database);			
	}
}

void noteDeletedSlave(long address)
{
	FILE* garbageZone = fopen(SLAVE_GARBAGE, "rb");			

	int garbageCount;
	fscanf(garbageZone, "%d", &garbageCount);

	long* delAddresses = malloc(garbageCount * sizeof(long)); 

	for (int i = 0; i < garbageCount; i++)
	{
		fscanf(garbageZone, "%ld", delAddresses + i);		
	}

	fclose(garbageZone);									
	garbageZone = fopen(SLAVE_GARBAGE, "wb");				
	fprintf(garbageZone, "%ld", garbageCount + 1);			

	for (int i = 0; i < garbageCount; i++)
	{
		fprintf(garbageZone, " %ld", delAddresses[i]);		
	}

	fprintf(garbageZone, " %d", address);					
	free(delAddresses);										
	fclose(garbageZone);									
}

void overwriteGarbageAddress(int garbageCount, FILE* garbageZone, struct Slave* record)
{
	long* delIds = malloc(garbageCount * sizeof(long));		

	for (int i = 0; i < garbageCount; i++)
	{
		fscanf(garbageZone, "%d", delIds + i);				
	}

	record->selfAddress = delIds[0];						
	record->nextAddress = delIds[0];

	fclose(garbageZone);									
	fopen(SLAVE_GARBAGE, "wb");							    
	fprintf(garbageZone, "%d", garbageCount - 1);			

	for (int i = 1; i < garbageCount; i++)
	{
		fprintf(garbageZone, " %d", delIds[i]);				
	}

	free(delIds);											
	fclose(garbageZone);									
}

int insertSlave(struct Master master, struct Slave slave, char* error)
{
	slave.exists = 1;

	FILE* database = fopen(SLAVE_DATA, "a+b");
	FILE* garbageZone = fopen(SLAVE_GARBAGE, "rb");

	int garbageCount;											

	fscanf(garbageZone, "%d", &garbageCount);

	if (garbageCount)											
	{
		overwriteGarbageAddress(garbageCount, garbageZone, &slave);
		reopenDatabase(database);								
		fseek(database, slave.selfAddress, SEEK_SET);			
	}
	else                                                       
	{
		fseek(database, 0, SEEK_END);

		int dbSize = ftell(database);

		slave.selfAddress = dbSize;	
		slave.nextAddress = dbSize;
	}

	fwrite(&slave, SLAVE_SIZE, 1, database);					

	if (!master.slavesCount)								    
	{
		master.firstSlaveAddress = slave.selfAddress;
	}
	else                                                       
	{
		linkAddresses(database, master, slave);
	}

	fclose(database);											

	master.slavesCount++;										
	updateMaster(master, error);								

	return 1;
}

int getSlave(struct Master master, struct Slave* slave, int personId, char* error)
{
	if (!master.slavesCount)									
	{
		strcpy(error, "This master has no slaves yet");
		return 0;
	}

	FILE* database = fopen(SLAVE_DATA, "rb");


	fseek(database, master.firstSlaveAddress, SEEK_SET);		
	fread(slave, SLAVE_SIZE, 1, database);

	for (int i = 0; i < master.slavesCount; i++)				
	{
		if (slave->personID == personId)						
		{
			fclose(database);
			return 1;
		}

		fseek(database, slave->nextAddress, SEEK_SET);
		fread(slave, SLAVE_SIZE, 1, database);
	}
	
	strcpy(error, "No such slave in database");					
	fclose(database);
	return 0;
}

int updateSlave(struct Slave slave, int personId)
{
	FILE* database = fopen(SLAVE_DATA, "r+b");

	fseek(database, slave.selfAddress, SEEK_SET);
	fwrite(&slave, SLAVE_SIZE, 1, database);
	fclose(database);
	
	return 1;
}

int deleteSlave(struct Master master, struct Slave slave, int personId, char* error)
{
	FILE* database = fopen(SLAVE_DATA, "r+b");
	struct Slave previous;

	fseek(database, master.firstSlaveAddress, SEEK_SET);

	do		                                                    
	{															
		fread(&previous, SLAVE_SIZE, 1, database);
		fseek(database, previous.nextAddress, SEEK_SET);
	}
	while (previous.nextAddress != slave.selfAddress && slave.selfAddress != master.firstSlaveAddress);

	relinkAddresses(database, previous, slave, &master);
	noteDeletedSlave(slave.selfAddress);						

	slave.exists = 0;											

	fseek(database, slave.selfAddress, SEEK_SET);				
	fwrite(&slave, SLAVE_SIZE, 1, database);					
	fclose(database);

	master.slavesCount--;										
	updateMaster(master, error);

}