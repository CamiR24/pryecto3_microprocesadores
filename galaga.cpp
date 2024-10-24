#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <cstdlib>   // Para rand()
#include <ctime>     // Para srand()
#include <pthread.h> // Para pthreads

using namespace std;

#define ANCHO_PANTALLA 100
#define ALTO_PANTALLA 40
#define ALTURA_OBJETIVO (ALTO_PANTALLA / 2 - 5) // Altura objetivo de los alienígenas

enum EstadoNave { MOSTRAR, EXPLOSION, NO_MOSTRAR };

struct Bullet {
    int x, y;
    bool active;
    Bullet(int startX, int startY)
        : x(startX), y(startY), active(true) {}
};

struct Explosion {
    int x, y;
    int duration; // Duración restante para mostrar la explosión
    Explosion(int startX, int startY)
        : x(startX), y(startY), duration(6) {}
};

struct Alien {
    int x, y;
    bool active;
    int vida;                      // Puntos de vida del alienígena
    bool reachedTarget = false;    // Indica si alcanzó la altura objetivo
    bool descending = false;       // Indica si está descendiendo hacia el jugador
    int targetX;                   // Posición x objetivo (posición del jugador)
    int descentPhase = 0;          // Fase del descenso
    int initialDescentSteps = 3;   // Pasos iniciales de descenso en Y
    pthread_t threadId = 0;        // ID del hilo asociado al alienígena
    Alien(int startX, int startY, int vidaInicial)
        : x(startX), y(startY), active(false), vida(vidaInicial), targetX(startX) {}
};

struct GameState {
    int vidasJugador = 3;
    int puntuacion = 0;
    int ronda = 1;
    int municion = 500;
    int playerX = ANCHO_PANTALLA / 2;
    int playerY = ALTO_PANTALLA - 4;
    vector<Bullet> bullets;
    vector<Alien> aliens;
    vector<Explosion> explosions;
    int aliensEnEscena = 0;
    int indiceActual = 0;
    bool gameOver = false; // Indicador de fin de juego
    bool gameWin = false;  // Indicador de victoria
    bool alienIsDescending = false; // Nuevo: indica si un alien está descendiendo
};

struct GameMode {
    const vector<string>* naveSeleccionada;
    int anchoNave;
    int altoNave;
    string alienSprite;
    int totalAliens;
    int groupSize;
    int vidaAlien; // Vida inicial de los alienígenas en este modo
};

struct ThreadArgs {
    GameState* estado;
    const GameMode* modo;
    Alien* alien; // Puntero al alienígena para su hilo
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

// Mensajes de Game Over y You Win
const vector<string> GAME_OVER_MESSAGE = {
    "  _____          __  __ ______    ______      ________ _____  ",
    " / ____|   /\\   |  \\/  |  ____|  / __ \\ \\    / /  ____|  __ \\ ",
    "| |  __   /  \\  | \\  / | |__    | |  | \\ \\  / /| |__  | |__) |",
    "| | |_ | / /\\ \\ | |\\/| |  __|   | |  | |\\ \\/ / |  __| |  _  / ",
    "| |__| |/ ____ \\| |  | | |____  | |__| | \\  /  | |____| | \\ \\ ",
    " \\_____/_/    \\_\\_|  |_|______|  \\____/   \\/   |______|_|  \\_\\",
    "                                                              ",
    "                                                              ",
};

const vector<string> YOU_WIN_MESSAGE = {
    " __     ______  _    _  __          _______ _   _ _ ",
    " \\ \\   / / __ \\| |  | | \\ \\        / /_   _| \\ | | |",
    "  \\ \\_/ / |  | | |  | |  \\ \\  /\\  / /  | | |  \\| | |",
    "   \\   /| |  | | |  | |   \\ \\/  \\/ /   | | | .   | |",
    "    | | | |__| | |__| |    \\  /\\  /   _| |_| |\\  |_|",
    "    |_|  \\____/ \\____/      \\/  \\/   |_____|_| \\_(_)",
    "                                                    ",
    "                                                    ",
};

pthread_mutex_t gameStateMutex;

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

void clearGameArea() {
    // Limpiar el área del juego, excepto los bordes
    for (int i = 2; i < ALTO_PANTALLA + 2; ++i) {
        setCursorPosition(1, i);
        cout << string(ANCHO_PANTALLA, ' ');
    }
}

void renderMessage(const vector<string>& message) {
    clearGameArea();
    int startY = ALTO_PANTALLA / 2 - message.size() / 2 + 2;
    int startX = ANCHO_PANTALLA / 2 - message[0].length() / 2 + 1;
    for (int i = 0; i < message.size(); ++i) {
        setCursorPosition(startX, startY + i);
        cout << message[i];
    }
    setCursorPosition(0, ALTO_PANTALLA + 3);
}

void renderExplosion(const Explosion& explosion) {
    for (int i = 0; i < EXPLOSION_SPRITE.size(); ++i) {
        int y = explosion.y + i;
        if (y >= 0 && y < ALTO_PANTALLA) {
            setCursorPosition(explosion.x,
                              y + 2); // Ajustar posición vertical
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
         << " | Puntuacion: " << estado.puntuacion
         << " | Ronda: " << estado.ronda
         << " | Municion: " << estado.municion;

    // Dibujar el borde superior
    setCursorPosition(0, 1);
    cout << '+' << string(ANCHO_PANTALLA, '-') << '+';

    if (estado.gameOver) {
        renderMessage(GAME_OVER_MESSAGE);
    } else if (estado.gameWin) {
        renderMessage(YOU_WIN_MESSAGE);
    } else {
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
}

void* alienThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    GameState* estado = args->estado;
    const GameMode* mode = args->modo;
    Alien* alien = args->alien;

    const int ALIEN_UPDATE_INTERVAL_MS = 50;

    while (true) {
        // Primero, verificamos si el alienígena está activo
        if (!alien->active) {
            break;
        }

        // Verificar si el juego ha terminado
        pthread_mutex_lock(&gameStateMutex);
        bool gameOver = estado->gameOver;
        bool gameWin = estado->gameWin;
        pthread_mutex_unlock(&gameStateMutex);

        if (gameOver || gameWin) {
            break;
        }

        // Actualizar el alienígena
        if (alien->active) {
            if (alien->descending) {
                // Lógica de descenso
                if (alien->descentPhase == 0) {
                    // Fase 0: Descenso inicial en zigzag
                    int moveDirectionX = rand() % 3 - 1;
                    alien->x += moveDirectionX;
                    alien->y++;
                    alien->initialDescentSteps--;
                    if (alien->initialDescentSteps <= 0) {
                        alien->descentPhase = 1;
                    }
                } else if (alien->descentPhase == 1) {
                    // Fase 1: Ajuste en X hacia targetX
                    if (alien->x < alien->targetX) {
                        alien->x++;
                    } else if (alien->x > alien->targetX) {
                        alien->x--;
                    }
                    if (alien->x == alien->targetX) {
                        alien->descentPhase = 2;
                    }
                } else if (alien->descentPhase == 2) {
                    // Fase 2: Descenso final en zigzag
                    int moveDirectionX = rand() % 3 - 1;
                    alien->x += moveDirectionX;
                    alien->y++;
                }

                // Asegurar límites horizontales
                if (alien->x < 0) alien->x = 0;
                if (alien->x >= ANCHO_PANTALLA - mode->alienSprite.length())
                    alien->x = ANCHO_PANTALLA - mode->alienSprite.length();

                // Verificar colisión con el jugador
                int playerX, playerY, playerAncho, playerAlto;
                pthread_mutex_lock(&gameStateMutex);
                playerX = estado->playerX;
                playerY = estado->playerY;
                playerAncho = mode->anchoNave;
                playerAlto = mode->altoNave;
                pthread_mutex_unlock(&gameStateMutex);

                if (alien->y >= playerY &&
                    alien->y < playerY + playerAlto &&
                    alien->x + mode->alienSprite.length() > playerX &&
                    alien->x < playerX + playerAncho) {
                    // Colisión con el jugador
                    pthread_mutex_lock(&gameStateMutex);
                    estado->vidasJugador--;
                    alien->active = false;
                    estado->aliensEnEscena--;
                    estado->alienIsDescending = false;

                    // Añadir explosión en la posición del jugador
                    estado->explosions.push_back(
                        Explosion(playerX, playerY));

                    if (estado->vidasJugador <= 0) {
                        estado->gameOver = true;
                    }
                    pthread_mutex_unlock(&gameStateMutex);
                }

                // Verificar si el alienígena ha salido de la pantalla
                if (alien->y >= ALTO_PANTALLA) {
                    pthread_mutex_lock(&gameStateMutex);
                    alien->active = false;
                    estado->aliensEnEscena--;
                    estado->alienIsDescending = false;
                    pthread_mutex_unlock(&gameStateMutex);
                }
            } else if (!alien->reachedTarget) {
                // Movimiento hacia la altura objetivo
                if (alien->y < ALTURA_OBJETIVO) {
                    alien->y++;
                } else {
                    alien->reachedTarget = true;
                }
            } else {
                // Intentar convertirse en el alienígena que desciende
                pthread_mutex_lock(&gameStateMutex);
                if (!estado->alienIsDescending) {
                    estado->alienIsDescending = true;
                    alien->descending = true;
                    alien->targetX = estado->playerX;
                    alien->descentPhase = 0;
                    alien->initialDescentSteps = 3;
                }
                pthread_mutex_unlock(&gameStateMutex);
            }
        }

        Sleep(ALIEN_UPDATE_INTERVAL_MS);
    }

    delete args; // Liberar la memoria asignada
    return NULL;
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
        alien.vida = mode.vidaAlien + (estado.ronda - 1); // Asignar vida según el modo y ronda
        estado.aliensEnEscena++;

        // Crear hilo para el alienígena
        ThreadArgs* alienArgs = new ThreadArgs;
        alienArgs->estado = &estado;
        alienArgs->modo = &mode;
        alienArgs->alien = &alien;
        pthread_create(&alien.threadId, NULL, alienThread, (void*)alienArgs);
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

void updateBullets(GameState& estado, const GameMode& mode) {
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
                        bullet.x < alien.x + mode.alienSprite.length() &&
                        bullet.y == alien.y) {
                        // Colisión detectada
                        bullet.active = false;
                        alien.vida--; // Reducir vida del alienígena

                        if (alien.vida <= 0) {
                            alien.active = false;
                            estado.puntuacion += 10;
                            estado.aliensEnEscena--;

                            // Añadir explosión en el punto de colisión
                            estado.explosions.push_back(
                                Explosion(alien.x, alien.y));

                            // Si el alienígena estaba descendiendo, liberar la bandera
                            if (alien.descending) {
                                estado.alienIsDescending = false;
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
}

void* updateThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    GameState* estado = args->estado;
    const GameMode* modo = args->modo;

    const int UPDATE_INTERVAL_MS = 16; // Aproximadamente 60 actualizaciones por segundo

    DWORD lastUpdateTime = GetTickCount();

    while (true) {
        DWORD currentTime = GetTickCount();
        DWORD elapsedTime = currentTime - lastUpdateTime;

        if (elapsedTime >= UPDATE_INTERVAL_MS) {
            pthread_mutex_lock(&gameStateMutex);
            if (estado->gameOver || estado->gameWin) {
                pthread_mutex_unlock(&gameStateMutex);
                break;
            }

            updateBullets(*estado, *modo);
            updateExplosions(*estado);

            // Verificar si podemos liberar el siguiente grupo
            if (estado->aliensEnEscena == 0 && estado->indiceActual < estado->aliens.size()) {
                estado->ronda++;
                liberarSiguienteAlien(*estado, *modo);
            }

            // Verificar si se ha ganado el juego
            if (estado->aliensEnEscena == 0 && estado->indiceActual >= estado->aliens.size()) {
                estado->gameWin = true;
            }

            pthread_mutex_unlock(&gameStateMutex);

            lastUpdateTime = currentTime;
        } else {
            Sleep(1); // Esperar un poco antes de volver a verificar
        }
    }

    return NULL;
}

void* inputThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    GameState* estado = args->estado;
    const GameMode* modo = args->modo;

    while (true) {
        pthread_mutex_lock(&gameStateMutex);
        if (estado->gameOver || estado->gameWin) {
            pthread_mutex_unlock(&gameStateMutex);
            break;
        }
        pthread_mutex_unlock(&gameStateMutex);

        // Usar GetAsyncKeyState para detectar teclas presionadas
        bool keyPressed = false;

        // Movimiento a la izquierda ('A' o tecla de flecha izquierda)
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
            pthread_mutex_lock(&gameStateMutex);
            if (estado->playerX > 0) estado->playerX--;
            pthread_mutex_unlock(&gameStateMutex);
            keyPressed = true;
        }

        // Movimiento a la derecha ('D' o tecla de flecha derecha)
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            pthread_mutex_lock(&gameStateMutex);
            if (estado->playerX < ANCHO_PANTALLA - modo->anchoNave)
                estado->playerX++;
            pthread_mutex_unlock(&gameStateMutex);
            keyPressed = true;
        }

        // Disparar (barra espaciadora)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            pthread_mutex_lock(&gameStateMutex);
            if(estado->municion <= 0)
                break;

            estado->municion--;
            estado->bullets.push_back(
                Bullet(estado->playerX + modo->anchoNave / 2,
                       estado->playerY - 1));
            pthread_mutex_unlock(&gameStateMutex);
            keyPressed = true;
            Sleep(50); // Pequeña pausa para evitar disparos demasiado rápidos
        }

        if (!keyPressed) {
            Sleep(5); // Evitar consumir CPU innecesariamente
        } else {
            Sleep(50); // Controlar la velocidad del movimiento
        }
    }

    return NULL;
}


void* renderThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    GameState* estado = args->estado;
    const GameMode* modo = args->modo;

    const int RENDER_INTERVAL_MS = 16; // Aproximadamente 60 FPS

    DWORD lastRenderTime = GetTickCount();

    while (true) {
        DWORD currentTime = GetTickCount();
        DWORD elapsedTime = currentTime - lastRenderTime;

        if (elapsedTime >= RENDER_INTERVAL_MS) {
            pthread_mutex_lock(&gameStateMutex);
            bool gameEnded = estado->gameOver || estado->gameWin;
            GameState localState = *estado; // Copiar el estado del juego
            pthread_mutex_unlock(&gameStateMutex);

            render(localState, *modo);

            if (gameEnded) {
                Sleep(5000); // Esperar 5 segundos
                break;
            }

            lastRenderTime = currentTime;
        } else {
            Sleep(1); // Esperar un poco antes de volver a renderizar
        }
    }

    return NULL;
}

GameMode mostrarMenu() {
    system("cls");
    cout << "Selecciona el modo de juego:\n";
    cout << "1. Modo 1 (40 Alienígenas)\n";
    cout << "2. Modo 2 (50 Alienígenas)\n";
    cout << "3. Salir del juego\n";
    cout << "Elige una opción: ";

    char opcion;
    while (true) {
        opcion = _getch();
        if (opcion == '1') {
            return {&NAVE1,
                    static_cast<int>(NAVE1[0].size()),
                    static_cast<int>(NAVE1.size()),
                    "<00>", 40, 8, 1}; // vidaAlien = 1
        } else if (opcion == '2') {
            return {&NAVE2,
                    static_cast<int>(NAVE2[0].size()),
                    static_cast<int>(NAVE2.size()),
                    "<000>", 50, 10, 2}; // vidaAlien = 2
        } else if (opcion == '3') {
            cout << "\n¡Hasta pronto!\n";
            exit(0);
        }
        else {
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
            int y = -i * 2; // Espaciamos las filas iniciales
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

    pthread_mutex_init(&gameStateMutex, NULL);

    ThreadArgs threadArgs;
    threadArgs.estado = &estado;
    threadArgs.modo = &modoSeleccionado;
    threadArgs.alien = nullptr; // No es necesario aquí

    // Liberar el primer grupo de alienígenas
    liberarSiguienteAlien(estado, modoSeleccionado);

    pthread_t inputThreadId, updateThreadId, renderThreadId;

    pthread_create(&inputThreadId, NULL, inputThread, (void*)&threadArgs);
    pthread_create(&updateThreadId, NULL, updateThread, (void*)&threadArgs);
    pthread_create(&renderThreadId, NULL, renderThread, (void*)&threadArgs);

    // Esperar a que los hilos terminen
    pthread_join(inputThreadId, NULL);
    pthread_join(updateThreadId, NULL);
    pthread_join(renderThreadId, NULL);

    // Esperar a que los hilos de los alienígenas terminen
    for (auto& alien : estado.aliens) {
        if (alien.threadId) {
            pthread_join(alien.threadId, NULL);
        }
    }

    pthread_mutex_destroy(&gameStateMutex);

    return 0;
}
