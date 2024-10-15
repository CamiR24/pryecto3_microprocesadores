#include <iostream>
#include <vector>
#include <string>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

#define ANCHO_PANTALLA 100
#define ALTO_PANTALLA 40
#define ALTURA_OBJETIVO (ALTO_PANTALLA / 2 - 5)

enum EstadoNave { MOSTRAR, EXPLOSION, NO_MOSTRAR };

struct Bullet {
    int x, y;
    bool active;
    Bullet(int startX, int startY) : x(startX), y(startY), active(true) {}
};

struct Explosion {
    int x, y;
    int duration;
    Explosion(int startX, int startY) : x(startX), y(startY), duration(3) {}
};

struct Alien {
    int x, y;
    bool active;
    int vida;
    bool reachedTarget = false;
    bool descending = false;
    int targetX;
    int descentPhase = 0;
    int initialDescentSteps = 3;
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
    int vidaAlien;
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
    move(y, x);
}

void hideCursor() {
    curs_set(0);
}

void renderExplosion(const Explosion& explosion) {
    for (int i = 0; i < EXPLOSION_SPRITE.size(); ++i) {
        int y = explosion.y + i;
        if (y >= 0 && y < ALTO_PANTALLA) {
            setCursorPosition(explosion.x, y + 2);
            printw(EXPLOSION_SPRITE[i].c_str());
        }
    }
}

void renderNaveJugador(int x, int y, const GameMode& mode) {
    for (int i = 0; i < mode.altoNave; ++i) {
        setCursorPosition(x, y + i);
        printw((*(mode.naveSeleccionada))[i].c_str());
    }
}

void renderAliens(const vector<Alien>& aliens, const string& alienSprite) {
    for (const auto& alien : aliens) {
        if (alien.active && alien.y >= 0 && alien.y < ALTO_PANTALLA) {
            setCursorPosition(alien.x, alien.y + 2);
            printw(alienSprite.c_str());
        }
    }
}

void render(const GameState& estado, const GameMode& mode) {
    setCursorPosition(0, 0);
    printw("Vidas: %d | Puntuacion: %d", estado.vidasJugador, estado.puntuacion);

    setCursorPosition(0, 1);
    printw("+%s+", string(ANCHO_PANTALLA, '-').c_str());

    for (int i = 0; i < ALTO_PANTALLA; i++) {
        setCursorPosition(0, i + 2);
        printw("|");
        for (int j = 0; j < ANCHO_PANTALLA; j++) {
            bool drawn = false;

            for (const auto& explosion : estado.explosions) {
                if (explosion.x <= j && j < explosion.x + EXPLOSION_SPRITE[0].length() &&
                    explosion.y <= i && i < explosion.y + EXPLOSION_SPRITE.size()) {
                    printw(EXPLOSION_SPRITE[i - explosion.y].substr(j - explosion.x, 1).c_str());
                    drawn = true;
                    break;
                }
            }

            if (!drawn) {
                for (const auto& bullet : estado.bullets) {
                    if (bullet.active && bullet.x == j && bullet.y == i) {
                        printw("|");
                        drawn = true;
                        break;
                    }
                }
            }

            if (!drawn) {
                for (const auto& alien : estado.aliens) {
                    if (alien.active && alien.x <= j &&
                        j < alien.x + mode.alienSprite.length() && alien.y == i) {
                        printw(mode.alienSprite.substr(j - alien.x, 1).c_str());
                        drawn = true;
                        break;
                    }
                }
            }

            if (!drawn) printw(" ");
        }
        printw("|");
    }

    setCursorPosition(0, ALTO_PANTALLA + 2);
    printw("+%s+", string(ANCHO_PANTALLA, '-').c_str());

    renderNaveJugador(estado.playerX, estado.playerY + 2, mode);
}

void liberarSiguienteAlien(GameState& estado, const GameMode& mode) {
    int aliensPorLiberar = mode.groupSize;
    for (int i = 0; i < aliensPorLiberar && estado.indiceActual < estado.aliens.size(); ++i) {
        Alien& alien = estado.aliens[estado.indiceActual++];
        alien.active = true;
        alien.reachedTarget = false;
        alien.descending = false;
        alien.descentPhase = 0;
        alien.initialDescentSteps = 3;
        alien.vida = mode.vidaAlien;
        estado.aliensEnEscena++;
    }
}

void updateAliens(GameState& estado, const GameMode& mode) {
    for (auto& alien : estado.aliens) {
        if (alien.active) {
            if (alien.descending) {
                if (alien.descentPhase == 0) {
                    int moveDirectionX = rand() % 3 - 1;
                    alien.x += moveDirectionX;
                    alien.y++;
                    alien.initialDescentSteps--;
                    if (alien.initialDescentSteps <= 0) {
                        alien.descentPhase = 1;
                    }
                } else if (alien.descentPhase == 1) {
                    if (alien.x < alien.targetX) {
                        alien.x++;
                    } else if (alien.x > alien.targetX) {
                        alien.x--;
                    }
                    if (alien.x == alien.targetX) {
                        alien.descentPhase = 2;
                    }
                } else if (alien.descentPhase == 2) {
                    int moveDirectionX = rand() % 3 - 1;
                    alien.x += moveDirectionX;
                    alien.y++;
                }

                if (alien.x < 0) alien.x = 0;
                if (alien.x >= ANCHO_PANTALLA - mode.alienSprite.length())
                    alien.x = ANCHO_PANTALLA - mode.alienSprite.length();

                if (alien.y >= estado.playerY && alien.y < estado.playerY + mode.altoNave &&
                    alien.x + mode.alienSprite.length() > estado.playerX &&
                    alien.x < estado.playerX + mode.anchoNave) {
                    estado.vidasJugador--;
                    alien.active = false;
                    estado.aliensEnEscena--;
                    estado.alienCurrentlyDescending = nullptr;
                    estado.explosions.push_back(Explosion(estado.playerX, estado.playerY));

                    if (estado.vidasJugador <= 0) {
                        setCursorPosition(0, ALTO_PANTALLA + 4);
                        printw("\n¡Juego Terminado!");
                        endwin();
                        exit(0);
                    }
                }

                if (alien.y >= ALTO_PANTALLA) {
                    alien.active = false;
                    estado.aliensEnEscena--;
                    if (&alien == estado.alienCurrentlyDescending) {
                        estado.alienCurrentlyDescending = nullptr;
                    }
                }
            } else if (!alien.reachedTarget) {
                if (alien.y < ALTURA_OBJETIVO) {
                    alien.y++;
                } else {
                    alien.reachedTarget = true;
                }
            }
        }
    }

    if (estado.alienCurrentlyDescending == nullptr) {
        vector<Alien*> readyAliens;
        for (auto& alien : estado.aliens) {
            if (alien.active && alien.reachedTarget && !alien.descending) {
                readyAliens.push_back(&alien);
            }
        }
        if (!readyAliens.empty()) {
            int index = rand() % readyAliens.size();
            Alien* selectedAlien = readyAliens[index];
            selectedAlien->descending = true;
            selectedAlien->targetX = rand() % (ANCHO_PANTALLA - mode.alienSprite.length());
            estado.alienCurrentlyDescending = selectedAlien;
        }
    }
}

void updateExplosions(GameState& estado) {
    for (auto& explosion : estado.explosions) {
        explosion.duration--;
    }
    estado.explosions.erase(
        remove_if(estado.explosions.begin(), estado.explosions.end(),
                  [](const Explosion& e) { return e.duration <= 0; }),
        estado.explosions.end());
}

void fireBullet(GameState& estado) {
    estado.bullets.push_back(Bullet(estado.playerX + 1, estado.playerY));
}

void updateBullets(GameState& estado, const GameMode& mode) {
    for (auto& bullet : estado.bullets) {
        if (bullet.active) {
            bullet.y--;
            if (bullet.y < 0) {
                bullet.active = false;
            }
        }
    }

    for (auto& alien : estado.aliens) {
        if (alien.active) {
            for (auto& bullet : estado.bullets) {
                if (bullet.active && bullet.y == alien.y &&
                    bullet.x >= alien.x && bullet.x < alien.x + mode.alienSprite.length()) {
                    bullet.active = false;
                    alien.vida--;
                    if (alien.vida <= 0) {
                        alien.active = false;
                        estado.aliensEnEscena--;
                        estado.puntuacion += 10;
                        estado.explosions.push_back(Explosion(alien.x, alien.y));
                        if (&alien == estado.alienCurrentlyDescending) {
                            estado.alienCurrentlyDescending = nullptr;
                        }
                    }
                    break;
                }
            }
        }
    }

    estado.bullets.erase(
        remove_if(estado.bullets.begin(), estado.bullets.end(),
                  [](const Bullet& b) { return !b.active; }),
        estado.bullets.end());
}

void procesarEntrada(GameState& estado, const GameMode& mode) {
    int input = getch();
    switch (input) {
        case KEY_LEFT:
            estado.playerX = max(estado.playerX - 1, 0);
            break;
        case KEY_RIGHT:
            estado.playerX = min(estado.playerX + 1, ANCHO_PANTALLA - mode.anchoNave);
            break;
        case ' ':
            fireBullet(estado);
            break;
    }
}

int main() {
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    hideCursor();

    GameState estado;

    GameMode mode = {
        &NAVE2,
        static_cast<int>(NAVE2[0].length()),
        static_cast<int>(NAVE2.size()),
        "W",
        100,
        10,
        1,
    };

    for (int i = 0; i < mode.totalAliens; i++) {
        estado.aliens.emplace_back(rand() % (ANCHO_PANTALLA - 1), 0, mode.vidaAlien);
    }

    liberarSiguienteAlien(estado, mode);

    while (true) {
        render(estado, mode);
        procesarEntrada(estado, mode);
        updateBullets(estado, mode);
        updateAliens(estado, mode);
        updateExplosions(estado);
        if (estado.aliensEnEscena == 0) {
            liberarSiguienteAlien(estado, mode);
        }

        refresh();
        usleep(50000);
    }

    endwin();
    return 0;
}

