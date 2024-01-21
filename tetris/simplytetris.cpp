#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cstring>
#pragma comment(lib, "winmm.lib")

using namespace std;

const int field_width = 25; 
const int field_height = 30; 
const int screen_width = field_width * 2;
const int screen_height = field_height;

const char c_figure = 219;
const char c_field = 176;
const char c_figure_down = 178;

int score = 0;
int speed = 10;
int level = 1;

bool isMusicPlaying1 = false;
bool isMusicPlaying2 = false;
bool isMusicPlaying3 = false;

typedef char TScreenMap[screen_height][screen_width];
typedef char TFieldMap[field_height][field_width];

char* shapeArray[] = {
    (char*)".....**..**.....", // квадрат
    (char*)"....****........", // палка
    (char*)"....***..*......", // Т
    (char*)"....***.*.......", // L
    (char*)"*...***.........", // J
    (char*)".....**.**......", // Z
    (char*)"**...**........." }; // S

const int shapeArrayCounter = sizeof(shapeArray) / sizeof(shapeArray[0]);

const int shape_width = 4;
const int shape_height = 4;
typedef char TShape[shape_height][shape_width];

void SetCurPos(int position_figure_x, int position_figure_y) {
    COORD coord;
    coord.X = position_figure_x;
    coord.Y = position_figure_y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

class TScreen {
public:
    TScreenMap scr;

    void SetEnd() {
        scr[screen_height - 1][screen_width - 1] = '\0';
    }

    TScreen() {
        Clear();
    }

    void Clear() {
        memset(scr, '.', sizeof(scr));
    }

    void Show() {
        SetCurPos(0, 0);
        SetEnd();
        cout << scr[0];
    }
};

class TField {
public:
    TFieldMap field;  // хранит игровое поле
    TField() {
        Clear();
    }
    void Clear() {
        memset(field, c_field, sizeof(field));  // заполняет игровое поле пустыми клетками
    }
    void Put(TScreenMap& scr);  // рисует игровое поле на экранном буфере (х2)
    void Burning();
};

class TFigure {
    int position_figure_x, position_figure_y;
    TShape vid;
    char turn;
    COORD coord[shape_width * shape_height];  // координаты для каждой клетки объекта
    int coordinateCount;  // количество координат объекта
    TField* field = nullptr;
public:
    TFigure() {
        memset(this, 0, sizeof(*this));
    }
    void Shape(const char* _vid) {
        int row_index = 0;
        for (row_index = 0; row_index < shape_height; ++row_index) {
            strncpy_s(vid[row_index], _countof(vid[row_index]), &_vid[row_index * shape_width], _TRUNCATE);
        }
    }
    void FieldSet(TField* _field) {
        field = _field;
    }
    void Pos(int _x, int _y) {
        position_figure_x = _x;
        position_figure_y = _y;
        CalcCoord();
    }
    char TurnGet() {
        return turn;
    }
    void TurnSet(char _turn);

    void Put(TScreenMap& scr);
    void Put(TFieldMap& field);
    bool Move(int xd, int dy);
    int Check();  // вышла ли фигура за границы экрана
private:
    void CalcCoord();
};

class TGame {
    TScreen screen;
    TField field;
    TFigure figure;
public:
    TGame();
    void PlayerControl();
    void Move();  // фигурка сама падает
    void Show();
};

TGame::TGame() {
    figure.FieldSet(&field);  // передаем фигуре адрес игр. поля
    figure.Shape(shapeArray[rand() % shapeArrayCounter]);
    figure.Pos(field_width / 2 - shape_width / 2, 0);
}

void TGame::PlayerControl() {
    static int trn = 0;
    if (GetKeyState('W') < 0) {
        ++trn;
    }
    if (trn == 1) {
        figure.TurnSet(figure.TurnGet() + 1), ++trn;
    }

    if (GetKeyState('W') >= 0) {
        trn = 0;
    }

    if (GetKeyState('S') < 0)
        figure.Move(0, 1);
    if (GetKeyState('A') < 0)
        figure.Move(-1, 0);
    if (GetKeyState('D') < 0)
        figure.Move(1, 0);
}

void TGame::Show() {
    screen.Clear();
    field.Put(screen.scr);
    figure.Put(screen.scr);
    screen.Show();
}

void TGame::Move() {
    static int tick = 0;
    ++tick;
    if (tick >= speed) {
        if (!figure.Move(0, 1)) {  // новая фигура
            figure.Put(field.field);
            figure.Shape(shapeArray[rand() % shapeArrayCounter]);
            figure.Pos(field_width / 2 - shape_width / 2, 0);
            if (figure.Check() > 0) {
                speed = 10;
                score = 0;
                field.Clear();
            }
        }
        field.Burning();
        tick = 0;
    }

}

void TFigure::Put(TScreenMap& scr) {
    for (int index = 0; index < coordinateCount; ++index) {
        scr[coord[index].Y][coord[index].X * 2] = scr[coord[index].Y][coord[index].X * 2 + 1] = c_figure;
    }
}

void TFigure::Put(TFieldMap& field) { // заполняем игровое поле по коорд. фигуры
    for (int index = 0; index < coordinateCount; ++index) {
        field[coord[index].Y][coord[index].X] = c_figure_down;
    }
}

void TFigure::TurnSet(char _turn) {
    int old_turn = turn;
    turn = (_turn > 3 ? 0 : (_turn < 0 ? 3 : _turn)); // поворот
    int check = Check();
    if (check == 0) {
        return;
    }
    if (check == 1) {
        int xx = position_figure_x;
        int k = (position_figure_x > (field_width / 2) ? -1 : +1);
        for (int index = 1; index < 3; ++index) {
            position_figure_x += k;
            if (Check() == 0) {
                return;
            }
        }
        position_figure_x = xx;
    }
    turn = old_turn;
    CalcCoord();
}

bool TFigure::Move(int dx, int dy) {
    int old_x = position_figure_x, old_y = position_figure_y; 
    Pos(position_figure_x + dx, position_figure_y + dy);
    int check = Check();
    if (check >= 1) {
        Pos(old_x, old_y);
        if (check == 2) {
            return false; // надо приземлить объект и создать новый
        }
    }
    return true;
}

int TFigure::Check() {
    CalcCoord();
    for (int position_figure_x = 0; position_figure_x < coordinateCount; ++position_figure_x) { //надо вернуть фигуру обратно
        if (coord[position_figure_x].X < 0 || coord[position_figure_x].X >= field_width) {
            return 1;
        }
    }
    for (int position_figure_y = 0; position_figure_y < coordinateCount; ++position_figure_y) { // если она в поле и упала, то приземляем
        if (coord[position_figure_y].Y >= field_height || field->field[coord[position_figure_y].Y][coord[position_figure_y].X] == c_figure_down) {
            return 2;
        }
    }
    return 0;
}

void TFigure::CalcCoord() {
    int new_coordinates_x, new_coordinates_y;
    coordinateCount = 0;
    for (int x_shape_width = 0; x_shape_width < shape_width; ++x_shape_width) {
        for (int y_shape_height = 0; y_shape_height < shape_height; ++y_shape_height) {
            if (vid[y_shape_height][x_shape_width] == '*') {
                if (turn == 0) {
                    new_coordinates_x = position_figure_x + x_shape_width, new_coordinates_y = position_figure_y + y_shape_height; // 90
                }
                if (turn == 1) {
                    new_coordinates_x = position_figure_x + (shape_height - y_shape_height - 1), new_coordinates_y = position_figure_y + x_shape_width; // 180
                }
                if (turn == 2) {
                    new_coordinates_x = position_figure_x + (shape_width - x_shape_width - 1), new_coordinates_y = position_figure_y + (shape_height - y_shape_height - 1); // 180
                }
                if (turn == 3) {
                    new_coordinates_x = position_figure_x + y_shape_height, new_coordinates_y = position_figure_y + (shape_height - x_shape_width - 1) + (shape_width - shape_height); // 270
                }
                coord[coordinateCount].X = (short)new_coordinates_x;
                coord[coordinateCount].Y = (short)new_coordinates_y;
                ++coordinateCount;
            }
        }
    }
}


void TField::Put(TScreenMap& scr) {
    for (int coordinate_field_width = 0; coordinate_field_width < field_width; ++coordinate_field_width) {
        for (int coordinate_field_height = 0; coordinate_field_height < field_height; ++coordinate_field_height) {
            scr[coordinate_field_height][coordinate_field_width * 2] = scr[coordinate_field_height][coordinate_field_width * 2 + 1] = field[coordinate_field_height][coordinate_field_width]; //x2
        }
    }
}

void TField::Burning() {
    for (int coordinate_field_height = field_height - 1; coordinate_field_height >= 0; --coordinate_field_height) { // проходим по всему полю снизу вверх
        static bool fillLine;
        fillLine = true;
        for (int coordinate_field_width = 0; coordinate_field_width < field_width; ++coordinate_field_width) { // ищем заполненную строку
            if (field[coordinate_field_height][coordinate_field_width] != c_figure_down) {
                fillLine = false;
            }
        }
        if (fillLine) {
            for (int position_figure_y = coordinate_field_height; position_figure_y >= 1; --position_figure_y) {
                
                memcpy(field[position_figure_y], field[position_figure_y - 1], sizeof(field[position_figure_y]));
            }
            ++score;
            if (3 <= score && score <= 6) {
                speed = 2;
            }
            if (score >= 6) {
                speed = 1;
            }
            //return;
        }
    }

    for (int coordinate_field_height = field_width - 1; coordinate_field_height >= 0; --coordinate_field_height) { // проходим по всему полю снизу вверх

        static bool fillColumn;
        fillColumn = true;

        for (int coordinate_field_width = 0; coordinate_field_width <= 10; ++coordinate_field_width) { // ищем заполненную строку

            if (field[coordinate_field_height][coordinate_field_width] != c_figure_down) {
                fillColumn = false;
            }

        }
        if (fillColumn) {

            for (int position_figure_y = 0; position_figure_y >= 10; ++position_figure_y) {

                memcpy(field[position_figure_y], field[position_figure_y - 1], sizeof(field[position_figure_y]));
            }
            //return;
        }
    }
}






int main() {
    int new_speed = 0;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD size = { 50, 30 }; 
    SetConsoleScreenBufferSize(console, size);

    char command[1000]; 

    srand(time(0));

    TGame game;

    while (1) {
        if (speed == 10 && !isMusicPlaying1) {
            level = 1;

            PlaySound(TEXT("TetrisMusic1.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

            isMusicPlaying1 = true;
        }

        if (speed == 2 && !isMusicPlaying2) {
            level = 2;

            PlaySound(TEXT("TetrisMusic2.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

            isMusicPlaying2 = true;
            isMusicPlaying1 = false;

        }
        if (speed == 1 && !isMusicPlaying3) {
            level = 3;

            PlaySound(TEXT("TetrisMusic3.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

            isMusicPlaying3 = true;
            isMusicPlaying1 = false;

        }
        game.PlayerControl();
        game.Move();
        game.Show();
        if (GetKeyState(VK_ESCAPE) < 0) {
            break;
        }
        cout << " Score: " << score << endl;
        cout << "Level: " << level << endl;
        Sleep(50);
    }

    return 0;
}
