//==============================================================================
// Copyright (C) John-Philip Taylor
// tyljoh010@myuct.ac.za
//
// This file is part of the EEE4084F Course
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>
//
// This is an adaptition of The "Hello World" example avaiable from
// https://en.wikipedia.org/wiki/Message_Passing_Interface#Example_program
//==============================================================================


/** \mainpage Prac3 Main Page
 *
 * \section intro_sec Introduction
 *
 * The purpose of Prac3 is to learn some basics of MPI coding.
 *
 * Look under the Files tab above to see documentation for particular files
 * in this project that have Doxygen comments.
 */



//---------- STUDENT NUMBERS --------------------------------------------------
//
// MLLDYL002
// WYLJUS002
//
//-----------------------------------------------------------------------------

/* Note that Doxygen comments are used in this file. */
/** \file Prac3
 *  Prac3 - MPI Main Module
 *  The purpose of this prac is to get a basic introduction to using
 *  the MPI libraries for prallel or cluster-based programming.
 */

// Includes needed for the program
#include "Prac3.h"

struct __attribute__((packed)) payload
{
int magic;
int cid;
int width;
int csize;
int size;
};

//Pixel Struct
typedef struct pixel{
    u_char r;
    u_char g;
    u_char b;

    pixel determine_median(pixel *neibours){ //Assumes 8 neibours will be passed
        //Create an ordered list of values
        u_char reds[9];
        u_char greens[9];
        u_char blues[9];

        //Insert first value
        reds[0] = r;
        greens[0] = g;
        blues[0] = b;

        //Insert into array making sure they are stored in asseding order (Thus the 5th item (reds[4]) is the median r value)
        for (int i = 0; i < 8; i++){
            u_char insertion_rvalue = (neibours+i)->r;
            u_char insertion_gvalue = (neibours+i)->g;
            u_char insertion_bvalue = (neibours+i)->b;
            for (int j = 0; j < i+1; j++){
                u_char temp;

                if (reds[j] > insertion_rvalue)
                {
                    temp = reds[j];
                    reds[j] = insertion_rvalue;
                    insertion_rvalue = temp;
                }

                if (greens[j] > insertion_gvalue)
                {
                    temp = greens[j];
                    greens[j] = insertion_gvalue;
                    insertion_gvalue = temp;
                }

                if (blues[j] > insertion_bvalue)
                {
                    temp = blues[j];
                    blues[j] = insertion_bvalue;
                    insertion_bvalue = temp;
                }                
            }
            reds[i+1] = insertion_rvalue;
            greens[i+1] = insertion_gvalue;
            blues[i+1] = insertion_bvalue;
        }

        pixel median_pixel = {reds[4], greens[4], blues[4]}; //Select the median (middle value) - easy because the array is in ascending order...
        return median_pixel; //Return the median pixel
    }
} pixel;
void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}
/** This is the master node function, describing the operations
    that the master will be doing */
void Master () {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 int  j;             //! j: Loop counter
 char buff[BUFSIZE]; //! buff: Buffer for transferring message data
 MPI_Status stat;    //! stat: Status of the MPI application
char *pool = (char*)malloc(BUFSIZE + sizeof(struct payload));

 // End of "Hello World" example................................................

 // Read the input image
 if(!Input.Read("Data/greatwall.jpg")){
  printf("Cannot read image\n");
  return;
 }

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;



 // This is example code of how to copy image files ----------------------------
 printf("Start of example code...\n");
 int size = Input.Height*Input.Width;
 int segment = (size / (numprocs-1) ) * Input.Components;
 int packets = ceil((double)(segment / BUFSIZE ));


 printf("size : %d\nsegment : %d\npackets : %d", size, segment, packets);
 for(j = 1; j < numprocs; j++){
  struct payload pl;
  pl.magic = 0xFE;

  pl.cid = j;
  pl.csize = packets;
  pl.width = Input.Width;
  pl.size = segment / Input.Components;
  memcpy((void*)pool, (void*)&pl, sizeof(struct payload));
  MPI_Send((void*)pool, sizeof(struct payload), MPI_CHAR, j, TAG, MPI_COMM_WORLD);

  // send image data
  for(int k = 0; k < packets; k++){
  memcpy((void*)(char*)pool, (void*)(&Input.Rows[0][0] + (segment * (j-1)) + (BUFSIZE * k)), BUFSIZE);
  MPI_Send((void*)pool, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
 

 } 


 }

}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
 // Start of "Hello World" example..............................................
 char idstr[32];
 char buff [BUFSIZE + sizeof(struct payload)];
 char *cluster = 0;
 MPI_Status stat;

 // receive from rank 0 (master):
 // This is a blocking receive, which is typical for slaves.


 MPI_Recv(buff,  sizeof(struct payload), MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
 struct payload * pptr = (struct payload*)buff;
 int csize = pptr->csize;
 int width = 0;
 int height = 0;
if(pptr->magic == 0xFE){
 //printf("PROCESSING CLUSTER |%d|\n", pptr->cid);
 //printf("magic : %d\n", pptr->magic);
 //printf("cid : %d\n", pptr->cid);
 //printf("csize : %d\n", pptr->csize);
 //printf("width : %d\n", pptr->width);


 height = pptr->size / pptr->width ;
 width = pptr->width;
 cluster = (char*)malloc (BUFSIZE * csize);

 for(int z = 0; z < csize; z++){
  MPI_Recv(buff, BUFSIZE , MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
  memcpy((void*)((char*)cluster + (z * BUFSIZE)), (char*)buff , BUFSIZE);
  
}
 //cluster contains the image subsection pixel data (r,g,b) array
 // height, width are vars for each subsection
 //DumpHex((void*)((char*)cluster), BUFSIZE * 2 );
 //printf("\n");
}


 




 // End of "Hello World" example................................................
}
//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char** argv){
 int myid;

 // MPI programs start with MPI_Init
 MPI_Init(&argc, &argv);

 // find out how big the world is
 MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

 // and this processes' rank is
 MPI_Comm_rank(MPI_COMM_WORLD, &myid);

 // At this point, all programs are running equivalently, the rank
 // distinguishes the roles of the programs, with
 // rank 0 often used as the "master".
 if(myid == 0) Master();
 else          Slave (myid);

 // MPI programs end with MPI_Finalize
 MPI_Finalize();
 return 0;
}
//------------------------------------------------------------------------------
