#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

int bmpHeight, bmpWidth;
int LineByte;
unsigned char *pBmpBuf1;

int value[500][1000]; // 存储图像的像素值
int seven2[89];       // 数据内容 左6*7位+中间分隔符5位01010+右6*7位
int barcode[12];

int start_mark;
int end_mark;
int BandWpix;
unsigned char *pBmpBuf;
int biBitCount;

void readBmp()
{
    BITMAPINFOHEADER info;
    errno_t err;
    char path[60] = "";

    printf_s("请输入要读取的bmp文件路径：\n");
    scanf_s("%s", path, 60);
    FILE *fp = NULL;
    err = fopen_s(&fp, path, "rb+");
    if (err)
    {
        printf("打开文件失败");
        system("pause");
        exit(0);
    }
    fseek(fp, sizeof(BITMAPFILEHEADER), 0); // 跳过文件头
    fread(&info, 40, 1, fp);                // 读取位图信息头
    bmpHeight = info.biHeight;
    bmpWidth = info.biWidth;
    biBitCount = info.biBitCount;
    printf("Height:%d,Width:%d\n", bmpHeight, bmpWidth);
    fseek(fp, sizeof(RGBQUAD), 1);                      // 跳过调色板
    LineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4; // 保证每一行字节数都为4的整数倍
    pBmpBuf = new unsigned char[LineByte * bmpHeight];
    fread(pBmpBuf, static_cast<size_t>(LineByte) * bmpHeight, 1, fp); // 读取位图数据
    fclose(fp);
}

void Rgb2Gray()
{
    unsigned char *pb1;
    for (int i = 0; i < bmpHeight; ++i)
    {
        for (int j = 0; j < bmpWidth; ++j)
        {
            pb1 = pBmpBuf + i * LineByte + j * 3;
            value[i][j] = (*(pb1) + *(pb1 + 1) + *(pb1 + 2)) / 3;
            if (value[i][j] > 255 / 2)
                value[i][j] = 1;
            else
                value[i][j] = 0;
        }
    }
}

void debug()
{
    for (int j = bmpHeight; j >= 0; --j) // 输出数组内有效数组的存储情况
    {
        for (int i = start_mark + BandWpix * 3; i <= end_mark - BandWpix * 3; ++i)
        {
            printf("%c", value[j][i] ? '`' : '#');
        }
        printf("\n");
    }
}

void Edge_detection()
{
    int black_count = 0;
    for (int j = 0; j < bmpWidth; ++j) // 找start_mark
    {
        for (int i = 0; i < bmpHeight; ++i)
        {
            if (value[i][j] == 0)
            {
                ++black_count;
            }
        }
        if (black_count > bmpHeight / 5 * 2) // 超过40%区域检测结果则认为边界起始有效
        {
            start_mark = j;
            break;
        }
        black_count = 0;
    }

    for (int j = bmpWidth - 1; j >= 0; --j) // 找end_mark
    {
        for (int i = 0; i < bmpHeight; ++i)
        {
            if (value[i][j] == 0)
            {
                ++black_count;
            }
        }
        if (black_count > bmpHeight / 5 * 2)
        {
            end_mark = j;
            break;
        }
        black_count = 0;
    }
}

void Decode1()
{
    int k = 0;
    int black_count = 0, white_count = 0;
    for (int i = start_mark + BandWpix * 3; i <= end_mark - BandWpix * 3; i += +BandWpix, ++k)
    {
        for (int j = 0; j < bmpHeight; ++j)
        {
            if (value[j][i] == 0)
            {
                ++black_count;
            }
            else
            {
                ++white_count;
            }
        }
        seven2[k] = black_count >= white_count ? 1 : 0;
        white_count = 0;
        black_count = 0;
        printf("%d ", seven2[k]); // 输出第一次解码结果
    }
}

void Decode2()
{
    int A[10] = {1101, 11001, 10011, 111101, 100011, 110001, 101111, 111011, 110111, 1011};                 // 奇性字符A
    int B[10] = {100111, 110011, 11011, 100001, 11101, 111001, 101, 10001, 1001, 10111};                    // 偶性字符B
    int C[10] = {1110010, 1100110, 1101100, 1000010, 1011100, 1001110, 1010000, 1000100, 1001000, 1110100}; // 偶性字符C
    for (int i = 0, j = 0, sum = 0; j < 12; i += 7, ++j, sum = 0)
    {
        if (i == 42)
        {
            i += 5;
            printf("||,");
        }
        sum += seven2[i] * 1000000;
        sum += seven2[i + 1] * 100000;
        sum += seven2[i + 2] * 10000;
        sum += seven2[i + 3] * 1000;
        sum += seven2[i + 4] * 100;
        sum += seven2[i + 5] * 10;
        sum += seven2[i + 6] * 1;
        printf("%07d,", sum);
        for (int k = 0; k < 10; ++k) // 把解码结果放在十位，奇偶标志放在个位
        {
            if (sum == A[k])
            {
                barcode[j] = k * 10 + 1;
            }
            else if (sum == B[k])
            {
                barcode[j] = k * 10;
            }
            else if (sum == C[k])
            {
                barcode[j] = k;
            }
        }
    }
}

void Decode3(int barcode[])
{
    int odevity;
    int D[10] = {111111, 110100, 110010, 110001, 101100, 100110, 100011, 101010, 101001, 100101};
    for (int i = 0; i < 12; ++i)
    {
        printf("%d,", i < 6 ? barcode[i] / 10 : barcode[i]);
    }

    printf("\n\n奇偶性：\n");
    for (int i = 0; i < 6; ++i)
    {
        printf("%c,", barcode[i] % 10 ? 'A' : 'B');
    }

    odevity = 0;
    odevity += barcode[0] % 10 * 100000; // 取出数据中的奇偶标识位
    odevity += barcode[1] % 10 * 10000;
    odevity += barcode[2] % 10 * 1000;
    odevity += barcode[3] % 10 * 100;
    odevity += barcode[4] % 10 * 10;
    odevity += barcode[5] % 10 * 1;

    for (int i = 0; i < 10; ++i)
    {
        if (odevity == D[i])
        {
            printf("\n\n条形码结果为：\n%d-", i);
        }
    }

    for (int i = 0; i < 6; ++i)
    {
        barcode[i] = barcode[i] / 10; // 剔除数据中的奇偶标识位
    }

    for (int i = 0; i < 12; ++i) // 输出条形码结果
    {
        if (i == 6)
        {
            printf("-");
        }
        printf("%d", barcode[i]);
    }
}

int main(int argc, char *argv[])
{
    readBmp();
    Rgb2Gray();                                                    // 二值化处理
    Edge_detection();                                              // 边缘检测，减少无效数据处理。
    BandWpix = (end_mark - start_mark + 1) / (12 * 7 + 3 + 3 + 5); // 计算单位像素宽度

    // debug();

    printf("\n条形码的单位像素宽度：%d\n条形码起始位置：%d\n条形码结束位置：%d", BandWpix, start_mark, end_mark);
    printf("\n\n第一次解码:\n");
    Decode1();

    printf("\n\n第二次解码:\n");
    Decode2();

    printf("\n\n第三次解码：\n");
    Decode3(barcode); // 前置码解码

    system("pause");
    return 0;
}