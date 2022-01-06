#include <iostream>
#include <fstream>
#include <string.h>
#include <thread>
#include <unistd.h>

using namespace std;

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
        }
    }

    void detectEdgesForSpecificRow(int index)
    {

        int filter[3][3] = {
            {1, 2, 1},
            {2, -13, 2},
            {1, 2, 1}};

        for (int j = 0; j < w; ++j)
        {
            int pixel = 0;
            for (int x = 0; x < 3; ++x)
            {
                for (int y = 0; y < 3; ++y)
                {
                    if (index + x >= h || j + y >= w)
                        continue;
                    pixel += grayScaleMatrix[index + x][j + y] * filter[x][y];
                }
            }
            edgeDetectionMatrix[index][j] = max(min(pixel, 255), 0);
        }
    }

    void pipes(int i = 0)
    {

        int p[2], pid, nbytes;
        if (pipe(p) < 0)
        {
            cout << "Pipe Error\n";
            exit(0);
        }
        if ((pid = fork()) > 0)
        {
            char *pixel_chars = new char[w + 1];
            for (int i = 0; i < h; i++)
            {
                for (int j = 0; j < w; j++)
                {
                    string buff = "";
                    buff += (char)(((int)(0.3 * r[i][j] + 0.59 * g[i][j] + 0.11 * b[i][j])) + 128);
                    strcpy(pixel_chars + j, buff.c_str());
                    // cout
                    //     << "((int)(0.3 * r[i][j] + 0.59 * g[i][j] + 0.11 * b[i][j])) : " << ((int)(0.3 * r[i][j] + 0.59 * g[i][j] + 0.11 * b[i][j])) << "\n";
                    // cout
                    //     << "(int)(*(pixel_chars + j)) : " << (int)(*(pixel + j)) << "\n";
                    // cout << "int val : " << ((int)(*(pixel_chars + j))) + 128 << "\n";
                }
                write(p[1], pixel_chars, w * sizeof(char) + 1);
            }
        }
        else
        {
            int index = 0;
            char buff2[w * sizeof(char) + 1];
            while ((nbytes = read(p[0], buff2, w * sizeof(char) + 1)) > 0)
            {
                for (int j = 0; j < w; j++)
                {
                    grayScaleMatrix[index][j] = ((int)(*(buff2 + j))) + 128;
                }
                if (index >= 2 && index < h)
                {
                    detectEdgesForSpecificRow(index - 2);
                }
                index++;
                if (index >= h)
                    break;
            }

            detectEdgesForSpecificRow(h - 2);
            detectEdgesForSpecificRow(h - 1);
            writePPM(fileName);
        }
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

    image.pipes();

    return 0;
}