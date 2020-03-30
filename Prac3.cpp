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
    int num_msgs;
    int num_pixels;
};

//Pixel Struct
typedef struct pixel
{
    u_char r;
    u_char g;
    u_char b;

} pixel;

pixel determine_median(pixel *thisp, pixel *neibours)
{ //Assumes 8 neibours will be passed
    //Create an ordered list of values
    u_char reds[9];
    u_char greens[9];
    u_char blues[9];

    //Insert first value
    reds[0] = thisp->r;
    greens[0] = thisp->g;
    blues[0] = thisp->b;

    //Insert into array making sure they are stored in asseding order (Thus the 5th item (reds[4]) is the median r value)
    for (int i = 0; i < 8; i++)
    {
        u_char insertion_rvalue = (neibours + i)->r;
        u_char insertion_gvalue = (neibours + i)->g;
        u_char insertion_bvalue = (neibours + i)->b;
        for (int j = 0; j < i + 1; j++)
        {
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
        reds[i + 1] = insertion_rvalue;
        greens[i + 1] = insertion_gvalue;
        blues[i + 1] = insertion_bvalue;
    }

    pixel median_pixel = {reds[4], greens[4], blues[4]}; //Select the median (middle value) - easy because the array is in ascending order...
    return median_pixel;                                 //Return the median pixel
}

void DumpHex(const void *data, size_t size)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i)
    {
        printf("%02X ", ((unsigned char *)data)[i]);
        if (((unsigned char *)data)[i] >= ' ' && ((unsigned char *)data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *)data)[i];
        }
        else
        {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            }
            else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}
/** This is the master node function, describing the operations
    that the master will be doing */
void Master()
{
    //! <h3>Local vars</h3>
    // The above outputs a heading to doxygen function entry
    int j;                                                    //! j: Loop counter
    char buff[BUFSIZE];                                       //! buff: Buffer for transferring message data
    MPI_Status stat;                                          //! stat: Status of the MPI application
    char *data_frame = (char *)malloc(BUFSIZE + sizeof(int)); //holds info to send over mpi message

    // End of "Hello World" example................................................

    // Read the input image
    if (!Input.Read("Data/small.jpg"))
    {
        printf("Cannot read image\n");
        return;
    }

    int total_num_pixels = Input.Height * Input.Width;          //Total number of pixels
    int num_rows = ceil((double)Input.Height / (numprocs - 1)); //Number of rows for each slave... The final slave just does remaining rows (remaining rows < num_rows)

    //printf("Height: %d\n", Input.Height);

    int image_size = total_num_pixels * Input.Components; //Image size in bytes
    char *tmp = (char *)malloc(image_size); //Declaring tmp

    for (int y = 0; y < Input.Height; y++)
    {
        for (int x = 0; x < Input.Width * Input.Components; x++)
        {
            tmp[(y * Input.Width * Input.Components) + x] = Input.Rows[y][x];
        }
    }

    int rows = 0;
    int total_image_data_sent = 0; //Track bytes already sent and index tmp memory accordingly
    int tids_offset = 0;           //Offset to include the additional two rows
    for (j = 1; j < numprocs; j++)
    {
        struct payload pl;
        pl.magic = 0xFE;

        pl.cid = j;
        if (j != numprocs - 1)
        {                          //All slaves except the last
            rows = num_rows;       //Number of rows that can be reconstructed from the data            
        }
        else
        {                                   //This is the last slave
            rows = Input.Height % num_rows; //The remaining number of rows (could be zero in this case evenly divided)
            if (rows == 0)
            {
                rows = num_rows;
            }
        }
        pl.num_msgs = ceil((double)((rows * Input.Width * Input.Components) / (double)BUFSIZE)); //Number of messages req to send all the rows in num_rows, final slave to send less msgs
        int additional_msgs = ceil((double)(((rows + 2) * Input.Width * Input.Components) / (double)BUFSIZE)) - pl.num_msgs;

        if (j != 1)
        { //Not first slave so add overlap rows
            printf("Slave %d -> Originally %d msgs... but now add %d to get >%d msgs<\n", j, pl.num_msgs, additional_msgs, pl.num_msgs+ additional_msgs);
            pl.num_msgs += additional_msgs;
            rows += 2;
            tids_offset += 2 * Input.Width * 3;
        }
        else
        {
            printf("Slave %d -> >%d msgs<\n", j, pl.num_msgs);
            tids_offset = 0;
        }

        pl.width = Input.Width;          //Number of pixels in a rows
        pl.num_pixels = pl.width * rows; //Number of pixels in the segment

        memcpy((void *)data_frame, (void *)&pl, sizeof(struct payload));                        //memcpy(dest, source, number bytes to cpy)
        MPI_Send((void *)data_frame, sizeof(struct payload), MPI_CHAR, j, TAG, MPI_COMM_WORLD); //Send the details of incoming data

        //Send image data
        for (int k = 0; k < pl.num_msgs /*&& total_image_data_sent < image_size*/; k++) //Added second condition because last slave doesnt utilize num_msgs but rather sends till image is sent
        {
            //Write message number to data frame
            int *ip = (int *)data_frame; //casting so we can write first 4 bytes
            *ip = k;

            //Write pixel data to data frame
            if (k != (pl.num_msgs - 1) /*&& total_image_data_sent < image_size - 128*/) //Second condition because last slave's num_msgs is incorrect
            {
                if(14748800 < total_image_data_sent - tids_offset){
                    printf("Mem copy from index: %d to be sent in msg %d\n", total_image_data_sent - tids_offset, k);
                }
                memcpy((void *)(char *)data_frame + sizeof(int), (void *)(tmp + total_image_data_sent - tids_offset), BUFSIZE); //Tack on the image data (128 bytes of idata)
                total_image_data_sent += BUFSIZE;

                if(14748800 < total_image_data_sent - tids_offset){
                    printf("Mem copy from index: %d success!\n", total_image_data_sent - tids_offset);
                }
            }
            else
            {
                printf("Sending final msg to slave %d\n", j);
                int remaining_bytes = (pl.num_pixels * 3) % BUFSIZE;
                if (remaining_bytes == 0)
                {
                    remaining_bytes = BUFSIZE;
                }
                memcpy((void *)(char *)data_frame + sizeof(int), (void *)(tmp + total_image_data_sent - tids_offset), remaining_bytes); //Tack on the image data
                total_image_data_sent += remaining_bytes;
            }
            MPI_Send((void *)data_frame, BUFSIZE + sizeof(int), MPI_CHAR, j, TAG, MPI_COMM_WORLD);
        }
    }

    //Recieve the results
    char *image_segment = (char *)malloc(Input.Height*Input.Width*Input.Components); //Entire image reconstruction data
    int base_offset = 0; //Keeps track in bytes of the offset based on the last slave

    //int debuglast = -128;

    for (int j = 1; j < numprocs; j++)
    {
        printf("Recieving from slave: %d\n", j);
        struct payload *plptr;
        do{
        MPI_Recv(buff, sizeof(struct payload), MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
        plptr = (struct payload *)buff;
        }while(plptr->magic != 0xFE);
        int remaining_bytes = plptr->num_pixels * 3;
        int width = 0;
        int height = 0;
        int num_msgs = plptr->num_msgs;
        if (plptr->magic == 0xFE)
        { 
            height = plptr->num_pixels / plptr->width; //Num pixels in segment/width (in pixels)
            width = plptr->width;
            

            printf("Incoming messages: %d\n", num_msgs);

            while (remaining_bytes > 0)
            {
                MPI_Recv(buff, BUFSIZE + sizeof(int), MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
                int id = *(int *)buff;
                //printf("REC ID %d\n", id);
                if (id < num_msgs - 1)
                {
                    /*
                    printf("id: %d    write to: %d\n", id, id * BUFSIZE + base_offset);
                    if(debuglast != id * BUFSIZE + base_offset - 128){
                        exit(0);
                    }
                    debuglast = id * BUFSIZE + base_offset; //debug
                    */
                    
                    memcpy((void *)((char *)image_segment + (id * BUFSIZE + base_offset)), (char *)buff + sizeof(int), BUFSIZE); //Copy from recieved message into correct location in image_segment
                    remaining_bytes -= BUFSIZE;
                }
                else
                {
                    int copy_size = remaining_bytes % BUFSIZE;
                    if (copy_size == 0)
                    {
                        copy_size = BUFSIZE;
                    }

/*
                    printf("id: %d    write to: %d\n", id, id * BUFSIZE + base_offset);
                    if(debuglast != id * BUFSIZE + base_offset - 128){
                        exit(0);
                    }
                    debuglast = id * BUFSIZE + base_offset; //debug
*/

                    memcpy((void *)((char *)image_segment + (id * BUFSIZE + base_offset)), (char *)buff + sizeof(int), copy_size);
                    remaining_bytes -= copy_size;
                    base_offset += id * BUFSIZE + copy_size;
                }
            }
        }
    }

    Output.Allocate(Input.Width, Input.Height, Input.Components);

    for(int i = 0; i < Output.Width * Output.Height; i++){
        pixel px = *((pixel *)(image_segment) + i);
        Output.Rows[(int)i / Output.Width][i % Output.Width * 3] = px.r;
        Output.Rows[(int)i / Output.Width][(i % Output.Width * 3) + 1] = px.g;
        Output.Rows[(int)i / Output.Width][(i % Output.Width * 3) + 2] = px.b;
        //printf("R, G, B - %d, %d, %d\n", px.r, px.g, px.b);
    }

    //char name[200];
    //sprintf(name, "Output.jpg");
    Output.Write("Data/Output.jpg");
}


    //------------------------------------------------------------------------------

    /** This is the Slave function, the workers of this MPI application. */
    void Slave(int ID)
    {

        printf("Slave %d activated!\n", ID);
        

        char idstr[32];
        char buff[BUFSIZE + sizeof(int)];
        char *image_segment = 0;
        MPI_Status stat;

        // receive from rank 0 (master):
        // This is a blocking receive, which is typical for slaves.

        MPI_Recv(buff, sizeof(struct payload), MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
        struct payload *plptr = (struct payload *)buff;
        int num_msgs = plptr->num_msgs; //Number of packets to be recieved
        int remaining_bytes = plptr->num_pixels * 3;
        int width = 0;
        int height = 0;
        JPEG segment;
        if (plptr->magic == 0xFE)
        { //Safty check to ensure valid header message
            //printf("PROCESSING image_segment |%d|\n", plptr->cid);
            //printf("magic : %d\n", plptr->magic);
            //printf("cid : %d\n", plptr->cid);
            //printf("num_msgs : %d\n", plptr->num_msgs);
            //printf("width : %d\n", plptr->width);

            height = plptr->num_pixels / plptr->width; //Num pixels in segment/width (in pixels)
            width = plptr->width;
            image_segment = (char *)malloc(remaining_bytes); //Entire image segment

            printf("\nSlave %d -> Number incoming messages : %d\n", ID, num_msgs);

            while (remaining_bytes > 0)
            {
                MPI_Recv(buff, BUFSIZE + sizeof(int), MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
                int id = *(int *)buff;
                if(ID == 3 && 38366 < id){
                    printf("Msg id is '%d' of '%d' msg -> slave %d\n", id, num_msgs, ID);
                }
                if (id < num_msgs - 1)
                {
                    memcpy((void *)((char *)image_segment + (id * BUFSIZE)), (char *)buff + sizeof(int), BUFSIZE); //Copy from recieved message into correct location in image_segment
                    remaining_bytes -= BUFSIZE;
                }
                else
                {
                    printf("Msg id is '%d' indicates last msg -> slave %d\n", id, ID);
                    int copy_size = remaining_bytes % BUFSIZE;
                    if (copy_size == 0)
                    {
                        copy_size = BUFSIZE;
                    }
                    memcpy((void *)((char *)image_segment + (id * BUFSIZE)), (char *)buff + sizeof(int), copy_size);
                    remaining_bytes -= copy_size;
                }
            }

            printf("Slave %d Width: %d\n", ID,  width);
            printf("Slave %d Height: %d\n", ID, height);
            segment.Allocate(width, height, 3);

            char name[200];
            sprintf(name, "Data/%d.jpg", ID);
            //segment.Write(name);
            //printf(name);

            //printf("..\n");
            //DumpHex((void*)((char*)&segment.Rows[0][0]), 10);
            //printf("..\n");
            //image_segment contains the image subsection pixel data (r,g,b) array
            // height, width are vars for each subsection
            // DumpHex((void*)((char*)image_segment), BUFSIZE * 3 );
            //printf("\n");
            //filter with MD filter

            pixel *pixels = (pixel *)image_segment;
            int yl = height; //Number of rows
            int xl = width;  //Number of pixels wide

            int offset;
            int empty = 0;
            for (int i = 0; i < yl * xl; i++)
            {
                pixel npx[8];

                //cases:
                if (i == 0)
                { //1 Top left corner
                    offset = i + 1;
                    npx[0] = *(pixels + offset);

                    offset = i + xl + 1;
                    npx[1] = *(pixels + offset);

                    offset = i + xl;
                    npx[2] = *(pixels + offset);

                    empty = 5;
                }

                if (i == xl - 1)
                { //2 Top right corner
                    offset = i - 1;
                    npx[0] = *(pixels + offset);

                    offset = i + xl - 1;
                    npx[1] = *(pixels + offset);

                    offset = i + xl;
                    npx[2] = *(pixels + offset);

                    empty = 5;
                }

                if (i == (yl - 1) * xl)
                { //3 Bottom left corner
                    offset = i - xl;
                    npx[0] = *(pixels + offset);

                    offset = i - xl + 1;
                    npx[1] = *(pixels + offset);

                    offset = i + 1;
                    npx[2] = *(pixels + offset);

                    empty = 5;
                }

                if (i == yl * xl - 1)
                { //4 Bottom right corner
                    offset = i - xl - 1;
                    npx[0] = *(pixels + offset);

                    offset = i - xl;
                    npx[1] = *(pixels + offset);

                    offset = i - 1;
                    npx[2] = *(pixels + offset);

                    empty = 5;
                }

                if (i % xl == 0 && empty == 0)
                { //5 Left side
                    offset = i - xl;
                    npx[0] = *(pixels + offset);

                    offset = i - xl + 1;
                    npx[1] = *(pixels + offset);

                    offset = i + 1;
                    npx[2] = *(pixels + offset);

                    offset = i + xl + 1;
                    npx[3] = *(pixels + offset);

                    offset = i + xl;
                    npx[4] = *(pixels + offset);

                    empty = 3;
                }

                if (i < xl - 1 && empty == 0)
                { //6 Top side
                    offset = i + 1;
                    npx[0] = *(pixels + offset);

                    offset = i + xl + 1;
                    npx[1] = *(pixels + offset);

                    offset = i + xl;
                    npx[2] = *(pixels + offset);

                    offset = i - 1;
                    npx[3] = *(pixels + offset);

                    offset = i + xl - 1;
                    npx[4] = *(pixels + offset);

                    empty = 3;
                }

                if ((i + 1) % xl == 0 && empty == 0)
                { //7 Right side
                    offset = i - xl - 1;
                    npx[0] = *(pixels + offset);

                    offset = i - xl;
                    npx[1] = *(pixels + offset);

                    offset = i - 1;
                    npx[2] = *(pixels + offset);

                    offset = i + xl - 1;
                    npx[3] = *(pixels + offset);

                    offset = i + xl;
                    npx[4] = *(pixels + offset);

                    empty = 3;
                }

                if (i == (yl - 1) * xl && empty == 0)
                { //8 Bottom side
                    offset = i - xl - 1;
                    npx[0] = *(pixels + offset);

                    offset = i - xl;
                    npx[1] = *(pixels + offset);

                    offset = i - 1;
                    npx[2] = *(pixels + offset);

                    offset = i - xl + 1;
                    npx[1] = *(pixels + offset);

                    offset = i + 1;
                    npx[2] = *(pixels + offset);

                    empty = 3;
                }

                if (empty == 0)
                {
                    offset = i - xl - 1;
                    npx[0] = *(pixels + offset);

                    offset = i - xl;
                    npx[1] = *(pixels + offset);

                    offset = i - 1;
                    npx[2] = *(pixels + offset);

                    offset = i - xl + 1;
                    npx[3] = *(pixels + offset);

                    offset = i + 1;
                    npx[4] = *(pixels + offset);

                    offset = i + xl - 1;
                    npx[5] = *(pixels + offset);

                    offset = i + xl;
                    npx[6] = *(pixels + offset);

                    offset = i + xl + 1;
                    npx[7] = *(pixels + offset);
                }
                else
                {

                    for (int i = 8 - empty; i < 8; i++)
                    {
                        npx[i] = {0, 0, 0};
                    }
                }

                empty = 0;

                pixel mdpixel = determine_median((pixel *)(pixels + i), &(npx[0]));

                segment.Rows[(int)i / xl][i % xl * 3] = mdpixel.r;
                segment.Rows[(int)i / xl][(i % xl * 3) + 1] = mdpixel.g;
                segment.Rows[(int)i / xl][(i % xl * 3) + 2] = mdpixel.b;
            }
        }

        char name[200];
        sprintf(name, "Data/%d.jpg", ID);
        segment.Write(name);

        //Send back to master
        char *data_frame = (char *)malloc(BUFSIZE + sizeof(int)); //holds info to send over mpi message

        int total_num_pixels = segment.Height * segment.Width; //Total number of pixels
        int num_rows = segment.Height;
        if (ID == 1 || ID == numprocs - 1)
        {
            num_rows--; //less one row
        }
        else
        {
            num_rows -= 2; //less top and bottom row
        }

        int num_msg = ceil((double)((num_rows * segment.Width * segment.Components) / (double)BUFSIZE)); //Number of messages req to send all the rows in row_fit

        //1Dify the image
        char *tmp = (char *)malloc(total_num_pixels * segment.Components); //Declaring tmp

        for (int y = 0; y < segment.Height; y++)
        {
            for (int x = 0; x < segment.Width * segment.Components; x++)
            {
                tmp[(y * segment.Width * segment.Components) + x] = segment.Rows[y][x];
            }
        }

        int total_image_data_sent = 0; //Track bytes already sent and index tmp memory accordingly
        int tids_offset = 0;           //Offset to remove additional rows if nessesary
        if (ID != 1)
        {
            tids_offset = segment.Width * segment.Components; //Ignore first row, the last row will be removed if num_rows is down by 2 as implemented earlier...
        }

        //Create the payload stats
        struct payload pl;
        pl.magic = 0xFE;
        pl.cid = ID;
        pl.width = segment.Width;            //Number of pixels in a rows
        pl.num_pixels = pl.width * num_rows; //Number of pixels in the segment
        pl.num_msgs = num_msg;

        //and send
        memcpy((void *)data_frame, (void *)&pl, sizeof(struct payload));                        //memcpy(dest, source, number bytes to cpy)
        MPI_Send((void *)data_frame, sizeof(struct payload), MPI_CHAR, 0, TAG, MPI_COMM_WORLD); //Send the details of incoming data

        //Send the segment data
        //Send image data
        for (int k = 0; k < pl.num_msgs; k++)
        {
            //Write message number to data frame
            int *ip = (int *)data_frame; //casting so we can write first 4 bytes
            *ip = k;

            //Write pixel data to data frame
            if (k != (pl.num_msgs - 1)) //not the last msg
            {
                memcpy((void *)(char *)data_frame + sizeof(int), (void *)(tmp + total_image_data_sent + tids_offset), BUFSIZE); //Tack on the image data (128 bytes of idata)
                total_image_data_sent += BUFSIZE;
            }
            else
            { //last msg
                int remaining_bytes = (pl.num_pixels * 3) % BUFSIZE;
                if (remaining_bytes == 0)
                {
                    remaining_bytes = BUFSIZE;
                }
                memcpy((void *)(char *)data_frame + sizeof(int), (void *)(tmp + total_image_data_sent + tids_offset), remaining_bytes); //Tack on the image data
                total_image_data_sent += remaining_bytes;
            }
            MPI_Send((void *)data_frame, BUFSIZE + sizeof(int), MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
        }

        //Done sending the segment data
    }
    //------------------------------------------------------------------------------

    /** This is the entry point to the program. */
    int main(int argc, char **argv)
    {
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
        if (myid == 0)
            Master();
        else
            Slave(myid);

        // MPI programs end with MPI_Finalize
        MPI_Finalize();
        return 0;
    }
    //------------------------------------------------------------------------------