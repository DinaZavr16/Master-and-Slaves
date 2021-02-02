#pragma once

#include <stdio.h>
#include "Master.h"
#include "Structures.h"

void printMaster(struct Master master)
{
	printf("Master\'s name: %s\n", master.name);
	printf("Master\'s standing: %d\n", master.standing);
}

void printSlave(struct Slave slave, struct Master master)
{
	printf("Master: %s, %d scores\n", master.name, master.standing);
	printf("Price: %d\n", slave.price);
	printf("Amount of trainings: %d\n", slave.amountTrainings);
}