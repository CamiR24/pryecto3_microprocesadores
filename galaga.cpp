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
    Bullet(int startX, int startY)
        : x(startX), y(startY), active(true) {}
};

struct Explosion {
    int x, y;
    int duration;  // Duración restante para mostrar la explosión
    Explosion(int startX, int startY)
        : x(startX), y(startY), duration(3) {}
};

struct Alien {
    int x, y;
    bool active;
    int vida;  // Añadido: Puntos de vida del alienígena
    bool reachedTarget = false;  // Indica si alcanzó la altura objetivo
    bool descending = false;     // Indica si está descendiendo hacia el jugador
    int targetX;                 // Posición x objetivo (posición del jugador)
    int descentPhase = 0;        // Fase del descenso
    int initialDescentSteps = 3; // Pasos iniciales de descenso en Y
    Alien(int startX, int startY, int vidaInicial)
        : x(startX), y(startY), active(false), vida(vidaInicial), targetX(startX) {}
};

struct GameState {
    int vidasJugador = 3;
    int puntuacion = 0;
    int playerX = ANCHO_PANTALLA / 2;
    int playerY = ALTO_PANTALLA - 4;
    vector<Bullet> bullets;
    vector<Alien> aliens;
    vector<Explosion> explosions;
    int aliensEnEscena = 0;
    int indiceActual = 0;
    Alien* alienCurrentlyDescending = nullptr;
};

struct GameMode {
    const vector<string>* naveSeleccionada;
    int anchoNave;
    int altoNave;
    string alienSprite;
    int totalAliens;
    int groupSize;
    int vidaAlien;  // Añadido: Vida inicial de los alienígenas en este modo
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

const vector<string> EXPLOSION_SPRITE = {
    " \\|/ ",
    "-- --",
    " /|\\ ",
};

void setCursorPosition(int x, int y) {
    COORD coord = {SHORT(x), SHORT(y)};
    SetConsoleCursorPosition(
        GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    HANDLE consoleHandle =
        GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.bVisible = FALSE;
    info.dwSize = 100;
    SetConsoleCursorInfo(consoleHandle, &info);
}

void renderExplosion(const Explosion& explosion) {
    for (int i = 0; i < EXPLOSION_SPRITE.size(); ++i) {
        int y = explosion.y + i;
        if (y >= 0 && y < ALTO_PANTALLA) {
            setCursorPosition(explosion.x,
                              y + 2);  // Ajustar posición vertical
            cout << EXPLOSION_SPRITE[i];
        }
    }
}

void renderNaveJugador(int x, int y,
                       const GameMode& mode) {
    for (int i = 0; i < mode.altoNave; ++i) {
        setCursorPosition(x, y + i);
        cout << (*(mode.naveSeleccionada))[i];
    }
}

void renderAliens(const vector<Alien>& aliens,
                  const string& alienSprite) {
    for (const auto& alien : aliens) {
        if (alien.active && alien.y >= 0 &&
            alien.y < ALTO_PANTALLA) {
            setCursorPosition(alien.x, alien.y + 2);
            cout << alienSprite;
        }
    }
}

void render(const GameState& estado,
            const GameMode& mode) {
    // Limpiar la línea superior
    setCursorPosition(0, 0);
    cout << string(ANCHO_PANTALLA + 2, ' ');
    setCursorPosition(0, 0);
    cout << "Vidas: " << estado.vidasJugador
         << " | Puntuacion: " << estado.puntuacion;

    // Dibujar el borde superior
    setCursorPosition(0, 1);
    cout << '+' << string(ANCHO_PANTALLA, '-') << '+';

    // Renderizar el campo de juego
    for (int i = 0; i < ALTO_PANTALLA; i++) {
        setCursorPosition(0, i + 2);
        cout << '|';
        for (int j = 0; j < ANCHO_PANTALLA; j++) {
            bool drawn = false;

            // Dibujar explosiones
            for (const auto& explosion : estado.explosions) {
                if (explosion.x <= j &&
                    j < explosion.x + EXPLOSION_SPRITE[0].length() &&
                    explosion.y <= i &&
                    i < explosion.y + EXPLOSION_SPRITE.size()) {
                    cout << EXPLOSION_SPRITE[i - explosion.y][j - explosion.x];
                    drawn = true;
                    break;
                }
            }

            if (!drawn) {
                // Dibujar balas
                for (const auto& bullet : estado.bullets) {
                    if (bullet.active && bullet.x == j &&
                        bullet.y == i) {
                        cout << '|';
                        drawn = true;
                        break;
                    }
                }
            }

            if (!drawn) {
                // Dibujar alienígenas
                for (const auto& alien : estado.aliens) {
                    if (alien.active && alien.x <= j &&
                        j < alien.x + mode.alienSprite.length() &&
                        alien.y == i) {
                        cout << mode.alienSprite[j - alien.x];
                        drawn = true;
                        break;
                    }
                }
            }

            if (!drawn) cout << ' ';
        }
        cout << '|';
    }

    // Dibujar el borde inferior
    setCursorPosition(0, ALTO_PANTALLA + 2);
    cout << '+' << string(ANCHO_PANTALLA, '-') << '+';

    // Renderizar la nave del jugador
    renderNaveJugador(estado.playerX,
                      estado.playerY + 2, mode);
}

void liberarSiguienteAlien(GameState& estado,
                           const GameMode& mode) {
    int aliensPorLiberar = mode.groupSize;
    for (int i = 0; i < aliensPorLiberar &&
                    estado.indiceActual < estado.aliens.size();
         ++i) {
        Alien& alien = estado.aliens[estado.indiceActual++];
        alien.active = true;
        alien.reachedTarget = false;
        alien.descending = false;
        alien.descentPhase = 0;
        alien.initialDescentSteps = 3;
        alien.vida = mode.vidaAlien;  // Asignar vida según el modo
        estado.aliensEnEscena++;
    }
}

void updateAliens(GameState& estado,
                  const GameMode& mode) {
    // Actualizar alienígenas
    for (auto& alien : estado.aliens) {
        if (alien.active) {
            if (alien.descending) {
                if (alien.descentPhase == 0) {
                    // Fase 0: Descenso inicial en zigzag
                    int moveDirectionX = rand() % 3 - 1;
                    alien.x += moveDirectionX;
                    alien.y++;
                    alien.initialDescentSteps--;
                    if (alien.initialDescentSteps <= 0) {
                        alien.descentPhase = 1;
                    }
                } else if (alien.descentPhase == 1) {
                    // Fase 1: Ajuste en X hacia targetX
                    if (alien.x < alien.targetX) {
                        alien.x++;
                    } else if (alien.x > alien.targetX) {
                        alien.x--;
                    }
                    if (alien.x == alien.targetX) {
                        alien.descentPhase = 2;
                    }
                } else if (alien.descentPhase == 2) {
                    // Fase 2: Descenso final en zigzag
                    int moveDirectionX = rand() % 3 - 1;
                    alien.x += moveDirectionX;
                    alien.y++;
                }

                // Asegurar límites horizontales
                if (alien.x < 0) alien.x = 0;
                if (alien.x >= ANCHO_PANTALLA -
                                   mode.alienSprite.length())
                    alien.x = ANCHO_PANTALLA -
                              mode.alienSprite.length();

                // Verificar colisión con el jugador
                if (alien.y >= estado.playerY &&
                    alien.y < estado.playerY + mode.altoNave &&
                    alien.x + mode.alienSprite.length() >
                        estado.playerX &&
                    alien.x < estado.playerX + mode.anchoNave) {
                    // Colisión con el jugador
                    estado.vidasJugador--;
                    alien.active = false;
                    estado.aliensEnEscena--;
                    estado.alienCurrentlyDescending = nullptr;

                    // Añadir explosión en la posición del jugador
                    estado.explosions.push_back(
                        Explosion(estado.playerX, estado.playerY));

                    if (estado.vidasJugador <= 0) {
                        setCursorPosition(0,
                                          ALTO_PANTALLA + 4);
                        cout << "\n¡Juego Terminado!";
                        exit(0);
                    }
                }

                if (alien.y >= ALTO_PANTALLA) {
                    alien.active = false;
                    estado.aliensEnEscena--;
                    if (&alien ==
                        estado.alienCurrentlyDescending) {
                        estado.alienCurrentlyDescending = nullptr;
                    }
                }
            } else if (!alien.reachedTarget) {
                // Movimiento hacia la altura objetivo
                if (alien.y < ALTURA_OBJETIVO) {
                    alien.y++;
                } else {
                    alien.reachedTarget = true;
                }
            }
        }
    }

    // Si no hay un alienígena descendiendo, seleccionamos uno
    if (estado.alienCurrentlyDescending == nullptr) {
        vector<Alien*> readyAliens;
        for (auto& alien : estado.aliens) {
            if (alien.active && alien.reachedTarget &&
                !alien.descending) {
                readyAliens.push_back(&alien);
            }
        }
        if (!readyAliens.empty()) {
            int index = rand() % readyAliens.size();
            Alien* selectedAlien = readyAliens[index];
            selectedAlien->descending = true;
            selectedAlien->targetX = estado.playerX;
            selectedAlien->descentPhase = 0;
            selectedAlien->initialDescentSteps = 3;
            estado.alienCurrentlyDescending = selectedAlien;
        } else {
            // Verificar si podemos liberar el siguiente grupo
            bool canLiberateNextGroup = true;
            for (auto& alien : estado.aliens) {
                if (alien.active && !alien.reachedTarget &&
                    !alien.descending) {
                    canLiberateNextGroup = false;
                    break;
                }
            }
            if (canLiberateNextGroup &&
                estado.indiceActual < estado.aliens.size()) {
                liberarSiguienteAlien(estado, mode);
            }
        }
    }
}

void updateExplosions(GameState& estado) {
    for (auto& explosion : estado.explosions) {
        explosion.duration--;
    }
    estado.explosions.erase(
        remove_if(estado.explosions.begin(),
                  estado.explosions.end(),
                  [](const Explosion& e) {
                      return e.duration <= 0;
                  }),
        estado.explosions.end());
}

void update(GameState& estado, const GameMode& mode) {
    // Mover balas
    for (auto& bullet : estado.bullets) {
        if (bullet.active) {
            bullet.y--;
            if (bullet.y < 0) bullet.active = false;
        }
    }

    // Colisiones entre balas y alienígenas
    for (auto& bullet : estado.bullets) {
        if (bullet.active) {
            for (auto& alien : estado.aliens) {
                if (alien.active) {
                    // Verificar colisión
                    if (bullet.x >= alien.x &&
                        bullet.x < alien.x +
                                       mode.alienSprite.length() &&
                        bullet.y == alien.y) {
                        // Colisión detectada
                        bullet.active = false;
                        alien.vida--;  // Reducir vida del alienígena

                        if (alien.vida <= 0) {
                            alien.active = false;
                            estado.puntuacion += 10;
                            estado.aliensEnEscena--;

                            // Añadir explosión en el punto de colisión
                            estado.explosions.push_back(
                                Explosion(alien.x, alien.y));

                            if (&alien ==
                                estado.alienCurrentlyDescending) {
                                estado.alienCurrentlyDescending = nullptr;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    // Eliminar balas inactivas
    estado.bullets.erase(
        remove_if(estado.bullets.begin(),
                  estado.bullets.end(),
                  [](const Bullet& b) { return !b.active; }),
        estado.bullets.end());

    updateAliens(estado, mode);
    updateExplosions(estado);
}

void input(GameState& estado, const GameMode& mode) {
    if (_kbhit()) {
        char tecla = _getch();
        switch (tecla) {
            case 'a':
                if (estado.playerX > 0) estado.playerX--;
                break;
            case 'd':
                if (estado.playerX <
                    ANCHO_PANTALLA - mode.anchoNave)
                    estado.playerX++;
                break;
            case ' ':
                estado.bullets.push_back(
                    Bullet(estado.playerX + mode.anchoNave / 2,
                           estado.playerY - 1));
                break;
        }
    }
}

GameMode mostrarMenu() {
    system("cls");
    cout << "Selecciona el modo de juego:\n";
    cout << "1. Modo 1 (40 Alienígenas)\n";
    cout << "2. Modo 2 (50 Alienígenas)\n";
    cout << "Elige una opción: ";

    char opcion;
    while (true) {
        opcion = _getch();
        if (opcion == '1') {
            return {&NAVE1,
                    static_cast<int>(NAVE1[0].size()),
                    static_cast<int>(NAVE1.size()),
                    "<00>", 40, 8, 1};  // vidaAlien = 1
        } else if (opcion == '2') {
            return {&NAVE2,
                    static_cast<int>(NAVE2[0].size()),
                    static_cast<int>(NAVE2.size()),
                    "<000>", 50, 10, 2};  // vidaAlien = 2
        } else {
            cout << "\nOpción inválida. Intenta de nuevo: ";
        }
    }
}

void inicializarAliens(GameState& estado,
                       const GameMode& mode) {
    int aliensPorFila = mode.groupSize;
    int filas = mode.totalAliens / aliensPorFila;

    for (int i = 0; i < filas; ++i) {
        for (int j = 0; j < aliensPorFila; ++j) {
            int x = j * (ANCHO_PANTALLA / aliensPorFila);
            int y = -i * 2;  // Espaciamos las filas iniciales
            estado.aliens.push_back(Alien(x, y, mode.vidaAlien));
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

    liberarSiguienteAlien(estado,
                          modoSeleccionado);

    while (true) {
        render(estado, modoSeleccionado);
        update(estado, modoSeleccionado);
        input(estado, modoSeleccionado);
        Sleep(5);
    }

    return 0;
}
