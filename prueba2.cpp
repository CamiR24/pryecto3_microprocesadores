#include <iostream>
#include <conio.h>  // Para getch() y detección de teclas
#include <windows.h> // Para SetConsoleCursorPosition y manejo de consola
#include <vector>    // Para manejar múltiples balas
#include <algorithm>

using namespace std;

const int WIDTH = 50;
const int HEIGHT = 20;
int playerX = WIDTH / 2;
int playerY = 17;

// Estructura para manejar una bala
struct Bullet {
    int x, y;
    bool active;

    Bullet(int startX, int startY) : x(startX), y(startY), active(true) {}
};

// Vector para almacenar todas las balas activas
vector<Bullet> bullets;

void drawBorder() {
    // Dibuja la parte superior del borde
    cout << '+';
    for (int i = 0; i < WIDTH; i++) cout << '-';
    cout << '+' << endl;

    // Dibuja los lados del borde
    for (int i = 0; i < HEIGHT; i++) {
        cout << '|';
        for (int j = 0; j < WIDTH; j++) {
            if (j == playerX && i == playerY)
                cout << 'A'; // Posición del jugador
            else {
                bool isBulletHere = false;
                // Comprueba si alguna bala está en esta posición
                for (auto &bullet : bullets) {
                    if (bullet.active && bullet.x == j && bullet.y == i) {
                        cout << '|'; // Dibuja la bala
                        isBulletHere = true;
                        break;
                    }
                }
                if (!isBulletHere) {
                    cout << ' '; // Espacio vacío
                }
            }
        }
        cout << '|' << endl;
    }

    // Dibuja la parte inferior del borde
    cout << '+';
    for (int i = 0; i < WIDTH; i++) cout << '-';
    cout << '+' << endl;
}

void shootBullet() {
    // Agrega una nueva bala al vector de balas
    bullets.push_back(Bullet(playerX, playerY - 1));
}

void moveBullets() {
    // Mueve todas las balas activas hacia arriba
    for (auto &bullet : bullets) {
        if (bullet.active) {
            bullet.y--;  // Mueve la bala hacia arriba
            if (bullet.y < 0) {
                bullet.active = false; // Desactiva la bala si llega al borde superior
            }
        }
    }

    // Elimina las balas inactivas del vector
    bullets.erase(
        remove_if(bullets.begin(), bullets.end(), [](Bullet &b) { return !b.active; }),
        bullets.end()
    );
}

void movePlayer(char input) {
    switch (input) {
        case 'a': if (playerX > 0) playerX--; break;
        case 'd': if (playerX < WIDTH - 1) playerX++; break;
        case 32: shootBullet(); break; // Space para disparar
    }
}

void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

int main() {
    char input;

    while (true) {

        // Orden normal:
        // * input
        // * update(modifica tu GameState)
        // * render(GameState)
            // 

        // Tu orden:
        // * render
        // * update
        // * input

        setCursorPosition(0, 0); // Mueve el cursor al inicio para redibujar la ventana
        drawBorder(); // Dibuja la ventana con el borde, el jugador y las balas

        moveBullets(); // Mueve todas las balas activas

        // Verifica si hay entrada del teclado
        if (_kbhit()) {
            input = _getch(); // Obtiene el carácter presionado
            movePlayer(input); // Mueve al jugador según la entrada
        }

        Sleep(5); // Pequeña pausa para no consumir demasiados recursos
    }

    return 0;
}
