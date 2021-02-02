#pragma once


struct Master
{
	int id;
	char name[16];
	int standing;
	long firstSlaveAddress;
	int slavesCount;
};

struct Slave
{
	int masterId;
	int personID;
	int price;
	int amountTrainings;
	int exists;
	long selfAddress;
	long nextAddress;
};

struct Indexer
{
	int id;	
	int address;
	int exists;
};