#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <thread>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>

#define SNAME_READ "/readimage16"
#define SNAME_WRITE "/writeimage16"

using namespace std;

sem_t *sem1;
sem_t *sem2;

class Image
{
public:
    int **grayScaleMatrix, **r, **g, **b, **edgeDetectionMatrix, h, w;
    string fileName;

    Image()
    {
        h = w = 0;
    }

    Image(string fileName)
    {
        this->fileName = fileName;
        getPPM(fileName);
    }

    void getPPM(string fileName)
    {

        // Get file pointer
        ifstream fpin(fileName);
        string first_line;
        int val;

        // Read header
        fpin >> first_line;
        fpin >> this->w;
        fpin >> this->h;
        fpin >> val;

        r = new int *[h];
        g = new int *[h];
        b = new int *[h];
        grayScaleMatrix = new int *[h];
        edgeDetectionMatrix = new int *[h];
        for (int i = 0; i < h; i++)
        {
            r[i] = new int[w];
            g[i] = new int[w];
            b[i] = new int[w];
            grayScaleMatrix[i] = new int[w];
            edgeDetectionMatrix[i] = new int[w];
        }

        // Read pixel values
        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                fpin >> r[i][j] >> g[i][j] >> b[i][j];
            }
        }
        fpin.close();
    }

    void sendDimensions()
    {
        key_t my_key = ftok("shmfile", 65);
        int shmid = shmget(my_key, sizeof(char), 0666 | IPC_CREAT);
        char *pixel = (char *)shmat(shmid, (void *)0, 0);
        string h_str = to_string(h), w_str = to_string(w);

        sem_wait(sem2);
        strcpy(pixel, h_str.c_str());
        sem_post(sem1);

        sem_wait(sem2);
        strcpy(pixel, w_str.c_str());
        sem_post(sem1);

        shmdt(pixel);
    }

    void grayScale()
    {
        key_t my_key = ftok("shmfile_image", 65);
        int shmid = shmget(my_key, w * h * sizeof(char) + 1, 0666 | IPC_CREAT);
        char *pixel = (char *)shmat(shmid, (void *)0, 0);

        for (int i = 0; i < h; ++i)
        {
            for (int j = 0; j < w; ++j)
            {
                string row = "";
                row += (char)(((int)(0.3 * r[i][j] + 0.59 * g[i][j] + 0.11 * b[i][j])) + 128);
                strcpy(pixel + i * w + j, row.c_str());
            }

            sem_post(sem1);
        }

        shmdt(pixel);
    }
};

int main(int argc, char **argv)
{

    sem1 = sem_open(SNAME_WRITE, O_CREAT, 0644, 0);
    sem2 = sem_open(SNAME_READ, O_CREAT, 0644, 1);

    Image image(argv[1]);
    image.fileName = argv[2];
    image.sendDimensions();

    image.grayScale();

    return 0;
}