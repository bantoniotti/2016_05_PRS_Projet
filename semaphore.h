/*
 * Header: semaphore.h
 * 
 * Provides wrapper functions around POSIX system calls semget, 
 * semctl and semop allowing the creation, initialization, opening
 * and removal of a single semaphore at a time and implementing 
 * operations down (also known as P or wait) and up (also known as V 
 * or signal) on it.
 * 
 * 
 * Copyright 2014 Razmig Kéchichian, INSA de Lyon, TC department
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 * 
 */ 

#ifndef __SEMAPHORE_H_SEMAPHORE__
#define __SEMAPHORE_H_SEMAPHORE__


#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>


int create_semaphore(int key)
{
	return semget(key, 1, 0660 | IPC_CREAT) ;
}

int open_semaphore(int key)
{
	return semget(key, 1, 0660) ;
}

int remove_semaphore(int id) 
{
	return semctl(id, 0, IPC_RMID) ;
}

int init_semaphore(int id, int val)
{
	return semctl(id, 0, SETVAL, val) ;
}

int up(int id)
{
	struct sembuf op ;
	op.sem_num = 0 ;
	op.sem_op = 1 ;
	op.sem_flg = 0 ;
	
	return semop(id, &op, 1) ;
}

int down(int id)
{
	struct sembuf op ;
	op.sem_num = 0 ;
	op.sem_op = -1 ;
	op.sem_flg = 0 ;
	
	return semop(id, &op, 1) ;
}


#endif
