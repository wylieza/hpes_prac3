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

/** This is the master node function, describing the operations
    that the master will be doing */
void Master () {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 int  j;             //! j: Loop counter
 char buff[BUFSIZE]; //! buff: Buffer for transferring message data
 MPI_Status stat;    //! stat: Status of the MPI application

 // Start of "Hello World" example..............................................
 printf("0: We have %d processors\n", numprocs);
 for(j = 1; j < numprocs; j++) {
  sprintf(buff, "Hello %d! ", j);
  MPI_Send(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
 }
 for(j = 1; j < numprocs; j++) {
  // This is blocking: normally one would use MPI_Iprobe, with MPI_ANY_SOURCE,
  // to check for messages, and only when there is a message, receive it
  // with MPI_Recv.  This would let the master receive messages from any
  // slave, instead of a specific one only.
  MPI_Recv(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
  printf("0: %s\n", buff);
 }
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
 for(j = 0; j < 10; j++){
     j = 9; //ONLY RUN ONCE DURING DEV.
  tic();
  int x, y;
  int count = 0;
  int index = 0;
  printf("Size: %d\n", Input.Height*Input.Width);
  pixel *pixels = (pixel*) malloc(Input.Height*Input.Width*sizeof(pixel));
  printf("Pixel generating\n");
  for(y = 0; y < Input.Height; y++){
   for(x = 0; x < Input.Width*Input.Components; x++){
    //Output.Rows[y][x] = Input.Rows[y][x];

    //Generate the pixels
    if (count%3 == 0 && j == 9){
        *(pixels+index++) = {(u_char)Input.Rows[y][x], (u_char)Input.Rows[y][x+1], (u_char)Input.Rows[y][x+2]};
    }
    count++;
   }
  }

    printf("Pixel filtering\n");

    //filter with MD filter
    int yl = Input.Height; //Number of rows
    int xl = (Input.Width); //Number of pixels wide
    int offset;
    int empty = 0;
    for (int i = 0; i < yl*xl; i++){
        pixel npx[8];
        
        //cases:
        if(i == 0){ //1
            offset = i+1;
            npx[0] = *(pixels + offset);
            
            offset = i+xl+1;
            npx[1] = *(pixels + offset);

            offset = i+xl;
            npx[2] = *(pixels + offset);

            empty = 5;
        }

        if(i == xl-1){ //2
            offset = i-1;
            npx[0] = *(pixels + offset);
            
            offset = i+xl-1;
            npx[1] = *(pixels + offset);

            offset = i+xl;
            npx[2] = *(pixels + offset);

            empty = 5;
        }

        if(i == (yl-1)*xl){ //3
            offset = i-xl;
            npx[0] = *(pixels + offset);
            
            offset = i-xl+1;
            npx[1] = *(pixels + offset);

            offset = i+1;
            npx[2] = *(pixels + offset);

            empty = 5;
        }

        if(i == yl*xl-1){ //4
            offset = i-xl-1;
            npx[0] = *(pixels + offset);
            
            offset = i-xl;
            npx[1] = *(pixels + offset);

            offset = i-1;
            npx[2] = *(pixels + offset);

            empty = 5;
        }

        if(i == yl*xl-1){ //4
            offset = i-xl-1;
            npx[0] = *(pixels + offset);
            
            offset = i-xl;
            npx[1] = *(pixels + offset);

            offset = i-1;
            npx[2] = *(pixels + offset);

            empty = 5;
        }
        
        if(i%xl == 0 && empty == 0){ //5
            offset = i-xl;
            npx[0] = *(pixels + offset);
            
            offset = i-xl+1;
            npx[1] = *(pixels + offset);

            offset = i+1;
            npx[2] = *(pixels + offset);
            
            offset = i+xl+1;
            npx[3] = *(pixels + offset);

            offset = i+xl;
            npx[4] = *(pixels + offset);

            empty = 3;
        }

        if(i < xl-1 && empty == 0){ //6
            offset = i+1;
            npx[0] = *(pixels + offset);
            
            offset = i+xl+1;
            npx[1] = *(pixels + offset);

            offset = i+xl;
            npx[2] = *(pixels + offset);
            
            offset = i-1;
            npx[3] = *(pixels + offset);
            
            offset = i+xl-1;
            npx[4] = *(pixels + offset);

            empty = 3;
        }

        if((i+1)%xl == 0 && empty == 0){ //7
            offset = i-xl-1;
            npx[0] = *(pixels + offset);
            
            offset = i-xl;
            npx[1] = *(pixels + offset);

            offset = i-1;
            npx[2] = *(pixels + offset);
            
            offset = i+xl-1;
            npx[3] = *(pixels + offset);

            offset = i+xl;
            npx[4] = *(pixels + offset);

            empty = 3;
        }

        if(i > (yl-1)*xl && empty == 0){ //8
            offset = i-xl-1;
            npx[0] = *(pixels + offset);
            
            offset = i-xl;
            npx[1] = *(pixels + offset);

            offset = i-1;
            npx[2] = *(pixels + offset);
            
            offset = i-xl+1;
            npx[1] = *(pixels + offset);

            offset = i+1;
            npx[2] = *(pixels + offset);

            empty = 3;
        }

        if(empty == 0){
            offset = i-xl-1;
            npx[0] = *(pixels + offset);
            
            offset = i-xl;
            npx[1] = *(pixels + offset);

            offset = i-1;
            npx[2] = *(pixels + offset);
            
            offset = i-xl+1;
            npx[3] = *(pixels + offset);

            offset = i+1;
            npx[4] = *(pixels + offset);

            offset = i+xl-1;
            npx[5] = *(pixels + offset);
            
            offset = i+xl;
            npx[6] = *(pixels + offset);

            offset = i+xl+1;
            npx[7] = *(pixels + offset);

            
        }else{

            for(int i = 8-empty; i < 8; i++){
                npx[i] = {0, 0, 0};
            }
        }
        empty = 0;

        pixel mdpixel = ((pixels + i)->determine_median(&(npx[0])));

        Output.Rows[(int) i/xl][i%xl*3] = mdpixel.r;
        Output.Rows[(int) i/xl][(i%xl*3) + 1] = mdpixel.g;
        Output.Rows[(int) i/xl][(i%xl*3) + 2] = mdpixel.b;

        //printf("Pixel %d processed\n", i);
    }

  printf("Time = %lg ms\n", (double)toc()/1e-3);

for(int i = 0; i < 1000 && j == 9; i++){
    pixel mdpx = (pixels+i)->determine_median((pixels+i+1));
    //printf("MDPX from main -> R: %d, G: %d, B: %d\n", mdpx.r, mdpx.g, mdpx.b);
}

 }
 printf("End of example code...\n\n");
 // End of example -------------------------------------------------------------

 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }
 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
 // Start of "Hello World" example..............................................
 char idstr[32];
 char buff [BUFSIZE];

 MPI_Status stat;

 // receive from rank 0 (master):
 // This is a blocking receive, which is typical for slaves.
 MPI_Recv(buff, BUFSIZE, MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
 sprintf(idstr, "Processor %d ", ID);
 strncat(buff, idstr, BUFSIZE-1);
 strncat(buff, "reporting for duty", BUFSIZE-1);

 // send to rank 0 (master):
 MPI_Send(buff, BUFSIZE, MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
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
