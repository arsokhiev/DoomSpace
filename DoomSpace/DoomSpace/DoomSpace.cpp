#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <utility>

#include <stdio.h>
#include <Windows.h>

int nScreenWidth = 120; // Ширина консольного окна
int nScreenHeight = 40; // Высота консольного окна
int nMapHeight = 16; // Высота игрового поля
int nMapWidth = 16; // Ширина игрового поля

float fPlayerX = 1.0f; // Координата игрока по оси X
float fPlayerY = 1.0f; // Координата игрока по оси Y
float fPlayerA = 0.0f; // Направление игрока

float fFOV = 3.14159f / 3.0f; // Угол обзора (поле видимости)
float fDepth = 30.0f; // Максимальная дистанция обзора

int main()
{
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight + 1]; // Массив для записи в буфер
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL); // Буфер экрана
    SetConsoleActiveScreenBuffer(hConsole); // Настройка консоли
    DWORD dwBytesWritten = 0; // Для дебага

    screen[nScreenWidth * nScreenHeight] = '\0'; // Последний символ - окончание строки
    WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, {0, 0}, &dwBytesWritten); // Запись в буфер

    std::wstring map;
    map += L"#########.......";
    map += L"#...............";
    map += L"#.......########";
    map += L"#..............#";
    map += L"#......##......#";
    map += L"#......##......#";
    map += L"#..............#";
    map += L"###............#";
    map += L"##.............#";
    map += L"#......####..###";
    map += L"#......#.......#";
    map += L"#......#.......#";
    map += L"#..............#";
    map += L"#......#########";
    map += L"#..............#";
    map += L"################";

    auto tp1 = std::chrono::system_clock::now(); // Переменные для подсчета
    auto tp2 = std::chrono::system_clock::now(); // пройденного времени

    while (1) // Игровой цикл
    {
        tp2 = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
        {
            fPlayerA -= (1.5f) * fElapsedTime; // Клавишей "A" поворачиваем по часовой стрелке
        }

        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
        {
            fPlayerA += (1.5f) * fElapsedTime; // Клавишей "D" поворачиваем против часовой стрелки
        }

        if (GetAsyncKeyState((unsigned short)'W') & 0x8000) // Клавишей "W" идём вперёд
        {
            fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;

            if (map.c_str()[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                // Если столкнулись со стеной, но откатываем шаг
                fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }

        if (GetAsyncKeyState((unsigned short)'S') & 0x8000) // Клавишей "S" идём назад
        {
            fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            if (map.c_str()[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                // Если столкнулись со стеной, но откатываем шаг
                fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        
        for (int x = 0; x < nScreenWidth; x++)
        {
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV; // Направление луча

            float fDistanceToWall = 0.0f; // Расстояние до препятствия в направлении fRayAngle
            
            bool bHitWall = false; // Достигнул ли луч стенку
            bool bBoundary = false;	// Идет ли луч в ребро

            float fEyeX = sinf(fRayAngle); // Координаты единичного вектора fRayAngle
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth) // Пока не столкнулись со стеной
            {
                // Или не вышли за радиус видимости
                fDistanceToWall += 0.1f;

                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall); // Точка на игровом поле
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall); // в которую попал луч

                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    // Если мы вышли за карту, то дальше смотреть нет смысла - фиксируем соударение на расстоянии видимости
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else
                {
                    if (map.c_str()[nTestY * nMapWidth + nTestX] == '#')
                    {
                        // Если встретили стену, то заканчиваем цикл
                        bHitWall = true;

                        std::vector <std::pair <float, float>> p;

                        for (int tx = 0; tx < 2; tx++)
                        {
                            // Проходим по всем 4м рёбрам
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vx = (float)nTestX + tx - fPlayerX; // Координаты вектора,
                                float vy = (float)nTestY + ty - fPlayerY; // ведущего из наблюдателя в ребро
                                float d = sqrt(vx*vx + vy*vy); // Модуль этого вектора
                                float dot = (fEyeX*vx / d) + (fEyeY*vy / d); // Скалярное произведение (единичных векторов)
                                p.push_back(std::make_pair(d, dot)); // Сохраняем результат в массив
                            }
                        }
                        // Мы будем выводить два ближайших ребра, поэтому сортируем их по модулю вектора ребра
                        sort(p.begin(), p.end(), [](const std::pair <float, float> &left, const std::pair <float, float> &right) {return left.first < right.first; });
 
                        float fBound = 0.005; // Угол, при котором начинаем различать ребро.
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        //if (acos(p.at(2).second) < fBound) bBoundary = true;
                    }
                }
            }

            //Вычисляем координаты начала и конца стенки по формулам (1) и (2)
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            short nShade;

            if (fDistanceToWall <= fDepth / 3.0f)       nShade = 0x2588; // Если стенка близко, то рисуем 
            else if (fDistanceToWall < fDepth / 2.0f)   nShade = 0x2593; // светлую полоску
            else if (fDistanceToWall < fDepth / 1.5f)   nShade = 0x2592; // Для отдалённых участков 
            else if (fDistanceToWall < fDepth)          nShade = 0x2591; // рисуем более темную
            else                                        nShade = ' ';

            if (bBoundary)
            {
                nShade = ' ';
            }
            
            for (int y = 0; y < nScreenHeight; y++) // При заданном X проходим по всем Y
            {
                // В этом цикле рисуется вертикальная полоска
                if (y <= nCeiling)
                {
                    screen[y * nScreenWidth + x] = ' ';
                }
                else if (y > nCeiling && y <= nFloor)
                {
                    screen[y * nScreenWidth + x] = nShade;
                }
                else
                {
                    // То же самое с полом - более близкие части рисуем более заметными символами
                    float b = 1.0f - ((float)y - nScreenHeight / 2.0) / ((float)nScreenHeight / 2.0);
                    
                    if (b < 0.25)       nShade = '#';
                    else if (b < 0.5)   nShade = 'x';
                    else if (b < 0.75)  nShade = '~';
                    else if (b < 0.9)   nShade = '-';
                    else                nShade = ' ';

                    screen[y * nScreenWidth + x] = nShade;
                }
            }
        }
        
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);

        // Display Map
        for (int nx = 0; nx < nMapWidth; nx++)
            for (int ny = 0; ny < nMapWidth; ny++)
            {
                screen[(ny+1)*nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        screen[((int)fPlayerX+1) * nScreenWidth + (int)fPlayerY] = 'P';

        // Display Frame
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }
    return 0;
}