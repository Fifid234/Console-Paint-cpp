#include <Windows.h>
#include <vector>
#include <string>

using std::wstring;
using std::vector;

struct Buffer
{
    COORD beg;
    COORD end;
    short col;
    short row;
    vector<CHAR_INFO> arr;

    Buffer() {}
    Buffer (COORD beg, COORD end) :
    beg(beg), end(end)
    {
        col = end.X - beg.X + 1;
        col = col < 0 ? col *= -1 : col;

        row = end.Y - beg.Y + 1;
        row = row < 0 ? row *= -1 : row;

        arr.assign(col * row, {L' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE});
    }
};

struct Button
{
    COORD beg;
    COORD end;
    short col;
    short row = 3;
    wstring text;
    vector<CHAR_INFO> arr;

    Button (COORD beg, wstring text) :
    beg(beg), text(text)
    {
        col = text.length() + 2;
        end = { (short)(beg.X + col - 1), (short)(beg.Y + row - 1) };

        arr.assign(row * col, {L' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE});

        arr[0].Char.UnicodeChar = L'╔';
        arr[col - 1].Char.UnicodeChar = L'╗';
        arr[col].Char.UnicodeChar = L'║';
        arr[col*2 - 1].Char.UnicodeChar = L'║';
        arr[(row - 1) * col].Char.UnicodeChar = L'╚';
        arr[row * col - 1].Char.UnicodeChar = L'╝';

        for (size_t i = 0; i < row; i++)
        {
            for (size_t j = 1; j < col - 1; j++)
            {
                if (i == 0 || i == row - 1)
                {
                    arr[i * col + j].Char.UnicodeChar = L'═';
                }
            }
        }

        for (size_t i = 0; i < text.length(); i++)
        {
            arr[col + 1 + i].Char.UnicodeChar = text[i];
        }
    }
};

struct Brush
{
    short size;
    CHAR_INFO info;

    Brush() : 
    info({L' ', BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY}), size(3) {}
};


void addBufferUI();
void initValues();
void drawBuffer();
bool getInputHandle();
void brushPreview();

HANDLE hOut, hIn;
COORD begCoord {0, 0}, endCoord;

const short menuYCoord = 7;

Buffer canvasBuffer = Buffer();
Brush mainBrush = Brush();

vector<Button> buttons = { 
    Button({1, 1}, L"Clear"), 
    Button({9, 1}, L" Red "), Button({17, 1}, L"Green"), Button({25, 1}, L" Blue "), Button({34, 1}, L"White"), Button({42, 1}, L"Yellow"),
    Button({9, 4}, L"Brown"), Button({17, 4}, L" Gray"), Button({25, 4}, L"Purple"), Button({34, 4}, L" Pink"), Button({42, 4}, L"Orange"),
    Button({53, 1}, L"1"), Button({57, 1}, L"3"), Button({61, 1}, L"5"), Button({65, 1}, L"7"), Button({69, 1}, L"9"),
};

COORD brushPreviewCoord;

int main()
{
    initValues();
    addBufferUI();
    drawBuffer();

    while (true)
    {
        if (getInputHandle())
        {
            drawBuffer();
            brushPreview();
        }
        Sleep(0);
    }
}

void addBufferUI()
{
    for (size_t i = 0; i < canvasBuffer.col; i++)
    {
        canvasBuffer.arr[i + menuYCoord * canvasBuffer.col].Char.UnicodeChar = L'=';
    }

    for (auto &button : buttons)
    {
        for (size_t i = 0; i < button.row; i++)
        {
            for (size_t j = 0; j < button.col; j++)
            {
                canvasBuffer.arr[(button.beg.Y + i) * canvasBuffer.col + button.beg.X + j] = button.arr[i * button.col + j];
            }
        }
    }
}

void updateCoordinates() 
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    
    endCoord.X = csbi.srWindow.Right;
    endCoord.Y = csbi.srWindow.Bottom;

    canvasBuffer = Buffer(begCoord, endCoord);
}

void initValues() 
{
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn = GetStdHandle(STD_INPUT_HANDLE);

    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hOut, &cci);
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cci);

    DWORD mode;
    GetConsoleMode(hIn, &mode);
    mode |= ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    SetConsoleMode(hIn, mode);

    updateCoordinates();
}

void drawBuffer() 
{
    if (canvasBuffer.arr.empty()) return;

    SMALL_RECT writeRegion = {
        canvasBuffer.beg.X,
        canvasBuffer.beg.Y,
        endCoord.X,
        endCoord.Y
    };

    COORD bufferSize = { canvasBuffer.col, canvasBuffer.row };

    WriteConsoleOutputW(hOut, canvasBuffer.arr.data(), bufferSize, {0, 0}, &writeRegion);
}

int isCursorInButton(const Button& button, const COORD& cursorPos)
{
    return cursorPos.X >= button.beg.X && cursorPos.X <= button.end.X &&
           cursorPos.Y >= button.beg.Y && cursorPos.Y <= button.end.Y;
}

bool getInputHandle()
{
    DWORD numEvents = 0;
    GetNumberOfConsoleInputEvents(hIn, &numEvents);
    if (numEvents == 0) return false;

    vector<INPUT_RECORD> rec(numEvents);
    DWORD read;
    ReadConsoleInputW(hIn, rec.data(), numEvents, &read);

    for (auto& event : rec)
    {
        if (event.EventType == MOUSE_EVENT) 
        { 
            MOUSE_EVENT_RECORD& me = event.Event.MouseEvent;

            if (me.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
            {
                if (me.dwMousePosition.Y <= menuYCoord)
                {
                    for (size_t i = 0; i < buttons.size(); i++)
                    {
                        if (isCursorInButton(buttons[i], me.dwMousePosition))
                        {
                            switch (i)
                            {
                                case 0:
                                    canvasBuffer.arr.assign(canvasBuffer.col * canvasBuffer.row, {L' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE});
                                    addBufferUI();
                                    break;
                                case 1:
                                    mainBrush.info.Attributes = BACKGROUND_RED | BACKGROUND_INTENSITY;
                                    break;
                                case 2:
                                    mainBrush.info.Attributes = BACKGROUND_GREEN | BACKGROUND_INTENSITY;
                                    break;
                                case 3:
                                    mainBrush.info.Attributes = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
                                    break;
                                case 4:
                                    mainBrush.info.Attributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
                                    break;
                                case 5:
                                    mainBrush.info.Attributes = 224;
                                    break;
                                case 6:
                                    mainBrush.info.Attributes = 96;
                                    break;
                                case 7:
                                    mainBrush.info.Attributes = 112;
                                    break;
                                case 8:
                                    mainBrush.info.Attributes = 80;
                                    break;
                                case 9:
                                    mainBrush.info.Attributes = 208;
                                    break;
                                case 10:
                                    mainBrush.info.Attributes = 224;
                                    break;
                                case 11:
                                    mainBrush.size = 1;
                                    break;
                                case 12:
                                    mainBrush.size = 3;
                                    break;
                                case 13:
                                    mainBrush.size = 5;
                                    break;
                                case 14:
                                    mainBrush.size = 7;
                                    break;
                                case 15:
                                    mainBrush.size = 9;
                                    break;
                            }
                            break;
                        }
                    }
                }
                else
                {
                    for (short i = -(mainBrush.size - 1)/2; i <= (mainBrush.size - 1)/2; i++)
                    {
                        for (short j = -(mainBrush.size - 1)/2; j <= (mainBrush.size - 1)/2; j++)
                        {
                            short localX = me.dwMousePosition.X + j;
                            short localY = me.dwMousePosition.Y + i;
    
                            if (localY <= menuYCoord ||
                                localY > endCoord.Y || 
                                localX < begCoord.X ||
                                localX >= endCoord.X) 
                                continue;
    
                            canvasBuffer.arr[localY * canvasBuffer.col + localX] = mainBrush.info;  
                        }
                    }
                }
            }
            if (me.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
            {
                for (short i = -(mainBrush.size - 1)/2; i <= (mainBrush.size - 1)/2; i++)
                {
                    for (short j = -(mainBrush.size - 1)/2; j <= (mainBrush.size - 1)/2; j++)
                    {
                        short localX = me.dwMousePosition.X + j;
                        short localY = me.dwMousePosition.Y + i;

                        if (localY <= menuYCoord ||
                            localY > endCoord.Y || 
                            localX < begCoord.X ||
                            localX >= endCoord.X) 
                            continue;

                        canvasBuffer.arr[localY * canvasBuffer.col + localX] = {L' ', 0};
                    }
                }
            }
            brushPreviewCoord = me.dwMousePosition;
        }
        else if (event.EventType == WINDOW_BUFFER_SIZE_EVENT) 
        {
            updateCoordinates();
            if (endCoord.Y >= menuYCoord)
                addBufferUI();
        }
    }

    return true;
}

void brushPreview()
{
    for (short i = -(mainBrush.size - 1)/2; i <= (mainBrush.size - 1)/2; i++)
    {
        for (short j = -(mainBrush.size - 1)/2; j <= (mainBrush.size - 1)/2; j++)
        {
            short localX = brushPreviewCoord.X + j;
            short localY = brushPreviewCoord.Y + i;

            if (localY <= menuYCoord ||
                localY > endCoord.Y || 
                localX < begCoord.X ||
                localX >= endCoord.X) 
                continue;

            SMALL_RECT rect = { localX, localY, localX, localY };
            WriteConsoleOutputW(hOut, &mainBrush.info, {1, 1}, {0, 0}, &rect);
        }
    }
}