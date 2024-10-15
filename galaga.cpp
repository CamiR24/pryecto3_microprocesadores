#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <cstdlib>  // Para rand()
#include <ctime>    // Para srand()
using namespace std;

#define ANCHO_PANTALLA 100
#define ALTO_PANTALLA 40
#define ALTURA_OBJETIVO (ALTO_PANTALLA / 2 - 5)  // Altura objetivo de los alienígenas

enum EstadoNave { MOSTRAR, EXPLOSION, NO_MOSTRAR };

struct Bullet {
    int x, y;
    bool active;
    Bullet(int startX, int startY) : x(startX), y(startY), active(true) {}
};

struct Alien {
    int x, y;
    bool active;
    bool reachedTarget = false;  // Indica si alcanzó la altura objetivo
    bool descendingZigZag = false; // Indica si está descendiendo en zigzag
    Alien(int startX, int startY) : x(startX), y(startY), active(false) {}
};

struct GameState {
    int vidasJugador = 3;
    int puntuacion = 0;
    int playerX = ANCHO_PANTALLA / 2;
    int playerY = ALTO_PANTALLA - 4;
    vector<Bullet> bullets;
    vector<Alien> aliens;
    int aliensEnEscena = 0;  // Control de alienígenas activos en pantalla
    int indiceActual = 0;    // Índice del siguiente grupo de aliens a liberar
};

struct GameMode {
    const vector<string>* naveSeleccionada;
    int anchoNave;
    int altoNave;
    string alienSprite;
    int totalAliens;
    int groupSize;
};

const vector<string> NAVE1 = {
    " A ",
    "/|\\",
};

const vector<string> NAVE2 = {
    "  A  ",
    "<( )>",
    " / \\",
};

void setCursorPosition(int x, int y) {
    COORD coord = {SHORT(x), SHORT(y)};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.bVisible = FALSE;
    info.dwSize = 100;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void renderNaveJugador(int x, int y, const GameMode& mode) {
    for (int i = 0; i < mode.altoNave; ++i) {
        setCursorPosition(x, y + i);
        cout << (*(mode.naveSeleccionada))[i];
    }
}

void renderAliens(const vector<Alien>& aliens, const string& alienSprite) {
    for (const auto& alien : aliens) {
        if (alien.active && alien.y >= 0 && alien.y < ALTO_PANTALLA) {
            setCursorPosition(alien.x, alien.y);
            cout << alienSprite;
        }
    }
}

void render(const GameState& estado, const GameMode& mode) {
    // Limpia la pantalla al inicio de cada frame


    setCursorPosition(0, 0);
    cout << "Vidas: " << estado.vidasJugador << " | Puntuacion: " << estado.puntuacion << endl;

    cout << '+' << string(ANCHO_PANTALLA, '-') << '+' << endl;
    for (int i = 0; i < ALTO_PANTALLA; i++) {
        cout << '|';
        for (int j = 0; j < ANCHO_PANTALLA; j++) {
            bool drawn = false;

            // Dibujar balas
            for (const auto& bullet : estado.bullets) {
                if (bullet.active && bullet.x == j && bullet.y == i) {
                    cout << '|';
                    drawn = true;
                    break;
                }
            }

            // Dibujar alienígenas
            if (!drawn) {
                for (const auto& alien : estado.aliens) {
                    if (alien.active && alien.x <= j && j < alien.x + mode.alienSprite.length() && alien.y == i) {
                        cout << mode.alienSprite[j - alien.x];
                        drawn = true;
                        break;
                    }
                }
            }

            if (!drawn) cout << ' ';
        }
        cout << '|' << endl;
    }
    cout << '+' << string(ANCHO_PANTALLA, '-') << '+' << endl;

    renderNaveJugador(estado.playerX, estado.playerY, mode);
}

void liberarSiguienteAlien(GameState& estado, const GameMode& mode) {
    int aliensPorLiberar = mode.groupSize;
    for (int i = 0; i < aliensPorLiberar && estado.indiceActual < estado.aliens.size(); ++i) {
        Alien& alien = estado.aliens[estado.indiceActual++];
        alien.active = true;          // Activar el alienígena
        alien.reachedTarget = false;  // Asegurarnos de que no ha alcanzado su objetivo aún
        estado.aliensEnEscena++;
    }
}

void updateAliens(GameState& estado, const GameMode& mode) {
    static int zigZagIndex = 0; // Índice del alienígena que comenzará a descender en zigzag

    bool liberarNuevoGrupo = true;

    for (int i = 0; i < estado.aliens.size(); ++i) {
        Alien& alien = estado.aliens[i];
        if (alien.active) {
            if (alien.reachedTarget) {
                if (alien.descendingZigZag) {
                    // Movimiento en zigzag
                    int moveDirection = rand() % 3 - 1;
                    alien.x += moveDirection;
                    alien.y++;

                    if (alien.y >= ALTO_PANTALLA) {
                        alien.active = false;
                        estado.aliensEnEscena--;
                    }
                }
            } else {
                // Movimiento hacia la altura objetivo
                if (alien.y < ALTURA_OBJETIVO) {
                    alien.y++;
                    liberarNuevoGrupo = false;
                } else {
                    alien.reachedTarget = true;
                }
            }
        }
    }

    // Iniciar el descenso en zigzag de los alienígenas uno por uno
    if (estado.aliensEnEscena > 0 && estado.aliens[zigZagIndex].active && estado.aliens[zigZagIndex].reachedTarget && !estado.aliens[zigZagIndex].descendingZigZag) {
        estado.aliens[zigZagIndex].descendingZigZag = true;
        zigZagIndex++;
    }

    // Si todos los alienígenas actuales han comenzado a descender en zigzag, liberamos el siguiente grupo
    if (zigZagIndex >= estado.indiceActual && estado.indiceActual < estado.aliens.size()) {
        liberarNuevoGrupo = true;
    }

    if (liberarNuevoGrupo && estado.aliensEnEscena == 0) {
        liberarSiguienteAlien(estado, mode);
        zigZagIndex = estado.indiceActual - mode.groupSize; // Reiniciamos el índice para el nuevo grupo
    }
}

void update(GameState& estado, const GameMode& mode) {
    for (auto& bullet : estado.bullets) {
        if (bullet.active) {
            bullet.y--;
            if (bullet.y < 0) bullet.active = false;
        }
    }

    estado.bullets.erase(
        remove_if(estado.bullets.begin(), estado.bullets.end(), [](const Bullet& b) { return !b.active; }),
        estado.bullets.end()
    );

    updateAliens(estado, mode);
}

void input(GameState& estado, const GameMode& mode) {
    if (_kbhit()) {
        char tecla = _getch();
        switch (tecla) {
            case 'a':
                if (estado.playerX > 0) estado.playerX--;
                break;
            case 'd':
                if (estado.playerX < ANCHO_PANTALLA - mode.anchoNave) estado.playerX++;
                break;
            case ' ':
                estado.bullets.push_back(Bullet(estado.playerX + mode.anchoNave / 2, estado.playerY - 1));
                break;
        }
    }
}

GameMode mostrarMenu() {
    system("cls");
    cout << "Selecciona el modo de juego:\n";
    cout << "1. Modo con Nave 1 (40 Alienígenas)\n";
    cout << "2. Modo con Nave 2 (50 Alienígenas)\n";
    cout << "Elige una opción: ";

    char opcion;
    while (true) {
        opcion = _getch();
        if (opcion == '1') {
            return {&NAVE1, static_cast<int>(NAVE1[0].size()), static_cast<int>(NAVE1.size()), "<00>", 40, 8};
        } else if (opcion == '2') {
            return {&NAVE2, static_cast<int>(NAVE2[0].size()), static_cast<int>(NAVE2.size()), "<000>", 50, 10};
        } else {
            cout << "\nOpción inválida. Intenta de nuevo: ";
        }
    }
}

void inicializarAliens(GameState& estado, const GameMode& mode) {
    int aliensPorFila = mode.groupSize;
    int filas = mode.totalAliens / aliensPorFila;

    for (int i = 0; i < filas; ++i) {
        for (int j = 0; j < aliensPorFila; ++j) {
            int x = j * (ANCHO_PANTALLA / aliensPorFila);
            int y = -i * 2; // Espaciamos las filas iniciales
            estado.aliens.push_back(Alien(x, y));
        }
    }
}

int main() {
    srand(time(0));

    GameMode modoSeleccionado = mostrarMenu();
    hideCursor();
    system("cls");

    GameState estado;
    inicializarAliens(estado, modoSeleccionado);

    liberarSiguienteAlien(estado, modoSeleccionado); // Liberamos el primer grupo de alienígenas

    while (true) {
        render(estado, modoSeleccionado);
        update(estado, modoSeleccionado);
        input(estado, modoSeleccionado);
        Sleep(50); // Aumentamos el tiempo de sleep para reducir el parpadeo
    }

    return 0;
}
