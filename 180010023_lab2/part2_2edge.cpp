#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <thread>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define SNAME_READ "/readimage16"
#define SNAME_WRITE "/writeimage16"

using namespace std;

sem_t *sem1;
sem_t *sem2;

class Image
{
public:
    int **edgeDetectionMatrix, **grayScaleMatrix, h, w, complete_rows;
    string fileName;

    Image()
    {
        h = w = 0;
    }

    void memoryFun()
    {
        edgeDetectionMatrix = new int *[h];
        grayScaleMatrix = new int *[h];
        for (int i = 0; i < h; i++)
        {
            edgeDetectionMatrix[i] = new int[w];
            grayScaleMatrix[i] = new int[w];
        }

        complete_rows = 0;
    }

    void retrieveFun()
    {
        key_t my_key = ftok("shmfile", 65);
        int shmid = shmget(my_key, sizeof(char), 0666 | IPC_CREAT);
        char *pixel = (char *)shmat(shmid, (void *)0, 0);

        sem_wait(sem1);
        h = atoi(pixel);
        sem_post(sem2);

        sem_wait(sem1);
        w = atoi(pixel);
        sem_post(sem2);

        shmdt(pixel);
        shmctl(shmid, IPC_RMID, NULL);
    }

    void detectEdgesForSpecificRow(int index)
    {

        int filter[3][3] = {
            {1, 2, 1},
            {2, -13, 2},
            {1, 2, 1}};

        for (int j = 0; j < w; j++)
        {
            int pixel = 0;
            for (int x = 0; x < 3; x++)
            {
                for (int y = 0; y < 3; y++)
                {
                    if (index + x >= h || j + y >= w)
                        continue;
                    pixel += grayScaleMatrix[index + x][j + y] * filter[x][y];
                }
            }
            edgeDetectionMatrix[index][j] = max(min(pixel, 255), 0);
        }
    }

    void EdgeDetection()
    {

        int filter[3][3] = {
            {1, 2, 1},
            {2, -13, 2},
            {1, 2, 1}};

        key_t my_key = ftok("shmfile_image", 65);
        int shmid = shmget(my_key, w * h * sizeof(char) + 1, 0666 | IPC_CREAT);

        char *pixel = (char *)shmat(shmid, (void *)0, 0);

        for (int i = 0; i < h; i++)
        {
            sem_wait(sem1);

            for (int j = 0; j < w; j++)
            {
                grayScaleMatrix[i][j] = ((int)(*(pixel + i * w + j))) + 128;
            }

            if (i >= 2 && i < h)
            {
                detectEdgesForSpecificRow(i - 2);
            }
        }

        detectEdgesForSpecificRow(h - 2);
        detectEdgesForSpecificRow(h - 1);

        shmdt(pixel);
        shmctl(shmid, IPC_RMID, NULL);

        writeToFile(fileName);
    }

    void writeToFile(string filename)
    {
        //writeToFile
        ofstream fpout(filename);
        int filter[3][3] = {
            {1, 2, 1},
            {2, -13, 2},
            {1, 2, 1}};

        fpout << "P2\n";
        fpout << w << " " << h << "\n";
        fpout << 255 << "\n";

        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                fpout << edgeDetectionMatrix[i][j] << "\n";
            }
        }

        fpout.close();
    }
};

int main(int argc, char **argv)
{

    sem2 = sem_open(SNAME_READ, 0);
    sem1 = sem_open(SNAME_WRITE, 0);

    Image image;

    image.fileName = argv[2];
    image.retrieveFun();
    image.memoryFun();

    image.EdgeDetection();

    sem_destroy(sem1);
    sem_destroy(sem2);

    return 0;
}