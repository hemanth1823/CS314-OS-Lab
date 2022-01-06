#include <iostream>
#include <fstream>
#include <string.h>
#include <thread>

using namespace std;

int pixels = 0;

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

        ifstream fpin(fileName);
        string first_line;
        int val;

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

        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                fpin >> r[i][j] >> g[i][j] >> b[i][j];
            }
        }
        fpin.close();
    }

    void grayScale()
    {
        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                grayScaleMatrix[i][j] = 0.3 * r[i][j] + 0.59 * g[i][j] + 0.11 * b[i][j];
            }
            pixels++;
        }
    }

    void EdgeDetection()
    {
        int filter[3][3] = {
            {1, 2, 1},
            {2, -13, 2},
            {1, 2, 1}};

        for (int i = 0; i < h; i++)
        {
            while (pixels < i + 3 && pixels < h)
                ;
            for (int j = 0; j < w; j++)
            {
                int pixel = 0;
                for (int k = 0; k < 3; k++)
                {
                    for (int l = 0; l < 3; l++)
                    {
                        if (i + k >= h || j + l >= w)
                            continue;
                        pixel += grayScaleMatrix[i + k][j + l] * filter[k][l];
                    }
                }
                edgeDetectionMatrix[i][j] = max(min(pixel, 255), 0);
            }
        }
        writePPM(fileName);
    }

    void writePPM(string filename)
    {
        ofstream fout(filename);

        fout << "P2\n";
        fout << w << " " << h << "\n";
        fout << 255 << "\n";
        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                fout << edgeDetectionMatrix[i][j] << "\n";
            }
        }
        fout.close();
    }
};

int main(int argc, char **argv)
{
    Image image(argv[1]);
    image.fileName = argv[2];

    thread gray_thread(&Image::grayScale, &image);
    thread edge_thread(&Image::EdgeDetection, &image);

    gray_thread.join();
    edge_thread.join();

    return 0;
}